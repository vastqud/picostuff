#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>

void read_status(spi_inst_t *spi, const uint cs, uint8_t *buf) {
    gpio_put(cs, 0); //high to low transition
    spi_read_blocking(spi, 0, buf, 1); //read
    gpio_put(cs, 1); //back to high
}

void init_board_led() {
    uint LED_PIN = 25;

    gpio_init(LED_PIN); //init board led
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
}

int main() {
    uint csn = 17, ce = 22, mosi = 19, sck = 18, miso = 16;
    spi_inst_t *spi = spi0;
    uint8_t data_buffer[1];

    stdio_init_all();
    init_board_led();

    //init cs high
    gpio_init(csn); 
    gpio_set_dir(csn, GPIO_OUT);
    gpio_put(csn, 1);

    //init ce low
    gpio_init(ce); 
    gpio_set_dir(ce, GPIO_OUT);
    gpio_put(ce, 0);

    //init SPI port at 1 MHz
    spi_init(spi, 1000 * 1000);

    spi_set_format(spi0,   //spi port 0
                    8,      //8 bits per transfer
                    1,      
                    1,      
                    SPI_MSB_FIRST);

    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_set_function(miso, GPIO_FUNC_SPI);

    sleep_ms(101);

    while (1) {
        read_status(spi, csn, data_buffer);
        int i = 0;

        for (i = 0; i <= 0; i++) {
            printf("%d", data_buffer[i]);
        }
        printf("\n");

        sleep_ms(1000);
    }
}

/*
PWR_UP in CONFIG (00) to 1 -> standby 1 (1.5ms)
CE pin low -> back to standby 1
CE pin high -> RX or TX mode (from standby)

status register default: 00011100 (28 / 1C)
*/