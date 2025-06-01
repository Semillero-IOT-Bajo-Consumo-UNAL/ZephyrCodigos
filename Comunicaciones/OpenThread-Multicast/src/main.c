#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
static struct net_context *udp_context;
/*
Eventos
*/

static void udp_receive(struct net_context *context, struct net_pkt *pkt,int status, void *user_data){
    ARG_UNUSED(context);
    ARG_UNUSED(user_data);
    ARG_UNUSED(status);

    size_t len = net_pkt_remaining_data(pkt);
    uint8_t buf[len + 1];

    if (net_pkt_read(pkt, buf, len) < 0) {
        printk("Failed to read UDP payload\n");
        net_pkt_unref(pkt);
        return;
    }

    buf[len] = '\0'; // Null-terminate
    printk("Received UDP message (len %zu): %s\n", len, buf);

    net_pkt_unref(pkt);
}


int udp_send(const char *msg, const char *dest_ip, uint16_t dest_port)
{
    struct sockaddr_in6 dest_addr = { 0 };
    int ret;

    // Configura dirección destino
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(dest_port);

    ret = net_addr_pton(AF_INET6, dest_ip, &dest_addr.sin6_addr);
    if (ret < 0) {
        printk("Invalid IPv6 address: %s\n", dest_ip);
        return ret;
    }

    // Envía directamente el buffer como mensaje UDP
    ret = net_context_sendto(udp_context,
                             msg,
                             strlen(msg),
                             (struct sockaddr *)&dest_addr,
                             sizeof(dest_addr),
                             NULL,         // callback
                             K_NO_WAIT,    // timeout
                             NULL);        // user_data

    if (ret < 0) {
        printk("UDP send failed: %d\n", ret);
    }

    return ret;
}

/*
Cuerpo principal
*/


static int enviarmsg(const struct shell *shell, size_t argc, char **argv)
{
    udp_send("Hola desde Zephyr", "fe03::1", 12345); 
    return 0;
}

void main(void)
{
    // Obtenemos el contexto UDP
    net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &udp_context);

    // "Bindeamos" el puerto 12345 para recibir y enviar los datos
    struct sockaddr_in6 local_addr = { 0 };
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_port = htons(12345);
    local_addr.sin6_addr = in6addr_any;
    net_context_bind(udp_context, (struct sockaddr *)&local_addr,sizeof(local_addr));

    // Cuando recibimos datos los mandamos a udp_receive_cb
    net_context_recv(udp_context, udp_receive, K_NO_WAIT, NULL);


    SHELL_CMD_REGISTER(comunicar, NULL, "Enciende un LED", enviarmsg);
}
