#include <zephyr/shell/shell.h>

/*
Subcomandos Estaticos
*/

// Definimos las funciones que van a ejecutar los comandos
static int PrenderFuncion(const struct shell *shell, size_t argc, char **argv)
{
    printk("Aqui deberia prender el LED\n");
    return 0;
}

static int ApagarFuncion(const struct shell *shell, size_t argc, char **argv)
{
    printk("Aqui deberia apagar el LED\n");
    return 0;
}

/* Creamos la estructura de los subcomandos */
SHELL_STATIC_SUBCMD_SET_CREATE(definicionComando,
        SHELL_CMD(Prender, NULL, "Comando para prender LED.", PrenderFuncion),
        SHELL_CMD(Apagar,   NULL, "Comando para apagar LED.", ApagarFuncion),
        SHELL_SUBCMD_SET_END
);


/* 
Registramos el comando con 
(NombreComando,&definicionComando,Descripcion del comando, Funcion Propia)
*/
SHELL_CMD_REGISTER(ControlLed, &definicionComando, "Comandos para controlar un LED", NULL);