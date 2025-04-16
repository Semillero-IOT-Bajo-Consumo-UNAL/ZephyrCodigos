#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

// Definimos la configuracion del ADC - El channel indica cual de los varios ADC vamos a usar
// 9 es el ADC frontal con la marca AN
#define ADC_NODE        DT_NODELABEL(adc0)
#define ADC_CHANNEL     9
#define ADC_RESOLUTION  12
#define ADC_BUFFER_SIZE 1

// Obtenemos el objeto del ADC como tal
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
static int16_t sample_buffer[ADC_BUFFER_SIZE];

// Definimos la configuracion al objeto en la variable de configuracion
// con lo que definimos antes
static struct adc_channel_cfg channel_cfg = {
    .gain             = ADC_GAIN_1,
    .reference        = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id       = ADC_CHANNEL,
    .differential     = 0,
};

// Definimos como vamos a realizar el escaneo de datos
static struct adc_sequence sequence = {
    .channels    = BIT(ADC_CHANNEL),
    .buffer      = sample_buffer,
    .buffer_size = sizeof(sample_buffer),
    .resolution  = ADC_RESOLUTION,
};

void main(void)
{
    // Finalmente configuramos el ADC
    int ret = adc_channel_setup(adc_dev, &channel_cfg);
    while (1) {
        // Realizamos lecturas e imprimimos cada segundo
        ret = adc_read(adc_dev, &sequence);
        if (ret < 0) {
            printk("ADC read failed: %d\n", ret);
        } else {
            int32_t mv = sample_buffer[0];
            adc_raw_to_millivolts(adc_ref_internal(adc_dev), ADC_GAIN_1, ADC_RESOLUTION, &mv);
            printk("ADC raw: %d, approx mV: %d\n", sample_buffer[0], mv);
        }

        k_sleep(K_MSEC(1000));
    }
}
