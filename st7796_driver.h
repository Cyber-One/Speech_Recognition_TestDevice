#ifndef ST7796_DRIVER_H
#define ST7796_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// Display dimensions
#define LCD_WIDTH  320
#define LCD_HEIGHT 480

// Color definitions (RGB565)
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_GRAY    0x8410
#define COLOR_DARKGRAY 0x4208
#define COLOR_ORANGE  0xFD20

// Function prototypes
void st7796_init(void);
void st7796_fill_screen(uint16_t color);
void st7796_draw_pixel(int16_t x, int16_t y, uint16_t color);
void st7796_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void st7796_draw_char(int16_t x, int16_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size);
void st7796_draw_string(int16_t x, int16_t y, const char* str, uint16_t color, uint16_t bg_color, uint8_t size);
void st7796_draw_vbar(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t value, uint16_t max_value, uint16_t color);
void st7796_set_rotation(uint8_t rotation);
void st7796_test_pattern(void);  // Test pattern for debugging

#endif // ST7796_DRIVER_H
