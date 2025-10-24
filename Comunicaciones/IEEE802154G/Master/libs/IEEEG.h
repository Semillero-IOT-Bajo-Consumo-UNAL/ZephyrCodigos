/*
 * ieeeg_server.c
 * Versión corregida para Zephyr 4
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/socket.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#define BUFFER_SIZE 128

/* Thread */
#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY 5
K_THREAD_STACK_DEFINE(receive_stack, THREAD_STACK_SIZE);
static struct k_thread receive_thread_data;

/* Callback informado por el usuario */
static void (*bindedServerFunction)(ssize_t received, char message[]) = NULL;

/* Socket file descriptor y puerto por defecto (usado por sendMessage) */
static int sock_fd = -1;
static uint16_t server_port = 0;

/* Obtener interfaz por defecto (si la necesitas) */
struct net_if* IEEEG_getIFace(void){
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        printk("[ERROR]: No encontre una interfaz de red.\n");
        return NULL;
    }

    net_if_up(iface);
    return iface;
}

/* (Opcional) ajustar potencia de antena -- revisa driver y estructura según tu radio */
static int IEEEG_setAntennaPower(struct net_if *iface, int16_t dbm){
    struct {
        int16_t dbm;
    } tx_power = {
        .dbm = dbm
    };

    int ret = net_mgmt(NET_REQUEST_IEEE802154_SET_TX_POWER,
                       iface, &tx_power, sizeof(tx_power));
    if (ret) {
        printk("Failed to set TX power to %d dBm: %d\n", dbm, ret);
        return ret;
    }

    printk("Potencia de la antena: %d dBm\n", dbm);
    return 0;
}

/* Inicializa y bindea socket UDP IPv6 al puerto indicado.
 * Retorna descriptor de socket o -1 en error.
 */
static int initialize_socket_and_bind(int port)
{
    int s;
    struct sockaddr_in6 local = {0};

    s = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        printk("Failed to create socket: %d\n", errno);
        return -1;
    }

    /* Bind a ANY (::) en el puerto indicado */
    local.sin6_family = AF_INET6;
    local.sin6_port = htons(port);
    local.sin6_addr = in6addr_any; /* bind a cualquier IPv6 local */

    if (zsock_bind(s, (struct sockaddr *)&local, sizeof(local)) < 0) {
        printk("bind() failed: errno=%d (%s)\n", errno, strerror(errno));
        zsock_close(s);
        return -1;
    }

    /* Intentar poner non-blocking (no crítico si falla) */
    #ifdef CONFIG_NET_SOCKETS_POSIX_NAMES
    {
        int flags = zsock_fcntl(s, F_GETFL, 0);
        if (flags >= 0) {
            if (zsock_fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
                printk("Warning: no se pudo poner non-blocking via fcntl: %d\n", errno);
            }
        }
    }
    #endif

    printk("Socket creado y bindeado (fd=%d) port=%d\n", s, port);
    return s;
}

/* Hilo receptor: usa zsock_poll para esperar datos de forma eficiente */
void receive_message(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    char recv_buffer[BUFFER_SIZE];
    ssize_t received;
    struct sockaddr_in6 from_addr;
    socklen_t from_addr_len;
    struct zsock_pollfd fds[1];

    if (sock_fd < 0) {
        printk("receive_message: socket inválido\n");
        return;
    }

    fds[0].fd = sock_fd;
    fds[0].events = ZSOCK_POLLIN;

    while (1) {
        /* poll con timeout de 50 ms (evita busy-waiting) */
        int rc = zsock_poll(fds, 1, 50);
        if (rc < 0) {
            printk("poll error: %d\n", errno);
            k_sleep(K_MSEC(50));
            continue;
        } else if (rc == 0) {
            /* timeout: nada por hacer */
            continue;
        }

        if (fds[0].revents & ZSOCK_POLLIN) {
            memset(recv_buffer, 0, sizeof(recv_buffer));
            from_addr_len = sizeof(from_addr);

            received = zsock_recvfrom(sock_fd, recv_buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&from_addr, &from_addr_len);
            if (received < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    printk("Recv failed: errno=%d (%s)\n", errno, strerror(errno));
                }
                continue;
            } else if (received > 0) {
                recv_buffer[received] = '\0';
                if (bindedServerFunction) {
                    bindedServerFunction(received, recv_buffer);
                } else {
                    printk("Mensaje recibido pero callback no registrado: %s\n", recv_buffer);
                }
            }
        }
    }
}

