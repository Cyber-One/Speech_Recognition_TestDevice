#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/resets.h"
#include "pico/multicore.h"
#include "st7796_driver.h"

// I2C Configuration - Using I2C0 on GPIO 20 (SDA) and GPIO 21 (SCL)
// Alternative: If GPIO 20/21 don't work, try I2C1 on GPIO 26/27
#define I2C_PORT i2c0
#define I2C_SDA 20  // I2C0 SDA (alternative pins)
#define I2C_SCL 21  // I2C0 SCL (alternative pins)
#define I2C_BASE_ADDR 0x60
#define I2C_BAUDRATE 400000

// Debug output control
#define DEBUG_VERBOSE 0  // Set to 1 for detailed output, 0 for minimal

// Button pins for address selection
#define BTN_ADDR_UP 14    // Increment I2C address
#define BTN_ADDR_DOWN 15  // Decrement I2C address

// Touch screen I2C (reserved for future use)
#define TOUCH_I2C_SDA 8
#define TOUCH_I2C_SCL 9
#define TOUCH_RST_PIN 10
#define TOUCH_INT_PIN 11

// Packet format from master
#define PACKET_HEADER 0xAA
#define PACKET_SIZE 41  // 1 header + 20 bins × 2 bytes
#define NUM_FREQ_BINS 20

// Spectrogram buffer: 100 time samples × 20 frequency bins
// Using circular buffer with head pointer for O(1) insertion
#define SPECTROGRAM_DEPTH 100
uint16_t spectrogram_buffer[SPECTROGRAM_DEPTH][NUM_FREQ_BINS] = {0};
volatile int spectrogram_head = 0;  // Points to newest data row (circular index)

// Receive buffer
uint8_t rx_buffer[PACKET_SIZE];
volatile int rx_index = 0;
volatile bool packet_ready = false;

// Current I2C address (changeable via buttons)
volatile uint8_t current_i2c_address = I2C_BASE_ADDR;
volatile bool address_changed = false;

// Frequency bin data for display
uint16_t freq_bins[NUM_FREQ_BINS] = {0};
volatile bool display_update_needed = false;

// Core synchronization flag for safe address changes
volatile bool core1_paused = false;

// Performance monitoring
volatile uint32_t packet_count = 0;
uint32_t last_packet_count = 0;
uint32_t last_perf_check_ms = 0;

// Button debouncing
uint32_t last_btn_up_time = 0;
uint32_t last_btn_down_time = 0;
#define DEBOUNCE_MS 200

// Button interrupt handlers
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    if (gpio == BTN_ADDR_UP && (now - last_btn_up_time) > DEBOUNCE_MS) {
        last_btn_up_time = now;
        if (current_i2c_address < 0x67) {
            current_i2c_address++;
            address_changed = true;
        }
    } else if (gpio == BTN_ADDR_DOWN && (now - last_btn_down_time) > DEBOUNCE_MS) {
        last_btn_down_time = now;
        if (current_i2c_address > I2C_BASE_ADDR) {
            current_i2c_address--;
            address_changed = true;
        }
    }
}

// Read address from GPIO pins
uint8_t read_address_selection(void) {
    return current_i2c_address;
}

// I2C IRQ handler for slave mode
void i2c1_irq_handler(void) {
    static uint32_t irq_count = 0;
    uint32_t status = i2c_get_hw(I2C_PORT)->intr_stat;
    
    // Debug: Print first few interrupts only (if verbose mode enabled)
    if (DEBUG_VERBOSE && irq_count < 5) {
        printf("I2C IRQ #%u: status=0x%08X\n", irq_count++, status);
    }
    
    // RX FIFO has data
    if (status & (1 << 2)) {  // IC_INTR_RX_FULL
        while (i2c_get_read_available(I2C_PORT) > 0) {
            uint8_t byte = i2c_get_hw(I2C_PORT)->data_cmd & 0xFF;
            
            if (rx_index < PACKET_SIZE) {
                rx_buffer[rx_index++] = byte;
                
                if (rx_index == PACKET_SIZE) {
                    packet_ready = true;
                    if (DEBUG_VERBOSE) printf("Packet complete! [%u bytes]\n", PACKET_SIZE);
                }
            }
        }
    }
    
    // STOP condition detected
    if (status & (1 << 9)) {  // IC_INTR_STOP_DET
        // Clear stop interrupt
        (void)i2c_get_hw(I2C_PORT)->clr_stop_det;
        
        // If incomplete packet, reset
        if (rx_index > 0 && rx_index < PACKET_SIZE) {
            rx_index = 0;
        }
    }
}

