#include <zephyr/shell/shell.h>

/*
COMANDO DE NIVEL 1
*/

// Definimos que se va a ejecutar cuando el usuario mande el comando
static int cmd_led_on(const struct shell *shell, size_t argc, char **argv)
{
    printk("¡Encendiendo LED!\n");
    return 0;
}

// Registramos el comando con (NombreComando,NULL,Descripcion del comando, Funcion)
SHELL_CMD_REGISTER(prenderLed, NULL, "Enciende un LED", cmd_led_on);

