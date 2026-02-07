#include "st7796_driver.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

// SPI and control pins
#define SPI_PORT spi0
#define PIN_SCLK 2
#define PIN_MOSI 3
#define PIN_MISO 4
#define PIN_CS   5
#define PIN_DC   6
#define PIN_RST  7

// ST7796 commands
#define ST7796_NOP        0x00
#define ST7796_SWRESET    0x01
#define ST7796_SLPIN      0x10
#define ST7796_SLPOUT     0x11
#define ST7796_INVOFF     0x20
#define ST7796_INVON      0x21
#define ST7796_DISPOFF    0x28
#define ST7796_DISPON     0x29
#define ST7796_CASET      0x2A
#define ST7796_RASET      0x2B
#define ST7796_RAMWR      0x2C
#define ST7796_MADCTL     0x36
#define ST7796_COLMOD     0x3A
#define ST7796_CSCON      0xF0

// Display dimensions
static uint16_t _width = LCD_WIDTH;
static uint16_t _height = LCD_HEIGHT;
static uint8_t _rotation = 0;

// Simple 5x7 font bitmap
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

// Hardware control functions
static inline void cs_select() {
    gpio_put(PIN_CS, 0);
}

static inline void cs_deselect() {
    gpio_put(PIN_CS, 1);
}

static inline void dc_command() {
    gpio_put(PIN_DC, 0);
}

static inline void dc_data() {
    gpio_put(PIN_DC, 1);
}

static inline void rst_high() {
    gpio_put(PIN_RST, 1);
}

static inline void rst_low() {
    gpio_put(PIN_RST, 0);
}

// Send command to display
static void st7796_write_command(uint8_t cmd) {
    dc_command();
    cs_select();
    spi_write_blocking(SPI_PORT, &cmd, 1);
    cs_deselect();
}

// Send data to display
static void st7796_write_data(uint8_t data) {
    dc_data();
    cs_select();
    spi_write_blocking(SPI_PORT, &data, 1);
    cs_deselect();
}

// Send data buffer to display
static void st7796_write_data_buf(const uint8_t* buf, size_t len) {
    dc_data();
    cs_select();
    spi_write_blocking(SPI_PORT, buf, len);
    cs_deselect();
}

// Set address window
static void st7796_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    st7796_write_command(ST7796_CASET);
    uint8_t data[4];
    data[0] = x0 >> 8;
    data[1] = x0 & 0xFF;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xFF;
    st7796_write_data_buf(data, 4);

    st7796_write_command(ST7796_RASET);
    data[0] = y0 >> 8;
    data[1] = y0 & 0xFF;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xFF;
    st7796_write_data_buf(data, 4);

    st7796_write_command(ST7796_RAMWR);
}

// Initialize display
void st7796_init(void) {
    printf("ST7796: Initializing SPI...\n");
    // Initialize SPI
    spi_init(SPI_PORT, 32 * 1000 * 1000); // 32 MHz
    gpio_set_function(PIN_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    printf("ST7796: SPI initialized at 32 MHz\n");

    // Initialize control pins
    printf("ST7796: Initializing control pins (CS=%d, DC=%d, RST=%d)\n", PIN_CS, PIN_DC, PIN_RST);
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);

    cs_deselect();
    rst_high();
    sleep_ms(10);
    printf("ST7796: Control pins configured\n");

    // Hardware reset
    printf("ST7796: Performing hardware reset...\n");
    rst_low();
    sleep_ms(20);
    rst_high();
    sleep_ms(120);
    printf("ST7796: Hardware reset complete\n");

    // Software reset
    printf("ST7796: Sending software reset command...\n");
    st7796_write_command(ST7796_SWRESET);
    sleep_ms(150);

    // Sleep out
    printf("ST7796: Waking display from sleep...\n");
    st7796_write_command(ST7796_SLPOUT);
    sleep_ms(120);

    // Interface Pixel Format: 16-bit color
    printf("ST7796: Setting 16-bit color mode...\n");
    st7796_write_command(ST7796_COLMOD);
    st7796_write_data(0x55); // 16-bit/pixel

    // Memory Access Control
    printf("ST7796: Configuring memory access...\n");
    st7796_write_command(ST7796_MADCTL);
    st7796_write_data(0x48); // Row/column address order

    // Display Inversion Off
    printf("ST7796: Configuring inversion...\n");
    st7796_write_command(ST7796_INVOFF);

    // Display ON
    printf("ST7796: Turning display ON...\n");
    st7796_write_command(ST7796_DISPON);
    sleep_ms(10);
    printf("ST7796: Display initialization complete!\n");
}

