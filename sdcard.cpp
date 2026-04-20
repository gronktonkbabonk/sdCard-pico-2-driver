#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "sdcard.h"

//https://nodeloop.org/guides/sd-card-spi-init-guide/

const int CMD_TIMEOUT = 1000;
const uint8_t FF_TOKEN = 0xFF;
const char R1_IDLE_STATE = 1;
const char R1_ILLEGAL_COMMAND = 1<<2;

SDCard::SDCard(spi_inst_t *spiInp, int csInp){
    spi = spiInp;
    cs = csInp;
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs,1); 
    sdCardInit();
}

void SDCard::sdCardInit(){
    sleep_ms(1);
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
    if(cmd8res == R1_IDLE_STATE) v2Init(); //v2 card present
    else if(cmd8res == R1_ILLEGAL_COMMAND) v1Init();
    else err("Bad response from SD, unable to determine version.");

    cmd(59, 1, 0x67);
    spi_init(spi,1000*1000*25);
}

void SDCard::v2Init(){
    if((response[3] & 0xF0) != 0x10) err("v2 Voltage out of range, Unusable card.");
    
    
    int timeout = CMD_TIMEOUT;
    uint8_t r1;
    do
    {
        cmd(55,0,0x65, 0, response, false);  
        r1=cmd(41,0x40000000, 0, false);
    } while (r1 != 0x00 && timeout--);
    if (timeout==0) err("Failed to initialise v2 card, timed out while waiting.");
    
    cmd(58, 0, 0xfd);
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & 0x00FF8000)) err("v2 OCR Voltage out of range, Unusable card.");
    
    addrMult = (((ocr >> 30) & 1)? 1 : 512);
    gpio_put(cs,1);
    FFClock();
}

void SDCard::v1Init(){
    int timeout = CMD_TIMEOUT;
    uint8_t r1;
    do
    {
        cmd(55,0,0x65, 0, response, false);  
        r1=cmd(41,0, 0, false);
    } while ((r1 != 0x00) && timeout--);
    if (timeout==0) err("Failed to initialise v1 card, timed out while waiting.");
    
    cmd(58, 0, 0xfd);
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & ((1 << 20) | (1 << 21)))) err("12 OCR Voltage out of range, Unusable card.");

    cmd(16, 512, 0);
    addrMult = 512;
    gpio_put(cs,1);
    
}


int SDCard::cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes, uint8_t responseBytesArray[], bool release, bool skip1){
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
        spi_read_blocking(spi, FF_TOKEN, &r1, 1);
        if(!(r1&0x80)){ // checks the most significant bit of r1 is 1, which would indicate an error
            if(extraResponseBytes != 0){
                //read until start byte ()
                int cmdTimeout = CMD_TIMEOUT;
                do{
                    uint8_t r1;
                    spi_read_blocking(spi, FF_TOKEN, &r1, 1);
                    sleep_ms(1);
                } while(response != &FF_TOKEN && cmdTimeout > 0);
                if(cmdTimeout == 0) err("timeout waiting for response");

                spi_read_blocking(spi, FF_TOKEN, responseBytesArray, extraResponseBytes);
                FFClock(2);
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

int SDCard::readBlocks(uint32_t blockAddr, size_t readNum, uint8_t buf[]){
    blockAddr *= addrMult;
    readNum *= addrMult;
    int readCmd = (readNum == 1)? 17:18;
    if(cmd(readCmd,blockAddr,0, readNum, buf,false) != 0) err("I/O error for read cmd");
    uint8_t dummybuf[0];
    if(readNum != 0) if(cmd(12,0,FF_TOKEN, 0,dummybuf,true) == 0x01) err("I/O error for multiread cmd.");
    return 1;
}

int SDCard::FFClock(int clocks){
    for (size_t i = 0; i < clocks; i++)
    {
        spi_write_blocking(spi, &FF_TOKEN, 1);
    }
    return 1;
}

void SDCard::err(const char* errMessage){
    gpio_put(cs,1);
    FFClock();
    throw std::runtime_error(errMessage);
}