// Parse and display received packet
void process_packet(void) {
    // Verify header
    if (rx_buffer[0] != PACKET_HEADER) {
        printf("Invalid header: 0x%02X (expected 0x%02X)\n", rx_buffer[0], PACKET_HEADER);
        return;
    }
    
    // Extract frequency bins
    for (int i = 0; i < NUM_FREQ_BINS; i++) {
        freq_bins[i] = (rx_buffer[1 + i * 2] << 8) | rx_buffer[2 + i * 2];
    }
    
    // Circular buffer insert: NO data copying! Just update index and overwrite oldest
    // Insert new data at current head position
    for (int col = 0; col < NUM_FREQ_BINS; col++) {
        spectrogram_buffer[spectrogram_head][col] = freq_bins[col];
    }
    
    // Move head forward (circular wrap)
    spectrogram_head = (spectrogram_head + 1) % SPECTROGRAM_DEPTH;
    
    // Count packets for performance monitoring
    packet_count++;
    
    // Mark display for update
    display_update_needed = true;
    
    // Minimal debug output for performance (disabled by default)
    // printf(".");  // Uncomment to see packet reception rate
    // if (packet_count % 62 == 0) printf(" %u pkts\n", packet_count);  // Every 1 sec
}

// Map magnitude value to color (spectrogram gradient)
// Black → Blue → Cyan → Green → Yellow → Red
// RGB565: RRRRRGGGGGGBBBBB (5-bit R, 6-bit G, 5-bit B)
uint16_t magnitude_to_color(uint16_t value, uint16_t max_value) {
    if (value == 0 || max_value == 0) return COLOR_BLACK;  // 0x0000
    
    // Normalize to 0-255 range
    int intensity = (value * 255) / max_value;
    if (intensity > 255) intensity = 255;
    if (intensity < 0) intensity = 0;
    
    int r, g, b;
    
    // Color gradient mapping (0-255 intensity range)
    if (intensity < 51) {
        // Black → Blue (0-20%)
        r = 0;
        g = 0;
        b = (intensity * 255) / 51;  // 0 → 255
    } else if (intensity < 102) {
        // Blue → Cyan (20-40%)
        r = 0;
        g = ((intensity - 51) * 255) / 51;  // 0 → 255
        b = 255;
    } else if (intensity < 153) {
        // Cyan → Green (40-60%)
        r = 0;
        g = 255;
        b = 255 - ((intensity - 102) * 255) / 51;  // 255 → 0
        if (b < 0) b = 0;
    } else if (intensity < 204) {
        // Green → Yellow (60-80%)
        r = ((intensity - 153) * 255) / 51;  // 0 → 255
        g = 255;
        b = 0;
    } else {
        // Yellow → Red (80-100%)
        r = 255;
        g = 255 - ((intensity - 204) * 255) / 51;  // 255 → 0
        if (g < 0) g = 0;
        b = 0;
    }
    
    // Bounds check all components
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    
    // Convert RGB888 (8-bit each) to RGB565 format
    // RGB565: bit 15-11=R(5bits), bit 10-5=G(6bits), bit 4-0=B(5bits)
    uint16_t r5 = (r >> 3) & 0x1F;  // 8-bit to 5-bit
    uint16_t g6 = (g >> 2) & 0x3F;  // 8-bit to 6-bit
    uint16_t b5 = (b >> 3) & 0x1F;  // 8-bit to 5-bit
    
    return (r5 << 11) | (g6 << 5) | b5;
}