/* API pública: inicia servidor UDP en ipv6.
 * Si ip == NULL o ip == "" bindea a :: (ANY). Si ip es no-nula y válida, bindea a esa IP.
 */
/* Asegúrate de tener estas includes (ya tenías algunas):
 * #include <zephyr/kernel.h>
 * #include <zephyr/net/socket.h>
 * #include <stdio.h>
 * #include <string.h>
 * #include <errno.h>
 */

int IEEEG_StartComunications(int port)
{
    /* Si hay una dirección por Kconfig, la usamos como valor por defecto */
    #ifdef CONFIG_NET_CONFIG_MY_IPV6_ADDR
        const char *ip = CONFIG_NET_CONFIG_MY_IPV6_ADDR;
    #else
        const char *ip = NULL;
        printf("Papacho tienes que configurar una IP con CONFIG_NET_CONFIG_MY_IPV6_ADDR\n")
        return 1
    #endif

    printk("Iniciando socket IPv6 en puerto %d (ip: %s)\n", port, (ip ? ip : "(null)"));

    struct net_if* iface = IEEEG_getIFace();
    (void) iface;

    if (ip && ip[0] != '\0') {
        struct sockaddr_in6 local = {0};
        local.sin6_family = AF_INET6;
        local.sin6_port = htons(port);
        if (inet_pton(AF_INET6, ip, &local.sin6_addr) != 1) {
            printk("IEEG_StartServer: IPv6 inválida: %s. Se usará in6addr_any\n", ip);
            sock_fd = initialize_socket_and_bind(port);
        } else {
            sock_fd = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
            if (sock_fd < 0) {
                printk("Failed to create socket: %d\n", errno);
                return -1;
            }
            if (zsock_bind(sock_fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
                printk("bind() failed (ip specific): errno=%d (%s)\n", errno, strerror(errno));
                zsock_close(sock_fd);
                sock_fd = -1;
                return -1;
            }
        }
    } else {
        sock_fd = initialize_socket_and_bind(port);
    }

    if (sock_fd < 0) {
        return -1;
    }

    server_port = (uint16_t)port;

    k_thread_create(&receive_thread_data, receive_stack, THREAD_STACK_SIZE,
                    receive_message, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    printk("Servidor UDP IPv6 iniciado (fd=%d, port=%d)\n", sock_fd, server_port);
    return 0;
}


/* Vincular callback para recibir mensajes */
int IEEEG_bindOnMessageEvent(void (*f)(ssize_t received,char message[])){
    bindedServerFunction = f;
    return 0;
}

/* Enviar mensaje UDP a ip (usa puerto por defecto con el que arrancó el server).
 * Si quieres enviar a otro puerto, añade una sobrecarga o modifica API.
 */
void IEEEG_sendMessage(char* mensaje, char* ip)
{
    if (!ip || ip[0] == '\0') {
        printk("IEEG_sendMessage: ip destino inválida\n");
        return;
    }
    if (server_port == 0) {
        printk("IEEG_sendMessage: puerto por defecto no inicializado\n");
        return;
    }
    if (sock_fd < 0) {
        printk("IEEG_sendMessage: socket no inicializado\n");
        return;
    }

    struct sockaddr_in6 dest = {0};
    dest.sin6_family = AF_INET6;
    dest.sin6_port = htons(server_port);

    if (inet_pton(AF_INET6, ip, &dest.sin6_addr) != 1) {
        printk("Invalid IPv6 address: %s\n", ip);
        return;
    }

    ssize_t sent = zsock_sendto(sock_fd, mensaje, strlen(mensaje), 0,
                    (struct sockaddr *)&dest,
                    sizeof(struct sockaddr_in6));

    if (sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printk("Send failed: errno=%d (%s)\n", errno, strerror(errno));
        }
    } else {
        printk("Sent %zd bytes to [%s]:%d -> %s\n", sent, ip, server_port, mensaje);
    }
}

/* Para parar el servidor */
void IEEEG_Stop(void)
{
    if (sock_fd >= 0) {
        zsock_close(sock_fd);
        sock_fd = -1;
    }

    /* Abortar hilo receptor */
    k_thread_abort(&receive_thread_data);

    server_port = 0;
    bindedServerFunction = NULL;

    printk("Servidor detenido y recursos liberados\n");
}

/* Función de inicio general (tu IEEEG_Start original) */
int IEEEG_Start(void){
    struct net_if* interfaz = IEEEG_getIFace();
    (void) interfaz;
    IEEEG_setAntennaPower(interfaz,20);
    return 0;
}
