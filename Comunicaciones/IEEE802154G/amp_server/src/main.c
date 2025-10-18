#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sem.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 128
#define RESPONSE_MESSAGE "Y el corazon tucun tucun tucun"
#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY 5

static struct k_sem sync_sem;
static char recv_buffer[BUFFER_SIZE];
static struct sockaddr_in6 client_addr;
static socklen_t client_addr_len;
static int server_sock;

static int set_tx_power(struct net_if *iface, int16_t dbm)
{
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

    printk("TX power set to %d dBm\n", dbm);
    return 0;
}

K_THREAD_STACK_DEFINE(receive_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(transmit_stack, THREAD_STACK_SIZE);
struct k_thread receive_thread_data;
struct k_thread transmit_thread_data;

void receive_thread(void *arg1, void *arg2, void *arg3)
{
    ssize_t received;

    while (1) {
        memset(recv_buffer, 0, BUFFER_SIZE);
        client_addr_len = sizeof(client_addr);

        received = recvfrom(server_sock, recv_buffer, BUFFER_SIZE - 1, 0,
                            (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (received < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printk("Failed to receive data: %d (%s)\n", errno, strerror(errno));
            }
            // EAGAIN/EWOULDBLOCK es normal con non-blocking
            k_sleep(K_MSEC(50));
            continue;
        }

        recv_buffer[received] = '\0';
        printk("Received %zd bytes from client: %s\n", received, recv_buffer);

        k_sem_give(&sync_sem);
    }
}

void transmit_thread(void *arg1, void *arg2, void *arg3)
{
    ssize_t sent;

    while (1) {
        k_sem_take(&sync_sem, K_FOREVER);

        sent = sendto(server_sock, RESPONSE_MESSAGE, strlen(RESPONSE_MESSAGE), 0,
                      (struct sockaddr *)&client_addr, client_addr_len);
        
        if (sent < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printk("Failed to send response: %d (%s)\n", errno, strerror(errno));
            }
        } else {
            printk("Response sent to client (%zd bytes)\n", sent);
        }

        k_sleep(K_MSEC(100));
    }
}

void main(void)
{
    struct sockaddr_in6 server_addr;
    struct net_if *iface;
    int flags;

    printk("Starting UDP server with semaphore synchronization\n");

    iface = net_if_get_default();
    if (!iface) {
        printk("No default net interface\n");
        return;
    }

    net_if_up(iface);
    k_sleep(K_MSEC(1500));
    
    set_tx_power(iface, 20);

    k_sem_init(&sync_sem, 0, 1);

    server_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (server_sock < 0) {
        printk("Failed to create socket: %d\n", errno);
        return;
    }
    printk("Socket created successfully\n");

    // Hacer el socket non-blocking
    flags = fcntl(server_sock, F_GETFL, 0);
    if (flags < 0) {
        printk("Failed to get socket flags: %d\n", errno);
        close(server_sock);
        return;
    }
    
    if (fcntl(server_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        printk("Failed to set non-blocking: %d\n", errno);
        close(server_sock);
        return;
    }
    printk("Socket set to non-blocking mode\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERVER_PORT);
    server_addr.sin6_addr = in6addr_any;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printk("Failed to bind socket: %d\n", errno);
        close(server_sock);
        return;
    }
    printk("Socket bound to port %d\n", SERVER_PORT);

    k_thread_create(&receive_thread_data, receive_stack, THREAD_STACK_SIZE,
                    receive_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&transmit_thread_data, transmit_stack, THREAD_STACK_SIZE,
                    transmit_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER);
    }
}