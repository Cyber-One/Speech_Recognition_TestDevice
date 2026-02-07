#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Minimal test - just LED and serial
int main() {
    // Initialize LED
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Blink immediately before USB init to prove code is running
    for (int i = 0; i < 5; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    // Initialize USB serial
    stdio_init_all();
    sleep_ms(3000);  // Wait for USB enumeration
    
    // Blink again after USB init
    for (int i = 0; i < 5; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    printf("\n\n=== MINIMAL TEST FIRMWARE ===\n");
    printf("If you see this, USB serial is working!\n");
    printf("LED should be blinking continuously...\n\n");
    
    // Continuous blink and print
    int count = 0;
    while (true) {
        gpio_put(LED_PIN, 1);
        printf("LED ON  - Count: %d\n", count);
        sleep_ms(500);
        
        gpio_put(LED_PIN, 0);
        printf("LED OFF - Count: %d\n", count);
        sleep_ms(500);
        
        count++;
    }
    
    return 0;
}
