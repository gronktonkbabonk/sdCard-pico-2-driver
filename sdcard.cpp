#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

//https://nodeloop.org/guides/sd-card-spi-init-guide/

const char FF_TOKEN = 0xFF;

class SDCard
{
private:
    /* data */
public:
    spi_inst_t *spi;
    int cs;
    SDCard(/* args */);
    void sdCardInit();
    int cmd(uint8_t cmd, uint32_t args, uint8_t crc);
};

SDCard::SDCard(/* args */)
{
}

void SDCard::sdCardInit(){

}

int SDCard::cmd(uint8_t cmd, uint32_t args, uint8_t crc){
    gpio_put(cs,0);
    uint8_t buf[6] = {
        (uint8_t) (cmd | 0x40), //  make sure the first two bits are 01, 0 is the start bit and 1 indicating this device is host
        (uint8_t) (args >> 24),
        (uint8_t) (args >> 16),
        (uint8_t) (args >> 8),
        (uint8_t) (args),
        (uint8_t) (crc | 0x01) // crc is 7 bits and it has to end with 1 (end bit)
    };
    spi_write_blocking(spi, buf, 6);
    gpio_put(cs,1);
}