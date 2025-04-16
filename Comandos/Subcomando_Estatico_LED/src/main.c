#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>

/*
Subcomandos Estaticos
*/

// Definimos el led que vamos a usar
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);


// Definimos las funciones que van a ejecutar los comandos
static int PrenderFuncion(const struct shell *shell, size_t argc, char **argv)
{
    gpio_pin_set_dt(&led, 1);
    return 0;
}

static int ApagarFuncion(const struct shell *shell, size_t argc, char **argv)
{
    gpio_pin_set_dt(&led, 0);
    return 0;
}

/* Creamos la estructura de los subcomandos */
SHELL_STATIC_SUBCMD_SET_CREATE(definicionComando,
        SHELL_CMD(Prender, NULL, "Comando para prender LED.", PrenderFuncion),
        SHELL_CMD(Apagar,   NULL, "Comando para apagar LED.", ApagarFuncion),
        SHELL_SUBCMD_SET_END
);


// Inicializamos nuestro led
int main(){
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    return 0;
}


/* 
Registramos el comando con 
(NombreComando,&definicionComando,Descripcion del comando, Funcion Propia)
*/
SHELL_CMD_REGISTER(ControlLed, &definicionComando, "Comandos para controlar un LED", NULL);