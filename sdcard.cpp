#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include "pico/stdlib.h"
#include "hardware/spi.h"

//https://nodeloop.org/guides/sd-card-spi-init-guide/

const int CMD_TIMEOUT = 1000;
const uint8_t FF_TOKEN = 0xFF;
const char R1_IDLE_STATE = 1;
const char R1_ILLEGAL_COMMAND = 1<<2;

class SDCard
{
private:
    int FFClock(int clocks=1);
    uint8_t response[16];
    void err(char* errMessage);
public:
    spi_inst_t *spi;
    int cs;
    SDCard(/* args */);
    void sdCardInit();
    int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBits=0, bool release = true, bool skip1 = true);
    void v1Init();
    void v2Init();
};

SDCard::SDCard(/* args */)
{
}

void SDCard::sdCardInit(){
    spi_init(spi, 1000*400); // clock rate to 400khz for init  
    FFClock(10);

    bool present = false;
    for (size_t i = 0; i < 5; i++)
    {
        if(cmd(0,0,0x95) == 0x01){
            present=true;
            break;
        }
    }
    if(!present){
        err("No sd card is present.");
    }
    //card is present

    int cmd8res = cmd(8, 0x1AA, 0x87, 4);
    if(cmd8res == R1_IDLE_STATE) v1Init(); //v2 card present
    else if(cmd8res == R1_ILLEGAL_COMMAND) v2Init();
    else err("Bad response from SD, unable to determine version.");

    spi_init(spi,1000*1000);
}

void SDCard::v2Init(){
    if((response[3] & 0xF0) != 0x10) err("v2 Voltage out of range, Unusable card.");
    
    cmd(58, 0, 0xfd);
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & ((1 << 20) | (1 << 21)))) err("v2 OCR Voltage out of range, Unusable card.");

    int timeout = CMD_TIMEOUT;
    uint8_t r1;
    do
    {
        cmd(55,0,0x65, 0, false);  
        r1=cmd(41,0x40000000, 0, false);
    } while (r1 != 0x00, timeout--);
    if (timeout==0) err("Failed to initialise v2 card, timed out while waiting.");
    gpio_put(cs,1);
}

void SDCard::v1Init(){
    if (cmd(58, 0, 0xfd));
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & ((1 << 20) | (1 << 21)))) err("v2 OCR Voltage out of range, Unusable card.");

}

int SDCard::cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes, bool release, bool skip1){
    gpio_put(cs,0);
    FFClock();
    uint8_t buf[6] = {
        (uint8_t) (cmd | 0x40), //  make sure the first two bits are 01, 0 is the start bit and 1 indicating this device is host
        (uint8_t) (args >> 24),
        (uint8_t) (args >> 16),
        (uint8_t) (args >> 8),
        (uint8_t) (args),
        (uint8_t) (crc)
    };

    spi_write_blocking(spi, buf, 6);

    if(skip1){
        FFClock();
    }

    uint8_t r1;
    memset(response, 0x00, 16);
    for (size_t i = 0; i < CMD_TIMEOUT; i++)
    {
        spi_read_blocking(spi, 0xFF, &r1, 1);
        if(!(r1&0x80)){ // checks the most significant bit of r1 is 1, which would indicate an error
            for (size_t j = 0; j < extraResponseBytes; j++)
            {
                spi_read_blocking(spi, 0xFF, &response[j], 1);
            }
            
            if(release){
                gpio_put(cs,1);
                FFClock();
            }
            return r1;
        }
    }
    
    //timeout
    gpio_put(cs,1);
    FFClock();
    return -1;
}

int SDCard::FFClock(int clocks){
    for (size_t i = 0; i < clocks; i++)
    {
        spi_write_blocking(spi, &FF_TOKEN, 1);
    }
    return 1;
}

void SDCard::err(char* errMessage){
    gpio_put(cs,1);
    FFClock();
    throw std::runtime_error(errMessage);
}