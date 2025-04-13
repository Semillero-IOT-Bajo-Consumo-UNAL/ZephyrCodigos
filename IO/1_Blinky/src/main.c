#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_MS 500
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void)
{
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(SLEEP_MS);
    }
	return 0;
}
