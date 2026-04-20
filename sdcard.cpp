#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "sdcard.h"

// Resources used:
//https://nodeloop.org/guides/sd-card-spi-init-guide/
//https://electronics.stackexchange.com/questions/77417/what-is-the-correct-command-sequence-for-microsd-card-initialization-in-spi
//2023 sd card physical layer simplified spec
//duckduckgo lol


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
    printf("init started \n");
    spi_init(spi, 1000*100); // clock rate to 400khz for init  
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
    printf("Sd card init successful! \n");
    spi_init(spi,1000*1000*25);
}

void SDCard::v2Init(){
    printf("v2 card detected\n");
    if((response[3] & 0xF0) != 0x10) err("v2 voltage out of range, Unusable card.");
    
    
    int timeout = CMD_TIMEOUT;
    uint8_t r1 = 0;
    do
    {
        cmd(55,0,0x65, 0, response, false);  
        r1=cmd(41,0x40000000, 0, false);
    } while (r1 != 0x00 && timeout--);
    if (timeout==0) err("Failed to initialise v2 card, timed out while waiting.");
    
    cmd(58, 0, 0xfd, 4);
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & 0x00FF8000)) err("v2 OCR voltage out of range, Unusable card.");
    printf("v2 ocr voltage in range.\n");

    
    addrMult = (((ocr >> 30) & 1)? 1 : 512);
    gpio_put(cs,1);
    FFClock();
    return;
}

void SDCard::v1Init(){
    printf("v1 card detected\n");
    int timeout = CMD_TIMEOUT;
    uint8_t r1;
    do
    {
        cmd(55,0,0x65, 0, response, false);  
        r1=cmd(41,0, 0, false);
    } while ((r1 != 0x00) && timeout--);
    if (timeout==0) err("Failed to initialise v1 card, timed out while waiting.");
    
    cmd(58, 0, 0xfd, 4 );
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & 0x00FF8000)) err("v1 OCR voltage out of range, Unusable card.");
    printf("v1 ocr voltage in range.\n");

    cmd(16, 512, 0);
    addrMult = 512;
    gpio_put(cs,1);
    return;    
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

    printf("cmd %u called \n", (unsigned int)cmd);

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
            printf("All extra response bytes finished\n");
            if(release){
                gpio_put(cs,1);
                printf("Cs released \n");
                FFClock();
            }
            return r1;
        }
    }
    
    //timeout
    gpio_put(cs,1);
    FFClock();
    printf("Timeout while waiting for cmd response \n");
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
    printf("System clocked for %i byte(s)\n",clocks);
    return 1;
}

void SDCard::err(const char* errMessage){
    gpio_put(cs,1);
    FFClock();
    throw std::runtime_error(errMessage);
}

uint8_t SDCard::dummyCmd(uint8_t cmd){
    switch (cmd)
    {
    case 0:
        return 0x01;
    case 0x08:
        // return 0x01;
        return R1_ILLEGAL_COMMAND;
    case 0x29:
        return 0x00;
    }
    return R1_ILLEGAL_COMMAND;
}

uint8_t SDCard::dummyResponse(uint8_t cmd, int i){
    uint8_t response8[4] = {0,0,0,0x10};
    uint8_t response58[4] = {0,0xFF,0x80,0};
    if (cmd==0x08) return response8[i];
    else return response58[i];
}