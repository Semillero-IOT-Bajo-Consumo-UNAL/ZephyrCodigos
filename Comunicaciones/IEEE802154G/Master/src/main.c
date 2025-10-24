#include <zephyr/kernel.h>
#include <fcntl.h>
#include "../libs/IEEEG.h"

void messageEvent(ssize_t received,char message[]){
    printf("\nMensaje Recibido:  ");
    for (size_t i = 0; i < received; i++)
    {
        printf("%c",message[i]);
    }
    printf("\n");
}

int main(void)
{
    k_sleep(K_MSEC(500)); // Zephyr necesita un poquito de tiempo para arrancar
    
    IEEEG_Start();    
    IEEEG_StartComunications("2001:db8::2",2304); // Pedir address y puerto
    IEEEG_bindOnMessageEvent(messageEvent);
    IEEEG_sendMessage("Hola mundo","2001:db8::1");

    
    
    
    while (1) {
        k_sleep(K_FOREVER);
    }
    return 0;
}