// Set display rotation
void st7796_set_rotation(uint8_t rotation) {
    _rotation = rotation % 4;
    st7796_write_command(ST7796_MADCTL);
    
    switch (_rotation) {
        case 0: // Portrait
            st7796_write_data(0x48);
            _width = LCD_WIDTH;
            _height = LCD_HEIGHT;
            break;
        case 1: // Landscape
            st7796_write_data(0x28);
            _width = LCD_HEIGHT;
            _height = LCD_WIDTH;
            break;
        case 2: // Portrait inverted
            st7796_write_data(0x88);
            _width = LCD_WIDTH;
            _height = LCD_HEIGHT;
            break;
        case 3: // Landscape inverted
            st7796_write_data(0xE8);
            _width = LCD_HEIGHT;
            _height = LCD_WIDTH;
            break;
    }
}

// Fill entire screen with color
void st7796_fill_screen(uint16_t color) {
    st7796_fill_rect(0, 0, _width, _height, color);
}

// Draw a single pixel
void st7796_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;
    
    st7796_set_addr_window(x, y, x, y);
    uint8_t data[2] = {color >> 8, color & 0xFF};
    st7796_write_data_buf(data, 2);
}

// Fill a rectangle
void st7796_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x < 0 || y < 0 || x >= _width || y >= _height) return;
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;
    
    st7796_set_addr_window(x, y, x + w - 1, y + h - 1);
    
    uint8_t data[2] = {color >> 8, color & 0xFF};
    dc_data();
    cs_select();
    
    for (int32_t i = 0; i < w * h; i++) {
        spi_write_blocking(SPI_PORT, data, 2);
    }
    
    cs_deselect();
}

// Draw a character
void st7796_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size) {
    if (c < 32 || c > 90) c = 32; // Space for invalid chars
    
    int char_idx = c - 32;
    
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = font5x7[char_idx][i];
        for (uint8_t j = 0; j < 8; j++) {
            if (line & 0x01) {
                st7796_fill_rect(x + i * size, y + j * size, size, size, color);
            } else if (bg_color != color) {
                st7796_fill_rect(x + i * size, y + j * size, size, size, bg_color);
            }
            line >>= 1;
        }
    }
}

// Draw a string
void st7796_draw_string(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg_color, uint8_t size) {
    int16_t cursor_x = x;
    while (*str) {
        st7796_draw_char(cursor_x, y, *str, color, bg_color, size);
        cursor_x += 6 * size;
        str++;
    }
}

// Draw a vertical bar graph
void st7796_draw_vbar(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t value, uint16_t max_value, uint16_t color) {
    // Calculate bar height based on value
    int16_t bar_height = (value * height) / max_value;
    if (bar_height > height) bar_height = height;
    
    // Draw background (empty portion)
    st7796_fill_rect(x, y, width, height - bar_height, COLOR_DARKGRAY);
    
    // Draw filled portion
    st7796_fill_rect(x, y + height - bar_height, width, bar_height, color);
    
    // Draw border
    // Top
    st7796_fill_rect(x, y, width, 1, COLOR_WHITE);
    // Bottom
    st7796_fill_rect(x, y + height - 1, width, 1, COLOR_WHITE);
    // Left
    st7796_fill_rect(x, y, 1, height, COLOR_WHITE);
    // Right
    st7796_fill_rect(x + width - 1, y, 1, height, COLOR_WHITE);
}

// Test pattern for debugging display
void st7796_test_pattern(void) {
    printf("Drawing test pattern...\n");
    
    // Draw colored rectangles
    st7796_fill_rect(0, 0, _width/3, _height/3, COLOR_RED);
    st7796_fill_rect(_width/3, 0, _width/3, _height/3, COLOR_GREEN);
    st7796_fill_rect(2*_width/3, 0, _width/3, _height/3, COLOR_BLUE);
    
    st7796_fill_rect(0, _height/3, _width/3, _height/3, COLOR_YELLOW);
    st7796_fill_rect(_width/3, _height/3, _width/3, _height/3, COLOR_CYAN);
    st7796_fill_rect(2*_width/3, _height/3, _width/3, _height/3, COLOR_MAGENTA);
    
    st7796_fill_rect(0, 2*_height/3, _width/3, _height/3, COLOR_WHITE);
    st7796_fill_rect(_width/3, 2*_height/3, _width/3, _height/3, COLOR_GRAY);
    st7796_fill_rect(2*_width/3, 2*_height/3, _width/3, _height/3, COLOR_BLACK);
    
    printf("Test pattern complete\n");
}