// Update TFT display with spectrogram
void update_display(void) {
    char buffer[32];
    
    // Display current I2C address at top
    snprintf(buffer, sizeof(buffer), "I2C: 0x%02X", current_i2c_address);
    st7796_draw_string(5, 5, buffer, COLOR_YELLOW, COLOR_BLACK, 2);
    
    // Performance monitoring: show packet rate every 500ms
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if ((now - last_perf_check_ms) >= 500) {
        uint32_t pkts_received = packet_count - last_packet_count;
        last_packet_count = packet_count;
        last_perf_check_ms = now;
        
        // Show packets per second (500ms = *2)
        snprintf(buffer, sizeof(buffer), "%u pkt/s", pkts_received * 2);
        st7796_draw_string(250, 5, buffer, COLOR_CYAN, COLOR_BLACK, 1);
    }
    
    // Capture current head position (volatile)
    int current_head = spectrogram_head;
    
    // Find max value in buffer for color scaling
    uint16_t max_value = 1;
    for (int row = 0; row < SPECTROGRAM_DEPTH; row++) {
        for (int col = 0; col < NUM_FREQ_BINS; col++) {
            if (spectrogram_buffer[row][col] > max_value) {
                max_value = spectrogram_buffer[row][col];
            }
        }
    }
    if (max_value < 32) max_value = 32;  // Minimum scaling
    
    // Draw spectrogram: 20 bins × 24px wide = 480px, 100 samples × 3px tall = 300px
    const int pixel_width = 24;   // Each frequency bin is 24 pixels wide
    const int pixel_height = 3;   // Each time sample is 3 pixels tall
    const int start_y = 30;        // Start below the I2C address text
    
    // Draw from newest (head-1) to oldest (head-100), wrapping circularly
    for (int display_row = 0; display_row < SPECTROGRAM_DEPTH; display_row++) {
        // Calculate circular buffer index: newest at top, oldest at bottom
        // head-1 is newest, head-2 is next, etc., wrapping at 0
        int buffer_idx = (current_head - 1 - display_row + SPECTROGRAM_DEPTH) % SPECTROGRAM_DEPTH;
        
        for (int col = 0; col < NUM_FREQ_BINS; col++) {
            uint16_t value = spectrogram_buffer[buffer_idx][col];
            uint16_t color = magnitude_to_color(value, max_value);
            
            int x = col * pixel_width;
            int y = start_y + display_row * pixel_height;
            
            // Draw a solid rectangle for this pixel
            st7796_fill_rect(x, y, pixel_width, pixel_height, color);
        }
    }
    
    display_update_needed = false;
}

// Core 1: Continuous display rendering loop
// This runs independently and continuously updates the display
// without blocking I2C reception on Core 0
void core1_display_loop(void) {
    if (DEBUG_VERBOSE) printf("[Core 1] Display renderer started\n");
    
    uint32_t last_update_time = 0;
    
    // Continuous display updates
    while (1) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Check if Core 0 is requesting a pause (for address change)
        if (!core1_paused) {
            // Update display at ~30 FPS regardless of packet arrival
            // This ensures smooth scrolling even if packets arrive at different rates
            if ((now - last_update_time) >= 33) {  // ~30 Hz = 33ms per frame
                update_display();
                last_update_time = now;
            }
        }
        
        tight_loop_contents();
    }
}

// Reconfigure I2C slave with new address
void reconfigure_i2c_address(void) {
    // Pause Core 1 display rendering to prevent race condition
    core1_paused = true;
    sleep_ms(50);  // Wait for current display update to finish
    
    // Disable I2C interrupts temporarily
    irq_set_enabled(I2C0_IRQ, false);
    
    // Disable I2C to configure
    i2c_hw_t *hw = i2c_get_hw(I2C_PORT);
    hw->enable = 0;
    while (hw->enable_status & 1);
    
    // Clear any incomplete packet in RX buffer
    // If we were in the middle of receiving on the old address, reset the index
    rx_index = 0;
    packet_ready = false;
    
    // Clear the receive buffer to avoid stale data
    memset(rx_buffer, 0, PACKET_SIZE);
    
    // Set new slave address
    hw->sar = current_i2c_address;
    
    // Clear I2C FIFO of any pending data
    while (i2c_get_read_available(I2C_PORT) > 0) {
        (void)i2c_get_hw(I2C_PORT)->data_cmd;
    }
    
    // Re-enable I2C
    hw->enable = 1;
    
    // Re-enable interrupts
    irq_set_enabled(I2C0_IRQ, true);
    
    printf("I2C address changed to: 0x%02X\n", current_i2c_address);
    
    // Update display with new address BEFORE resuming Core 1
    // (prevents SPI bus conflict with Core 1's display rendering)
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "I2C: 0x%02X", current_i2c_address);
    st7796_draw_string(5, 5, buffer, COLOR_YELLOW, COLOR_BLACK, 2);
    
    // Resume Core 1 display rendering AFTER display update completes
    core1_paused = false;
    
    address_changed = false;
}

