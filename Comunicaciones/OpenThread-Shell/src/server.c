#include <zephyr.h>
#include <net/socket.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(udp_receiver, LOG_LEVEL_INF);

#define LISTEN_PORT 1234

void main(void)
{
    int sock;
    struct sockaddr_in6 local_addr = { 0 };
    char buf[128];
    struct sockaddr_in6 src_addr;
    socklen_t addr_len = sizeof(src_addr);
    int len;

    /* 1) Crear y bindear el socket UDP */
    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        LOG_ERR("socket() failed: %d", sock);
        return;
    }

    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_addr = in6addr_any;
    local_addr.sin6_port = htons(LISTEN_PORT);

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        LOG_ERR("bind() failed: %d", errno);
        close(sock);
        return;
    }

    LOG_INF("Esperando mensajes en puerto %d …", LISTEN_PORT);

    /* 2) Bucle de recepción */
    while (1) {
        len = recvfrom(sock, buf, sizeof(buf)-1, 0,
                       (struct sockaddr *)&src_addr, &addr_len);
        if (len < 0) {
            LOG_ERR("recvfrom() failed: %d", len);
            break;
        }
        buf[len] = '\0';
        char addr_str[INET6_ADDRSTRLEN];
        net_addr_ntop(AF_INET6, &src_addr.sin6_addr, addr_str, sizeof(addr_str));
        LOG_INF("Recibido %d bytes de [%s]:%d: \"%s\"",
                len, addr_str, ntohs(src_addr.sin6_port), buf);
    }

    close(sock);
}
