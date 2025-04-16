#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>

// Definimos el led que vamos a usar
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

// Definimos que se va a ejecutar cuando el usuario mande el comando
static int cmd_led_on(const struct shell *shell, size_t argc, char **argv)
{
    // Conmutamos el led de la beagle cuando se ejecute el comando
    gpio_pin_toggle_dt(&led);
    return 0;
}

// Inicializamos nuestro led
int main(){
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    return 0;
}

// Registramos el comando con (NombreComando,NULL,Descripcion del comando, Funcion)
SHELL_CMD_REGISTER(prenderLed, NULL, "Enciende un LED", cmd_led_on);