int main() {
    // Initialize USB serial
    stdio_init_all();
    sleep_ms(3000);  // Wait longer for USB enumeration
    
    printf("\n=== I2C Slave Test Device with TFT Display ===\n");
    printf("Firmware starting...\n");
    printf("Pico SDK Version: %s\n", PICO_SDK_VERSION_STRING);
    
    // Blink onboard LED to show we're alive
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    printf("Blinking LED to show startup...\n");
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(200);
    }
    
    // Initialize TFT display
    printf("\n--- Display Initialization ---\n");
    printf("Initializing ST7796 display...\n");
    printf("SPI Pins: SCLK=2, MOSI=3, MISO=4, CS=5, DC=6, RST=7\n");
    
    st7796_init();
    printf("Display initialized\n");
    
    st7796_set_rotation(1); // Landscape mode (480x320)
    printf("Rotation set to landscape\n");
    
    st7796_fill_screen(COLOR_BLACK);
    printf("Screen cleared to black\n");
    
    // Display startup message
    printf("Drawing startup text...\n");
    st7796_draw_string(10, 10, "I2C FFT Display", COLOR_CYAN, COLOR_BLACK, 3);
    st7796_draw_string(10, 40, "Initializing...", COLOR_WHITE, COLOR_BLACK, 2);
    sleep_ms(1000);
    
    printf("Clearing screen again...\n");
    st7796_fill_screen(COLOR_BLACK);
    printf("Display initialization complete!\n");
    
    // Show test pattern for 2 seconds
    printf("\n--- Display Test Pattern ---\n");
    st7796_test_pattern();
    printf("Test pattern displayed for 2 seconds...\n");
    sleep_ms(2000);
    st7796_fill_screen(COLOR_BLACK);
    printf("Test pattern cleared\n");
    
    // Initialize button pins for address selection
    printf("\n--- Button Initialization ---\n");
    printf("Setting up buttons on GPIO %d (UP) and %d (DOWN)\n", BTN_ADDR_UP, BTN_ADDR_DOWN);
    gpio_init(BTN_ADDR_UP);
    gpio_init(BTN_ADDR_DOWN);
    gpio_set_dir(BTN_ADDR_UP, GPIO_IN);
    gpio_set_dir(BTN_ADDR_DOWN, GPIO_IN);
    gpio_pull_up(BTN_ADDR_UP);
    gpio_pull_up(BTN_ADDR_DOWN);
    printf("Buttons configured with pull-ups\n");
    
    // Set up button interrupts (falling edge = button press)
    gpio_set_irq_enabled_with_callback(BTN_ADDR_UP, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN_ADDR_DOWN, GPIO_IRQ_EDGE_FALL, true);
    printf("Button interrupts enabled\n");
    
    // Set initial I2C address
    current_i2c_address = I2C_BASE_ADDR;
    printf("\n--- I2C Slave Configuration ---\n");
    printf("Initial I2C Address: 0x%02X\n", current_i2c_address);
    
    // Initialize I2C pins
    printf("Setting up I2C1 on GPIO %d (SDA) and %d (SCL)\n", I2C_SDA, I2C_SCL);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Complete I2C peripheral reset using the reset controller
    printf("Performing hard reset of I2C0 peripheral...\n");
    reset_block(1u << RESET_I2C0);  // Assert reset on I2C0
    unreset_block(1u << RESET_I2C0); // Release reset
    sleep_us(100);
    printf("I2C0 hardware reset complete\n");
    
    // Now configure I2C0 for SLAVE-ONLY mode
    printf("Configuring I2C0 for SLAVE-ONLY mode\n");
    
    i2c_hw_t *hw = i2c_get_hw(I2C_PORT);
    
    // Disable I2C while configuring
    hw->enable = 0;
    sleep_us(10);
    while (hw->enable_status & 1);
    printf("I2C0 disabled\n");
    
    // Configure IC_CON for slave-only mode
    // Bit 0: MASTER_MODE = 0 (slave mode)
    // Bit 6: IC_SLAVE_DISABLE = 0 (slave enabled)
    uint32_t con = hw->con;
    con &= ~(1 << 0);  // Clear MASTER_MODE
    con &= ~(1 << 6);  // Clear IC_SLAVE_DISABLE  
    hw->con = con;
    printf("IC_CON set to 0x%04X (slave mode)\n", hw->con);
    
    // Set slave address
    hw->sar = current_i2c_address;
    printf("Slave address set to 0x%02X\n", current_i2c_address);
    
    // I2C timing for 400 kHz
    hw->ss_scl_hcnt = 36;
    hw->ss_scl_lcnt = 36;
    printf("I2C timing configured\n");
    
    // RX FIFO trigger
    hw->rx_tl = 0;
    printf("RX FIFO threshold set\n");
    
    // Enable slave-mode interrupts
    hw->intr_mask = (1 << 2) | (1 << 9);  // RX_FULL | STOP_DET
    printf("Slave interrupts enabled\n");
    
    // Enable I2C
    hw->enable = 1;
    sleep_us(10);
    while (!(hw->enable_status & 1));
    printf("I2C0 enabled in SLAVE-ONLY mode\n");
    
    // Check bus idle state
    bool scl_idle = gpio_get(I2C_SCL);
    bool sda_idle = gpio_get(I2C_SDA);
    printf("I2C Bus Status: SCL=%s, SDA=%s (both should be HIGH when idle)\n",
           scl_idle ? "HIGH" : "LOW", sda_idle ? "HIGH" : "LOW");
    
    if (!scl_idle || !sda_idle) {
        printf("WARNING: I2C bus not idle! Check for bus conflict\n");
    }
    
    // Set up I2C IRQ handler
    irq_set_exclusive_handler(I2C0_IRQ, i2c1_irq_handler);
    irq_set_enabled(I2C0_IRQ, true);
    printf("I2C IRQ handler registered\n");
    
    printf("\n--- System Ready ---\n");
    printf("I2C slave ready on pins SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);
    printf("Listening on address 0x%02X\n", current_i2c_address);
    printf("Waiting for packets (41 bytes: 0xAA + 20 bins)...\n");
    printf("Use buttons on GPIO %d (up) and %d (down) to change address\n\n", BTN_ADDR_UP, BTN_ADDR_DOWN);
    
    // Display initial I2C address on screen
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "I2C: 0x%02X", current_i2c_address);
    st7796_draw_string(5, 5, buffer, COLOR_YELLOW, COLOR_BLACK, 2);
    
    // Draw legend
    st7796_draw_string(5, 40, "Frequency Spectrum", COLOR_WHITE, COLOR_BLACK, 2);
    st7796_draw_string(5, 440, "500-5000 Hz (20 bins)", COLOR_GRAY, COLOR_BLACK, 1);
    
    // Launch Core 1 for display rendering
    if (DEBUG_VERBOSE) printf("\n[Core 0] Launching Core 1 for display rendering...\n");
    multicore_launch_core1(core1_display_loop);
    sleep_ms(100);  // Give Core 1 time to start
    if (DEBUG_VERBOSE) printf("[Core 0] Core 1 launched, I2C reception ready\n\n");
    
    // Main loop - Core 0 handles I2C reception only
    uint32_t loop_count = 0;
    uint32_t last_heartbeat = 0;
    
    while (1) {
        loop_count++;
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Heartbeat every 5 seconds to confirm main loop is running
        if ((now - last_heartbeat) >= 5000) {
            if (DEBUG_VERBOSE) printf("[Core 0 Heartbeat] %u packets received, loop_count=%u\n", packet_count, loop_count);
            last_heartbeat = now;
        }
        
        // Handle I2C address change
        if (address_changed) {
            reconfigure_i2c_address();
        }
        
        // Handle received packet
        if (packet_ready) {
            // Process received packet
            if (DEBUG_VERBOSE) printf("Packet #%u received and queued for display\n", packet_count + 1);
            process_packet();
            
            // Reset for next packet
            packet_ready = false;
            rx_index = 0;
        }
        
        // Core 0 is now free to handle only I2C reception
        // Display rendering is handled by Core 1 continuously
        tight_loop_contents();
    }
    
    return 0;
}
