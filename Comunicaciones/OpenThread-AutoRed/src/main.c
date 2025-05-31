#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/dataset.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(openthread_auto, LOG_LEVEL_DBG);

void main(void)
{
    const struct openthread_context *ot_context = openthread_get_default_context();
    otInstance *instance = ot_context->instance;

    otOperationalDataset dataset;
    memset(&dataset, 0, sizeof(dataset));

    // Activa el timestamp para el dataset
    dataset.mActiveTimestamp.mSeconds = 1;
    dataset.mComponents.mIsActiveTimestampPresent = true;

    // Clave maestra de 16 bytes
    const uint8_t master_key[16] = {
        0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB,
        0xCC, 0xDD, 0xEE, 0xFF
    };
    memcpy(dataset.mNetworkKey.m8, master_key, 16);
    dataset.mComponents.mIsNetworkKeyPresent = true;

    // Nombre de la red (hasta 16 caracteres)
    const char *network_name = "ZephyrNet";
    memcpy(dataset.mNetworkName.m8, network_name, strlen(network_name));
    dataset.mComponents.mIsNetworkNamePresent = true;

    // PAN ID
    dataset.mPanId = 0x1234;
    dataset.mComponents.mIsPanIdPresent = true;

    // Canal (por ejemplo, canal 15)
    dataset.mChannel = 15;
    dataset.mComponents.mIsChannelPresent = true;

    // Aplicar este dataset como activo
    otDatasetSetActive(instance, &dataset);

    // Levantar la interfaz IPv6 y activar Thread
    otIp6SetEnabled(instance, true);
    otThreadSetEnabled(instance, true);

    LOG_INF("Thread network initialized and started.");
}
