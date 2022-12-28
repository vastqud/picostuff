#include "pico/stdlib.h"

int main() {
    uint OUT_PIN = 22;
    uint POWER_LED = 15;
    uint LED_PIN = 25;

    gpio_init(OUT_PIN);
    gpio_init(POWER_LED);
    gpio_init(LED_PIN);

    gpio_set_dir(OUT_PIN, GPIO_OUT);
    gpio_set_dir(POWER_LED, GPIO_OUT);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_put(POWER_LED, 1);
    gpio_put(LED_PIN, 1);

    while (1) {
        gpio_put(OUT_PIN, 0);
        sleep_ms(10000);
        gpio_put(OUT_PIN, 1);
        sleep_ms(10000);
    }
}