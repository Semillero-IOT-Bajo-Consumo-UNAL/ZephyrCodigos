#include <zephyr.h>
#include <net/socket.h>

void send_thread_message(void)
{
    int sock;
    struct sockaddr_in6 peer = {0};
    const char *msg = "¡Hola desde Zephyr!";
    int err;

    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("Error al crear socket: %d\n", sock);
        return;
    }

    peer.sin6_family = AF_INET6;
    net_addr_pton(AF_INET6, "fd11:22:33:44::2", &peer.sin6_addr);
    peer.sin6_port = htons(1234);

    err = sendto(sock, msg, strlen(msg), 0,
                 (struct sockaddr *)&peer, sizeof(peer));
    if (err < 0) {
        printk("Error enviando: %d\n", err);
    } else {
        printk("Enviado %d bytes\n", err);
    }

    close(sock);
}
