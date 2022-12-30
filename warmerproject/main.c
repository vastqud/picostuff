#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <stdio.h>

static const uint8_t RF_SETUP = 0x06;//set RF power to 0dBm and 1Mbps
static const uint8_t RX_ADDR_P0 = 0x0A;//data pipe 0 address
static const uint8_t TX_ADDR = 0x10;//tx address (should be same as rx pipe 0 address)
static const uint8_t FEATURE = 0x1D;//disables dynamic payload length;
static const uint8_t DYNPD = 0x1C;//disables dynamic payload length for all pipes;
static const uint8_t CONFIG = 0x00;//masks interrupts, enters power up mode, and tx mode
static const uint8_t FIFO_STATUS = 0x17;
static const uint8_t RF_CHNL = 0x05;//set to channel 122
static const uint8_t EN_RXADDR = 0x02; //enable only one data pipe
static const uint8_t SETUP_RETR = 0x04; //disable auto retransmit
static const uint8_t STATUS = 0x07;
static const uint8_t OBSERVE_TX = 0x08;

static const uint8_t WRITE_COMMAND_WORD = 0x20; //0b00100000
static const uint8_t READ_COMMAND_WORD = 0x00;

static const uint8_t PAYLOAD_SIZE = 1; //bytes

void init_board_led(uint fifo_led) {
    uint LED_PIN = 25;

    gpio_init(LED_PIN); //init board led
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    gpio_init(fifo_led); //init board led
    gpio_set_dir(fifo_led, GPIO_OUT);
    gpio_put(fifo_led, 1);
}

void write_to_register(spi_inst_t *spi, const uint cs, const uint8_t reg, uint8_t data[], int mbytes) {
    mbytes++;
    data[0] = 0x20 | reg; //first byte (write command)

    gpio_put(cs, 0);
    spi_write_blocking(spi, data, mbytes);
    gpio_put(cs, 1);
}

void load_payload_for_transmit(spi_inst_t *spi, const uint cs, uint8_t data) {
    uint8_t command = 0xA0;

    gpio_put(cs, 0);
    spi_write_blocking(spi, &command, 1); //send command byte
    spi_write_blocking(spi, &data, PAYLOAD_SIZE); //write data
    gpio_put(cs, 1);
}

void flush_tx_fifo(spi_inst_t *spi, const uint cs) {
    uint8_t command = 0xE1;

    gpio_put(cs, 0);
    spi_write_blocking(spi, &command, 1);
    gpio_put(cs, 1);
}

void read_register(spi_inst_t *spi, const uint cs, const uint8_t reg, uint8_t *buf, int mbytes) {
    uint8_t read_command = 0x00 | reg;

    gpio_put(cs, 0);
    spi_write_blocking(spi, &read_command, 1); //send read command byte
    spi_read_blocking(spi, 0, buf, mbytes); //read n mbytes from spi to buf array memory
    gpio_put(cs, 1);
}

void set_defaults(spi_inst_t *spi, const uint cs) {
    uint8_t rf_data[2] = {0, 0x06};
    uint8_t feature_data[2] = {0, 0x00};
    uint8_t dynpd_data[2] = {0, 0x00};
    uint8_t rf_chnl_data[2] = {0, 0x7A};
    uint8_t en_pipe_data[2] = {0, 0x01};
    uint8_t retr_data[2] = {0, 0xFF};

    uint8_t addr_p0_data[6] = {0, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2};
    uint8_t tx_addr_data[6] = {0, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2};

    write_to_register(spi, cs, RF_SETUP, rf_data, 1); 
    write_to_register(spi, cs, FEATURE, feature_data, 1);
    write_to_register(spi, cs, DYNPD, dynpd_data, 1);
    write_to_register(spi, cs, RF_CHNL, rf_chnl_data, 1);
    write_to_register(spi, cs, EN_RXADDR, en_pipe_data, 1);
    write_to_register(spi, cs, SETUP_RETR, retr_data, 1);

    write_to_register(spi, cs, RX_ADDR_P0, addr_p0_data, 5);
    write_to_register(spi, cs, TX_ADDR, tx_addr_data, 5);
}

void set_tx(spi_inst_t *spi, const uint cs, const uint ce) {
    uint8_t config_data[2] = {0, 0x0A};

    write_to_register(spi, cs, CONFIG, config_data, 1); //configure CONFIG register
    gpio_put(ce, 1); //set CE high
}

void irq_call() {
    printf("interrupt triggerred");
}

int main() {
    uint csn = 17, ce = 22, mosi = 19, sck = 18, miso = 16, tx_fifo_led = 15, irq = 0;
    spi_inst_t *spi = spi0;
    uint8_t buffer[5];
    int counter = 0;

    stdio_init_all();
    init_board_led(tx_fifo_led);

    //init cs high
    gpio_init(csn); 
    gpio_set_dir(csn, GPIO_OUT);
    gpio_put(csn, 1);

    gpio_init(irq);
    gpio_set_dir(irq, GPIO_IN);
    //gpio_set_irq_enabled_with_callback(irq, GPIO_IRQ_LEVEL_LOW, 1, irq_call);

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
    set_tx(spi, csn, ce);

    while (1) {
        counter++;

        if (counter == 15) {
            uint8_t data = 0b11011001;
            load_payload_for_transmit(spi, csn, data); //wil set MAX_RT high after 60ms
        }
        if (counter == 20) {
            flush_tx_fifo(spi, csn); //flush the fifo
            uint8_t new_status[2] = {0, 0x1E};
            write_to_register(spi, csn, STATUS, new_status, 1); //clear MAX_RT of STATUS reg
        }
        if (counter == 25) {
            uint8_t data = 0b11011111;
            load_payload_for_transmit(spi, csn, data); //transmit another packet, setting MAX_RT high again after 60ms
        }

        sleep_ms(10);
        read_register(spi, csn, OBSERVE_TX, buffer, 1);
        uint8_t status = buffer[0];

        sleep_ms(10); //have to wait between reads. wait less than 60 ms. 60ms is how long it takes for radio to try to retransmit with our current settings (15 times at 4000 microseconds each)
                      //wait any longer than that and the irq will be out of sync with the retries because itll pick it up one iteration sooner
        read_register(spi, csn, STATUS, buffer, 1);
        uint8_t status2 = buffer[0];

        int num_rt_this_packet = (status & 0x0F);
        int total_fails = (status & 0xF0) >> 4;
        int rt_irq_enabled = (status2 & 0x10) >> 4; //read MAX_RT bit
        
        if (rt_irq_enabled == 1) {
            gpio_put(tx_fifo_led, 0); //max retries reached, turn off (MAX_RT irq is set, cant transmit)
        } else if (rt_irq_enabled == 0) {
            gpio_put(tx_fifo_led, 1); //ready to transmit (MAX_RT irq is reset)
        }
        
        printf("_total_fails: %d  num_rt: %d  rt_irq_enabled: %d   counter: %d\n", total_fails, num_rt_this_packet, rt_irq_enabled, counter);
        sleep_ms(550);
    }
}

/*
cp warmerproject.uf2 /h/
*/