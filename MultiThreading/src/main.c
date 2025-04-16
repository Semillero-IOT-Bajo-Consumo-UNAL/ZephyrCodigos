// Importamos librerias de multithreading
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

// Importamos configuraciones inciales del segundo thread
#define STACK_SIZE 512
#define PRIORITY 5

// Definimos que va a ejecutar nuestro segundo hilo
void thread_func(void *arg1, void *arg2, void *arg3) {
    while (1) {
        printk("Hola desde el hilo secundario!\n");
        k_msleep(1000);
    }
}

// Cojemos las configuraciones que ya teniamos y se las metemos al thread
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
struct k_thread thread_data;

// Iniciamos la ejecucion de nuestro programa
void main(void) {
    k_thread_create(&thread_data, thread_stack, STACK_SIZE,
                    thread_func,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);
     while (1) {
        printk("Hola desde el hilo principal!\n");
        k_msleep(1000);
    }
}