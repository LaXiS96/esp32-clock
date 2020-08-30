#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_log.h"

// 128x32 0.91" OLED
// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

#define OLED_I2C_NUM I2C_NUM_0
#define OLED_I2C_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_BUF_SIZE (OLED_WIDTH * OLED_HEIGHT / 8) // 8 pixel per byte

static const char *OLED_TAG = "OLED";

static uint8_t fb[OLED_BUF_SIZE];

static const uint8_t font_two[] = {
    0x00, 0x00, 0xE0, 0xF0, 0x70, 0x70, 0x38, 0x38, 0x38, 0x38, 0x38, 0x78, 0xF8, 0xF0, 0xE0, 0xE0,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0xE0, 0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0,
    0xF8, 0x7C, 0x3E, 0x1F, 0x0F, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x3C, 0x3E,
    0x3F, 0x3F, 0x3B, 0x39, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x00, 0x00};

esp_err_t oled_write(uint8_t *data, size_t len, bool isCommand)
{
    esp_err_t ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    // Slave address
    i2c_master_write_byte(cmd, OLED_I2C_ADDR << 1, true);
    // Control byte
    i2c_master_write_byte(cmd, isCommand ? 0x00 : 0x40, true);

    i2c_master_write(cmd, data, len, true);

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(OLED_I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t oled_init(void)
{
    i2c_config_t conf;

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = GPIO_NUM_21;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = GPIO_NUM_22;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config(OLED_I2C_NUM, &conf);

    i2c_driver_install(OLED_I2C_NUM, conf.mode, 0, 0, 0);

    static uint8_t cmds[] = {
        0xAE,     // Display OFF
        0xA8, 31, // Multiplex ratio = (number of lines - 1) = 31
        // 0xD3, 0, // Display vertical offset = 0 (RESET)
        0x40,       // Display start line = (0x40 + 0) = first line (0) (RESET)
        0xA1,       // Segment remap = column 127 mapped to SEG0
        0xC8,       // COM output scan direction = remapped
        0xDA, 0x02, // COM pins hardware configuration = sequential, no remap
        0x20, 0x01, // Memory addressing mode = vertical
        0x81, 0x7F, // Contrast
        // 0xA4, // Disable entire display ON (RESET)
        // 0xA6, // Disable inverted display (RESET)
        // 0xD5, 0x80, // Clock divide ratio = (RESET), oscillator frequency = (RESET)
        // 0xD9, 0xF1, // Precharge period: phase 1 = 15, phase 2 = 1
        // 0xDB, 0x40, // VCOMH deselect level = ??? value not referenced in datasheet!
        0x8D, 0x14, // Enable charge pump
        0xAF,       // Display ON
    };
    oled_write(cmds, sizeof(cmds), true);

    ESP_LOGI(OLED_TAG, "initialization completed");

    return ESP_OK;
}

void oled_test(void)
{
    for (int i = 0; i < sizeof(font_two); i++)
    {
        // fb[OLED_WIDTH * i / 8] = font_zero[i];
        fb[i] = font_two[i];
    }
}

void oled_update(void)
{
    static uint8_t cmds[] = {
        0x21, 0, OLED_WIDTH - 1,      // Set column addresses: start = 0, end = 127
        0x22, 0, OLED_HEIGHT / 8 - 1, // Set page addresses: start = 0, end = 3
    };
    oled_write(cmds, sizeof(cmds), true);

    oled_write(fb, sizeof(fb), false);

    // TODO translate horizontal bytes to vertical bytes (8x8bit block at a time)
    // uint8_t tfb[OLED_BUF_SIZE];
    // for (int x = 0; x < OLED_BUF_SIZE; x++)
    // {
    //     uint8_t vb[8]; // vertical bytes

    //     // for each horizontal byte
    //     for (int ih = 0; ih < 8; ih++)
    //     {
    //         uint8_t hb = fb[ih * (OLED_WIDTH / 8)];

    //         // for each vertical byte
    //         for (int iv = 0; iv < 8; iv++)
    //         {
    //             vb[iv] = (hb & (1 << (7 - iv))) >> (7 - iv);
    //         }
    //     }
    // }

    // oled_write(tfb, sizeof(tfb), false);
}
