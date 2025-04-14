#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_MS 500
#define LED_NODE DT_ALIAS(salidadigital)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

#define BTN_NODE DT_ALIAS(miboton)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BTN_NODE, gpios);

int main(void)
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_DOWN);

    
    while (1) {
        int val = gpio_pin_get_dt(&button);
        if (val < 0) {
            printk("Error leyendo el pin\n");
        } else if (val == 0) {
            gpio_pin_set_dt(&led, 1);
        } else {
            gpio_pin_set_dt(&led, 0);
        }
    }
	return 0;
}