#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>

#define LCD_ADDR 0x3F  // Cambia según tu módulo
#define LCD_BACKLIGHT 0x08
#define ENABLE_BIT 0x04
#define REGISTER_SELECT_COMMAND 0x00
#define REGISTER_SELECT_DATA 0x01

const struct device *i2c;
const struct device *pwm_dev;

void lcd_strobe(uint8_t data)
{
    uint8_t buf = data | ENABLE_BIT;
    i2c_write(i2c, &buf, 1, LCD_ADDR);
    k_busy_wait(1);
    buf = data & ~ENABLE_BIT;
    i2c_write(i2c, &buf, 1, LCD_ADDR);
    k_busy_wait(50);
}

void lcd_write4bits(uint8_t data)
{
    i2c_write(i2c, &data, 1, LCD_ADDR);
    lcd_strobe(data);
}

void lcd_send(uint8_t value, uint8_t mode)
{
    uint8_t high_nibble = (value & 0xF0) | mode | LCD_BACKLIGHT;
    uint8_t low_nibble = ((value << 4) & 0xF0) | mode | LCD_BACKLIGHT;

    lcd_write4bits(high_nibble);
    lcd_write4bits(low_nibble);
}

void lcd_command(uint8_t cmd)
{
    lcd_send(cmd, REGISTER_SELECT_COMMAND);
}

void lcd_write_char(char c)
{
    lcd_send(c, REGISTER_SELECT_DATA);
}

void lcd_init(void)
{
    k_sleep(K_MSEC(50));

    lcd_write4bits(0x30 | LCD_BACKLIGHT);
    k_sleep(K_MSEC(5));
    lcd_write4bits(0x30 | LCD_BACKLIGHT);
    k_busy_wait(150);
    lcd_write4bits(0x30 | LCD_BACKLIGHT);
    k_sleep(K_MSEC(5));
    lcd_write4bits(0x20 | LCD_BACKLIGHT);
    k_sleep(K_MSEC(5));

    lcd_command(0x28);
    lcd_command(0x08);
    lcd_command(0x01);
    k_sleep(K_MSEC(2));
    lcd_command(0x06);
    lcd_command(0x0C);
}

void lcd_print(const char *str)
{
    while (*str) {
        lcd_write_char(*str++);
    }
}

void lcd_clear(void)
{
    lcd_command(0x01);
    k_sleep(K_MSEC(2));
}

// Nota: frecuencia en Hz, duración en ms, texto para LCD
void play_note(uint32_t freq, uint32_t duration_ms, const char *text)
{
    uint32_t period_cycles;
    uint32_t pulse_cycles;
    int ret;

    

    lcd_clear();
    lcd_print(text);

    if (freq == 0) {
        // Silencio
        pwm_set_cycles(pwm_dev, 0, 1000000, 0, 0);
        k_sleep(K_MSEC(duration_ms));
        return;
    }

    uint64_t pwm_freq;

    if (pwm_get_cycles_per_sec(pwm_dev, 0, &pwm_freq) != 0) {
        printk("Error leyendo frecuencia PWM\n");
        return;
    }

    // PWM base a 1 MHz (ciclos por segundo)
    period_cycles = pwm_freq / freq;
    pulse_cycles = period_cycles / 2; // 50% duty cycle

    ret = pwm_set_cycles(pwm_dev, 0, period_cycles, pulse_cycles, 0);
    if (ret) {
        printk("Error configurando PWM: %d\n", ret);
    }

    k_sleep(K_MSEC(duration_ms));

    // Apagar PWM después de nota
    pwm_set_cycles(pwm_dev, 0, 1000000, 0, 0);
    k_sleep(K_MSEC(50)); // Pequeña pausa
}

void main(void)
{
    i2c = device_get_binding("i2c");  // Cambia si tu i2c tiene otro label
    if (!i2c || !device_is_ready(i2c)) {
        printk("I2C device not found or not ready\n");
        return;
    }

    pwm_dev = device_get_binding("chicharron");  // Cambia según tu label PWM
    if (!pwm_dev) {
        printk("PWM device not found\n");
        return;
    }

    printk("Inicializando LCD...\n");
    lcd_init();

    // Canción simple: Do - Re - Mi - Silencio
    struct {
        uint32_t freq;
        uint32_t duration;
        const char *text;
    } song[] = {

{ 196.0, 236, "Cuan" },
{ 261.6, 263, "Cuando" },

{ 311.1, 37, "Cuando" },
{ 329.6, 331, "Cuando llegan" },

{ 329.6, 331, "" },
{ 0, 10, "" },
{ 329.6, 232, "Las" },
{ 0, 10, "" },
{ 329.6, 232, "Las ho" },
{ 0, 10, "" },
{ 329.6, 232, "Las horas" },
{ 0, 10, "" },
{ 329.6, 232, "Las horas de"},
{ 0, 10, "" },
{ 329.6, 232, "Las horas de la" },
{ 0, 10, "" },
{ 329.6, 331, "Tar" },
{ 0, 10, "" },
{ 329.6, 331, "Tarde" },
{ 0, 10, "" },
{ 392, 232, "Tarde, que" },
{ 0, 10, "" },
{ 293, 232, "Tarde, que me" },
{ 0, 10, "" },
{ 349, 232, "encuen" },
{ 0, 10, "" },
{ 293, 232, "encuetro" }, 
{ 0, 10, "" },
{ 220, 232, "encuetro tan" },
{ 0, 10, "" },
{ 261, 232, "so" },
{ 0, 10, "" },
{ 329.6, 232, "solo" },
{ 0, 10, "" },
{ 164.6, 232, "solo y muy" },
{ 0, 10, "" },
{ 293.6, 232, "solo y muy le" },

{ 0, 10, "" },
{ 349, 232, "solo y muy lejos" },
{ 0, 10, "" },
{ 440, 232, "de" },
{ 0, 10, "" },
{ 329.63 , 232, "de ti" },
{ 0, 10, "" },
{ 392 , 232, "de ti :c" },
{ 0, 10, "" },
{ 196 , 232, "de ti :c" },
{ 0, 1000, "de ti :c" },


    };

    while (1) {
        for (int i = 0; i < sizeof(song) / sizeof(song[0]); i++) {
            play_note(song[i].freq, song[i].duration, song[i].text);
        }
    }
}
