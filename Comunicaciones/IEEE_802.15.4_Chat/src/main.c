#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(ieee_demo, LOG_LEVEL_DBG);

static struct net_if_link_cb link_cb;

// Función para la recepción de datos
static void recv_cb(struct net_if *iface, struct net_pkt *pkt)
{
    uint8_t data[128] = {0};
    int len = net_pkt_read(pkt, data, sizeof(data) - 1);

    if (len > 0) {
        data[len] = '\0';
        LOG_INF("📥 Received: %s", data);
    }

    net_pkt_unref(pkt);
}

// Función para enviar paquetes usando el protocolo IEEE
static void send_packet(struct net_if *iface, const char *msg)
{
    size_t len = strlen(msg);

    struct net_pkt *pkt = net_pkt_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_NO_WAIT);
    if (!pkt) {
        LOG_ERR("❌ No se pudo alocar el paquete");
        return;
    }

    if (net_pkt_write(pkt, msg, len)) {
        LOG_ERR("❌ No se pudo escribir en el paquete");
        net_pkt_unref(pkt);
        return;
    }

    net_pkt_set_iface(pkt, iface);

    if (net_send_data(pkt) < 0) {
        LOG_ERR("❌ Error al enviar");
        net_pkt_unref(pkt);
    } else {
        LOG_INF("📤 Enviado: %s", msg);
    }
}

// Comando para enviar un mensaje cuando el usuario lo desee
static int cmd_send_msg(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(shell, "❌ El mensaje no fue proporcionado");
        return -EINVAL;
    }

    // Concatenamos todos los argumentos en un solo mensaje
    char msg[256] = {0};  // Asegúrate de que el tamaño sea suficiente para tu mensaje

    for (size_t i = 1; i < argc; i++) {
        if (i > 1) {
            strncat(msg, " ", sizeof(msg) - strlen(msg) - 1);  // Añadimos un espacio entre palabras
        }
        strncat(msg, argv[i], sizeof(msg) - strlen(msg) - 1);  // Añadimos cada palabra
    }

    struct net_if *iface = net_if_get_default();
    send_packet(iface, msg);  // Enviar el mensaje concatenado
    return 0;
}

SHELL_CMD_REGISTER(send_msg, NULL, "Envía un mensaje de red", cmd_send_msg);

void main(void)
{
    // Configuración de la interfaz de red
    struct net_if *iface = net_if_get_default();

    // Registramos la función que va a recibir los paquetes de red
    net_if_register_link_cb(&link_cb, recv_cb);

    // Verificamos que la interfaz de red esté funcionando
    if (!net_if_is_up(iface)) {
        net_if_up(iface);
        LOG_INF("✅ Interfaz activada");
    }

    // Shell ya está listo para recibir comandos
    LOG_INF("✅ Shell listo. Usa 'send_msg <mensaje>' para enviar un mensaje.");
}
