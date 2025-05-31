#include <openthread/instance.h>
#include <zephyr/net/openthread.h>

void main(void) {
    struct openthread_context *ot_context = openthread_get_default_context();
    openthread_start(ot_context);
}
