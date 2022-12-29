#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>

static const uint8_t RF_SETUP = 0x06;//0b00110;
static const uint8_t RX_ADDR_P0 = 0x0A;//0b01010;
static const uint8_t RX_ADDR_P1 = 0x0C;//0b01100;
static const uint8_t TX_ADDR = 0x10;//0b10000;
static const uint8_t FEATURE = 0x1D;//0b11101;
static const uint8_t DYNPD = 0x1C;//0b11100;
static const uint8_t CONFIG = 0x00;//0b00000;
static const uint8_t FIFO_STATUS = 0x17;//0b10111;

static const uint8_t WRITE_COMMAND_WORD = 0x20; //0b00100000
static const uint8_t READ_COMMAND_WORD = 0x00;

void read_status(spi_inst_t *spi, const uint cs, uint8_t *buf) {
    gpio_put(cs, 0); //high to low transition
    spi_read_blocking(spi, 0, buf, 1); //read & write the data to buffer
    gpio_put(cs, 1); //back to high
}

void init_board_led() {
    uint LED_PIN = 25;

    gpio_init(LED_PIN); //init board led
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
}

void write_register_command(spi_inst_t *spi, const uint cs, uint8_t addr, uint8_t data) {
    uint8_t msg[2];

    msg[0] = WRITE_COMMAND_WORD | addr; //set command word
    msg[1] = data;

    gpio_put(cs, 0);
    spi_write_blocking(spi, msg, 2); //send the write command word + data bits
    gpio_put(cs, 1);
}

void set_rx(spi_inst_t *spi, const uint cs, const uint ce) {
    write_register_command(spi, cs, CONFIG, 0x7B);
    gpio_put(ce, 1);
}

void set_defaults(spi_inst_t *spi, const uint cs) { //configures registers for a RX device
    write_register_command(spi, cs, RF_SETUP, 0x06); //set RF_SETUP register
    write_register_command(spi, cs, FEATURE, 0x06); //set FEATURE register
    write_register_command(spi, cs, DYNPD, 0x03); //set DYNPD data
}

void read_register(spi_inst_t *spi, const uint cs, uint8_t addr, uint8_t *buf, const uint8_t nbytes) {
    uint8_t msg = READ_COMMAND_WORD | addr;

    gpio_put(cs, 0); //high to low transition
    spi_write_blocking(spi, &msg, 1); //send read command word (1 byte)
    spi_read_blocking(spi, 0, buf, nbytes); //read & write the data to buffer
    gpio_put(cs, 1); //back to high
}

int main() {
    uint csn = 17, ce = 22, mosi = 19, sck = 18, miso = 16;
    spi_inst_t *spi = spi0;
    uint8_t data_buffer[5];

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

    sleep_ms(103);

    //set_rx(spi, csn, ce);
    write_register_command(spi, csn, CONFIG, 0x7B);
    write_register_command(spi, csn, FEATURE, 0x06);
    //write_register_command(spi, csn, DYNPD, 0x03);
    //set_defaults(spi, csn);

    while (1) {
        read_register(spi, csn, CONFIG, data_buffer, 1);
        int i = 0;

        for (i = 0; i <= 4; i++) {
            printf("%d ", data_buffer[i]);
        }
        printf("\n");

        sleep_ms(1000);
    }
}

/*
read works kind of. when it works, it prints the target byte as the second byte in the buffer instead of the first.
also, it only appears to print the most recently written byte to any register.

0x06         RF_SETUP   0x06 00110
0xF2F2F2F2F2 RX_ADDR_P0 0x0A 01010
0xE3E3E3E3E3 RX_ADDR_P1 0x0C 01100
0xF2F2F2F2F2 TX_ADDR    0x10 10000
0x06         FEATURE    0x1D 11101
0x03	     DYNPD      0x1C 11100

for TX device:
PWR_UP set high, PRIM_RX set low, payload in TX FIFO, high pulse on CE
0x7A CONFIG 0x00 00000

for RX device:
PWR_UP set high, PRIM_RX set high, CE set high
0x7B CONFIG 0x00 00000

test RX device:
read FIFO_STATUS 0x17 10111 register
should see: 00010001 / 0x11 / 17

10001011
cp warmerproject.uf2 /h/
12 0 12 0 12
1100 0000 1100 0000 1100

00001110

00000000
00000110

00010000
*/