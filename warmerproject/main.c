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
static const uint8_t RF_CHNL = 0x05;

static const uint8_t WRITE_COMMAND_WORD = 0x20; //0b00100000
static const uint8_t READ_COMMAND_WORD = 0x00;

void write_to_register(spi_inst_t *spi, const uint cs, const uint8_t reg, uint8_t data[], int mbytes) {
    mbytes++;
    data[0] = 0x20 | reg; //first byte (write command)

    gpio_put(cs, 0);
    spi_write_blocking(spi, data, mbytes);
    gpio_put(cs, 1);
}

void read_register(spi_inst_t *spi, const uint cs, const uint8_t reg, uint8_t *buf, int mbytes) {
    uint8_t read_command = 0x00 | reg;

    gpio_put(cs, 0);
    spi_write_blocking(spi, &read_command, 1); //send read command
    spi_read_blocking(spi, 0, buf, mbytes); //read one byte from spi to buf array
    gpio_put(cs, 1);
}

void init_board_led() {
    uint LED_PIN = 25;

    gpio_init(LED_PIN); //init board led
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);
}

void set_defaults(spi_inst_t *spi, const uint cs) {
    uint8_t rf_data[2] = {0, 0x06};
    uint8_t feature_data[2] = {0, 0x06};
    uint8_t dynpd_data[2] = {0, 0x03};
    uint8_t rf_chnl_data[2] = {0, 0x7A};

    uint8_t addr_p0_data[6] = {0, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2};
    uint8_t addr_p1_data[6] = {0, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3};
    uint8_t tx_addr_data[6] = {0, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2};

    write_to_register(spi, cs, RF_SETUP, rf_data, 1); 
    write_to_register(spi, cs, FEATURE, feature_data, 1);
    write_to_register(spi, cs, DYNPD, dynpd_data, 1);
    write_to_register(spi, cs, RF_CHNL, rf_chnl_data, 1);

    write_to_register(spi, cs, RX_ADDR_P0, addr_p0_data, 5);
    write_to_register(spi, cs, RX_ADDR_P1, addr_p1_data, 5);
    write_to_register(spi, cs, TX_ADDR, tx_addr_data, 5);
}

void set_rx(spi_inst_t *spi, const uint cs, const uint ce) {
    uint8_t config_data[2] = {0, 0x7B};

    write_to_register(spi, cs, CONFIG, config_data, 1); //configure CONFIG register
    gpio_put(ce, 1); //set CE high
}

int main() {
    uint csn = 17, ce = 22, mosi = 19, sck = 18, miso = 16;
    spi_inst_t *spi = spi0;
    uint8_t buffer[5];

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
                    0,      
                    0,      
                    SPI_MSB_FIRST);

    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_set_function(miso, GPIO_FUNC_SPI);

    sleep_ms(103);
    set_defaults(spi, csn);
    set_rx(spi, csn, ce);

    while (1) {
        read_register(spi, csn, RF_CHNL, buffer, 1);
        
        int i = 0;
        for (i = 0; i <= 4; i++) {
            printf("%d ", buffer[i]);
        }
        printf("\n");

        sleep_ms(1000);
    }
}

/*
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

cp warmerproject.uf2 /h/
*/