#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sem.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#define SERVER_PORT 12345        // Server port
#define MESSAGE "Hello from Client!"
#define SLEEP_TIME_MS 1000       // Time between sending messages (in milliseconds)
#define BUFFER_SIZE 128

#define THREAD_STACK_SIZE 1024   // Stack size for each thread
#define THREAD_PRIORITY 5        // Priority for threads



// Shared resources
static int sock;
static struct sockaddr_in6 server_addr;

// Semaphores
K_SEM_DEFINE(socket_sem, 1, 1); // Semaphore for socket access

// Thread stacks and thread objects
K_THREAD_STACK_DEFINE(send_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(receive_stack, THREAD_STACK_SIZE);
struct k_thread send_thread_data;
struct k_thread receive_thread_data;


#define PA_PIN     29
#define SUBG_PIN   30
static const struct gpio_dt_spec pa_gpio = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(antenna_mux0), gpios, {0});
void enable_pa(const struct device *gpio)
{
    /* PA = DIO29 = 1, SUBG = DIO30 = 0 -> 20 dBm TX según tu tabla */
    gpio_pin_set(gpio, PA_PIN, 1);
    gpio_pin_set(gpio, SUBG_PIN, 0);
}

void disable_pa(const struct device *gpio)
{
    /* Volver al estado "off" */
    gpio_pin_set(gpio, PA_PIN, 0);
    gpio_pin_set(gpio, SUBG_PIN, 0);
}


static int initialize_socket(struct sockaddr_in6 *server_addr)
{
    int s;

    // Configure the server address
    memset(server_addr, 0, sizeof(struct sockaddr_in6));
    server_addr->sin6_family = AF_INET6;
    server_addr->sin6_port = htons(SERVER_PORT);

    /* CONFIG_NET_CONFIG_PEER_IPV6_ADDR se expande a cadena si está en prj.conf */
    if (inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR, &server_addr->sin6_addr) != 1) {
        printk("Invalid IPv6 address format (CONFIG_NET_CONFIG_PEER_IPV6_ADDR)\n");
        return -1;
    }

    // Create a socket
    s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        printk("Failed to create socket: %d\n", errno);
        return -1;
    }
    printk("Socket created successfully (fd=%d)\n", s);

    return s;
}

void send_message(void *arg1, void *arg2, void *arg3)
{
    ssize_t sent;

    while (1) {
        // Take semaphore to gain exclusive access to the socket
        k_sem_take(&socket_sem, K_FOREVER);

        sent = sendto(sock, MESSAGE, strlen(MESSAGE), 0,
                      (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in6));
        if (sent < 0) {
            printk("Failed to send message: ret=%zd errno=%d (%s)\n",
                   sent, errno, strerror(errno));
        } else {
            printk("Message sent successfully (ret=%zd): %s\n", sent, MESSAGE);
        }

        // Release the semaphore
        k_sem_give(&socket_sem);

        k_sleep(K_MSEC(SLEEP_TIME_MS)); // Sleep before sending the next message
    }
}

void receive_message(void *arg1, void *arg2, void *arg3)
{
    char recv_buffer[BUFFER_SIZE];
    ssize_t received;
    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        // Take semaphore to gain exclusive access to the socket
        k_sem_take(&socket_sem, K_FOREVER);

        memset(recv_buffer, 0, BUFFER_SIZE);
        received = recvfrom(sock, recv_buffer, BUFFER_SIZE - 1, 0,
                            (struct sockaddr *)&client_addr, &client_addr_len);
        if (received < 0) {
            printk("Failed to receive data: ret=%zd errno=%d (%s)\n",
                   received, errno, strerror(errno));
        } else {
            // Null-terminate the received data for printing
            recv_buffer[received] = '\0';
            printk("Received %zd bytes from client: %s\n", received, recv_buffer);
        }

        // Release the semaphore
        k_sem_give(&socket_sem);

        k_sleep(K_MSEC(SLEEP_TIME_MS)); // Sleep before checking for the next message
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

    /* Si tienes CONFIG_NET_CONFIG_SETTINGS y no AUTO_INIT, subir la interfaz ayuda */
    net_if_up(iface);

    /* espera corta para que la pila procese el 'if up' y net_config asigne dir. */
    k_sleep(K_MSEC(1500));
    //try_set_tx_power_netmgmt(20);

    if (!device_is_ready(pa_gpio.port)) {
    printk("PA GPIO device not ready\n");
    return;
    }

    gpio_pin_configure_dt(&pa_gpio, GPIO_OUTPUT_INACTIVE);
    gpio_pin_set_dt(&pa_gpio, 1);

    // Initialize socket
    sock = initialize_socket(&server_addr);
    if (sock < 0) {
        return;
    }

    // Create the sending thread
    k_thread_create(&send_thread_data, send_stack, THREAD_STACK_SIZE,
                    (k_thread_entry_t)send_message, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    // Create the receiving thread
    k_thread_create(&receive_thread_data, receive_stack, THREAD_STACK_SIZE,
                    (k_thread_entry_t)receive_message, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER); // Keep the main thread idle
    }

    /* unreachable */
    // close(sock);
}
