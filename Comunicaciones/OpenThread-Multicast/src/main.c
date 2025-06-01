#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#define UDP_PORT 12345

static struct net_context *udp_context;

static void udp_receive_cb(struct net_context *context, struct net_pkt *pkt,
    int status, void *user_data)
{
ARG_UNUSED(context);
ARG_UNUSED(user_data);
ARG_UNUSED(status);

if (!pkt) {
printk("UDP receive callback: no packet\n");
return;
}

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

void main(void)
{
    int ret;
    struct sockaddr_in6 local_addr = { 0 };

    printk("Starting UDP listener...\n");

    // Create UDP context
    ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &udp_context);
    if (ret < 0) {
        printk("Failed to get UDP context: %d\n", ret);
        return;
    }

    // Bind to port
    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_port = htons(UDP_PORT);
    local_addr.sin6_addr = in6addr_any;

    ret = net_context_bind(udp_context, (struct sockaddr *)&local_addr,
                           sizeof(local_addr));
    if (ret < 0) {
        printk("Failed to bind UDP context: %d\n", ret);
        return;
    }

    // Register receive callback
    ret = net_context_recv(udp_context, udp_receive_cb, K_NO_WAIT, NULL);
    if (ret < 0) {
        printk("Failed to set receive callback: %d\n", ret);
        return;
    }

    printk("UDP socket bound and listening on port %d\n", UDP_PORT);
}
