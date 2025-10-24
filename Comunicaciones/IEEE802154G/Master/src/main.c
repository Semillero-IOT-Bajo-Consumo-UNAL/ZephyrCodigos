#include <zephyr/kernel.h>
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
    IEEEG_StartComunications(2304); 
    IEEEG_bindOnMessageEvent(messageEvent);

    
    
    
    while (1) {
        IEEEG_sendMessage("holaaa","2001:db8::1");

        k_sleep(K_MSEC(1000));
    }
    return 0;
}