#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pwm.h>
#define LCD_ADDR 0x3F  // Cambia según tu módulo (0x27 o 0x3F suelen ser comunes)
#define LCD_BACKLIGHT 0x08
#define ENABLE_BIT 0x04
#define READ_WRITE 0x00  // Siempre 0 para escritura
#define REGISTER_SELECT_COMMAND 0x00
#define REGISTER_SELECT_DATA 0x01



const struct device *i2c;

void lcd_strobe(uint8_t data)
{
    uint8_t buf = data | ENABLE_BIT;
    i2c_write(i2c, &buf, 1, LCD_ADDR);
    k_busy_wait(1); // ~1 microsegundo
    buf = data & ~ENABLE_BIT;
    i2c_write(i2c, &buf, 1, LCD_ADDR);
    k_busy_wait(50); // >37us para comando
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
    k_sleep(K_MSEC(50)); // Espera power on

    // Secuencia de inicialización según datasheet
    lcd_write4bits(0x30 | LCD_BACKLIGHT);
    k_sleep(K_MSEC(5));
    lcd_write4bits(0x30 | LCD_BACKLIGHT);
    k_busy_wait(150);
    lcd_write4bits(0x30 | LCD_BACKLIGHT);
    k_sleep(K_MSEC(5));
    lcd_write4bits(0x20 | LCD_BACKLIGHT); // Modo 4 bits
    k_sleep(K_MSEC(5));

    // Configuraciones del LCD
    lcd_command(0x28); // 4 bits, 2 líneas, 5x8 puntos
    lcd_command(0x08); // Display apagado
    lcd_command(0x01); // Limpia pantalla
    k_sleep(K_MSEC(2));
    lcd_command(0x06); // Entrada modo incremento, sin desplazamiento
    lcd_command(0x0C); // Display encendido, cursor apagado
}

void lcd_print(const char *str)
{
    while (*str) {
        lcd_write_char(*str++);
    }
}

void main(void)
{
    i2c = device_get_binding("i2c");

    if (!i2c || !device_is_ready(i2c)) {
        printk("I2C device not found or not ready\n");
        return;
    }

    printk("Inicializando LCD...\n");
    lcd_init();

    printk("Escribiendo en LCD...\n");
    lcd_print("Alooooo!!!!");

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}