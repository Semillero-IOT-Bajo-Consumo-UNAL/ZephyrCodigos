#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sem.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define SERVER_PORT 12345
#define MESSAGE "Hello from Client!"
#define SLEEP_TIME_MS 1000
#define BUFFER_SIZE 128

#define THREAD_STACK_SIZE 1024
#define THREAD_PRIORITY 5

static int sock;
static struct sockaddr_in6 server_addr;

K_SEM_DEFINE(send_sem, 1, 1);
K_SEM_DEFINE(recv_sem, 1, 1);

K_THREAD_STACK_DEFINE(send_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(receive_stack, THREAD_STACK_SIZE);
struct k_thread send_thread_data;
struct k_thread receive_thread_data;

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

static int initialize_socket(struct sockaddr_in6 *server_addr)
{
    int s;
    int flags;

    memset(server_addr, 0, sizeof(struct sockaddr_in6));
    server_addr->sin6_family = AF_INET6;
    server_addr->sin6_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR, 
                  &server_addr->sin6_addr) != 1) {
        printk("Invalid IPv6 address format\n");
        return -1;
    }

    s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        printk("Failed to create socket: %d\n", errno);
        return -1;
    }

    // Hacer el socket non-blocking
    flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) {
        printk("Failed to get socket flags: %d\n", errno);
        close(s);
        return -1;
    }
    
    if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
        printk("Failed to set non-blocking: %d\n", errno);
        close(s);
        return -1;
    }

    printk("Socket created successfully (fd=%d, non-blocking)\n", s);
    return s;
}

void send_message(void *arg1, void *arg2, void *arg3)
{
    ssize_t sent;

    while (1) {
        if (k_sem_take(&send_sem, K_NO_WAIT) != 0) {
            k_sleep(K_MSEC(10));
            continue;
        }

        sent = sendto(sock, MESSAGE, strlen(MESSAGE), 0,
                      (struct sockaddr *)&server_addr, 
                      sizeof(struct sockaddr_in6));
        
        if (sent < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printk("Send failed: errno=%d (%s)\n", errno, strerror(errno));
            }
        } else {
            printk("Sent %zd bytes: %s\n", sent, MESSAGE);
        }

        k_sem_give(&send_sem);
        k_sleep(K_MSEC(SLEEP_TIME_MS));
    }
}

void receive_message(void *arg1, void *arg2, void *arg3)
{
    char recv_buffer[BUFFER_SIZE];
    ssize_t received;
    struct sockaddr_in6 from_addr;
    socklen_t from_addr_len;

    while (1) {
        if (k_sem_take(&recv_sem, K_NO_WAIT) != 0) {
            k_sleep(K_MSEC(10));
            continue;
        }

        memset(recv_buffer, 0, BUFFER_SIZE);
        from_addr_len = sizeof(from_addr);
        
        received = recvfrom(sock, recv_buffer, BUFFER_SIZE - 1, 0,
                           (struct sockaddr *)&from_addr, &from_addr_len);
        
        if (received < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printk("Recv failed: errno=%d (%s)\n", errno, strerror(errno));
            }
            // EAGAIN/EWOULDBLOCK es normal, significa "no hay datos"
        } else if (received > 0) {
            recv_buffer[received] = '\0';
            printk("Received %zd bytes: %s\n", received, recv_buffer);
        }

        k_sem_give(&recv_sem);
        k_sleep(K_MSEC(50));  // Poll cada 50ms
    }
}

void main(void)
{
    printk("Socket client with semaphores example\n");

    struct net_if *iface = net_if_get_default();
    if (!iface) {
        printk("No default net interface\n");
        return;
    }

    net_if_up(iface);
    k_sleep(K_MSEC(1500));
    
    set_tx_power(iface, 20);

    sock = initialize_socket(&server_addr);
    if (sock < 0) {
        return;
    }

    k_thread_create(&send_thread_data, send_stack, THREAD_STACK_SIZE,
                    send_message, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&receive_thread_data, receive_stack, THREAD_STACK_SIZE,
                    receive_message, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER);
    }
}