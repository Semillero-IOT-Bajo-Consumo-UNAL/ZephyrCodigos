#include <zephyr/kernel.h>

/* Definimos la constante SLEEP_TIME_MS como 1000 representando 1000 milisegundos  */
#define SLEEP_TIME_MS   1000

int main(void)
{
    // Imprimimos un mensaje
	printk("ejemplo: Nuestro programa ha iniciado xd \n");
    // Imprimimos un mensaje, esperamos 1 segundo y repetimos el codigo
	while (1) {
		printk("ejemplo: esto se deberia ejecutar cada segundo \n");
		k_msleep(SLEEP_TIME_MS);
	}
	// Retornamos 0 (el numero que pasemos indica codigos de error)
	// al ser 0 estamos diciendo que el programa finalizo correctamente
	return 0;
}