#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>

struct Note {
    uint32_t frequency;  // Frecuencia en Hz (0 para silencio)
    uint32_t duration_ms; // Duración en milisegundos
};

void play_note(const struct device *pwm_dev, uint32_t channel, uint32_t pwm_freq, struct Note note)
{
    if (note.frequency == 0) {
        // Silencio
        pwm_set_cycles(pwm_dev, channel, 1000, 0, 0);
        k_sleep(K_MSEC(note.duration_ms));
        return;
    }

    uint32_t period = pwm_freq / note.frequency;
uint32_t pulse = (period * 3) / 4;
    pwm_set_cycles(pwm_dev, channel, period, pulse, 0);
    k_sleep(K_MSEC(note.duration_ms));
}

void play_song(const struct device *pwm_dev, uint32_t channel, const struct Note *song, size_t length)
{
    uint64_t pwm_freq;
    int ret = pwm_get_cycles_per_sec(pwm_dev, channel, &pwm_freq);
    if (ret) {
        printk("Error obteniendo frecuencia PWM: %d\n", ret);
        return;
    }

    for (size_t i = 0; i < length; i++) {
        play_note(pwm_dev, channel, pwm_freq, song[i]);
    }

    // Apagar PWM al final
    pwm_set_cycles(pwm_dev, channel, 1000, 0, 0);
}

void main(void)
{
    const struct device *pwm_dev = device_get_binding("chicharron");
    if (!pwm_dev) {
        printk("PWM device not found\n");
        return;
    }

    struct Note song[] = {
        {262, 500},  // Do  (C4) 500ms
        {294, 500},  // Re  (D4)
        {330, 500},  // Mi  (E4)
        {349, 500},  // Fa  (F4)
        {392, 500},  // Sol (G4)
        {440, 500},  // La  (A4)
        {494, 500},  // Si  (B4)
        {523, 500},  // Do5 (C5)
        {0,  500}    // Silencio 500ms
    };

    printk("Reproduciendo canción...\n");
    play_song(pwm_dev, 0, song, sizeof(song)/sizeof(song[0]));
    printk("ayp...\n");

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
