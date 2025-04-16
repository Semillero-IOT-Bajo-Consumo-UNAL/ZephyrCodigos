#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>

#define SENSOR_NODE DT_ALIAS(sensor1)  // Usa el alias, no NODELABEL

void main(void)
{
    const struct device *sensor = DEVICE_DT_GET(SENSOR_NODE);

    if (!device_is_ready(sensor)) {
        printk("Sensor no está listo\n");
        return;
    }

    struct sensor_value val;

    while (1) {
        if (sensor_sample_fetch(sensor) < 0) {
            printk("Error al pedir muestra\n");
            continue;
        }

        if (sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &val) < 0) {
            printk("Error al obtener lectura\n");
            continue;
        }

        printk("Humedad: %d.%06d lx\n", val.val1, val.val2);
        k_sleep(K_SECONDS(1));
    }
}
