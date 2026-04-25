#include "sdHardware.h"
#include "sdSoftware.h"
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <time.h> 
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ff.h"
#include "diskio.h"

// Resources used:
//https://github.com/sbcshop/MicroSD-Breakout/blob/main/sdcard.py -> implementation example
//https://nodeloop.org/guides/sd-card-spi-init-guide/
//https://electronics.stackexchange.com/questions/77417/what-is-the-correct-command-sequence-for-microsd-card-initialization-in-spi
//2023 sd card physical layer simplified spec
//https://elm-chan.org/fsw/ff/ -> lots of stuff here
//duckduckgo lol

spi_inst_t *spi;
uint8_t response[16];
uint64_t capacity = 0;
int addrMult;
bool initialised = false;
int cs;
int cd;

uint64_t bitSlicer(uint8_t buf[], size_t width, int startLoc, int arrSize){
    startLoc = arrSize-1-startLoc;
    uint64_t returnNum = 0;
    for(size_t i = 0; i < width; i++) returnNum |= ((buf[(startLoc+i)/8] >> (7-((startLoc+i)%8)))&1)<<(width-1-i);
    return returnNum;
}

void getCardSize(int ver){
    cmd(9,0,0,0,false,false);

    uint8_t reply;    
    int cmdtimeout = CMD_TIMEOUT;
    do{
        spi_read_blocking(spi, FF_TOKEN, &reply,1);
        cmdtimeout--;
    }while (reply != DATA_START && cmdtimeout > 0);
    if(cmdtimeout == 0) fatalErr("timeout while waiting for csd register.");
    spi_read_blocking(spi, FF_TOKEN, response, 16);
    gpio_put(cs,1);   
    FFClock(2);
   
    if (ver == 1){
        int READ_BL_LEN = bitSlicer(response, 4, 83,128);
        int C_SIZE_MULT = bitSlicer(response, 3, 78,128);
        int C_SIZE = bitSlicer(response, 12, 54,128);
        int BLOCK_LEN = 1<<READ_BL_LEN;
        int MULT = 1<<(C_SIZE_MULT+2);
        int BLOCKNR = (C_SIZE+1)*MULT;
        capacity = BLOCKNR * BLOCK_LEN;
    }else{
        uint64_t C_SIZE = bitSlicer(response,22,69,128);
        capacity = ((C_SIZE+1ull) << 19);
    }
}

extern "C" int initialiseCard(){
    if (initialised) return 0;
    sleep_ms(1);
    printf("init started \n");
    spi_init(spi, 1000*100); // clock rate to 400khz for init  
    spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    FFClock(10);

    bool present = false;
    for (size_t i = 0; i < 5; i++)
    {
        if(cmd(0,0,0x95,0,true,false) == 0x01){
            present=true;
            break;
        }
    }
    if(!present){
        fatalErr("No sd card is present.");
    }
    //card is present
    //printf("Card is present\n");

    int cmd8res = cmd(8, 0x1AA, 0x87, 4, true,false);
    if(cmd8res == R1_IDLE_STATE) v2Init(); //v2 card present
    else if(cmd8res == R1_ILLEGAL_COMMAND) v1Init();
    else fatalErr("Bad response from SD, unable to determine version.");

    printf("Sd card init successful! \n");
    printf("Detected card capacity: %f GB\n",(float)capacity/(float)1000000000);
    spi_init(spi,1000*1000*25);
    return 0;
    initialised = true;
}

void v2Init(){
    //printf("v2 card detected\n");
    if((response[2] & 0x0F) != 0x01) fatalErr("v2 voltage out of range, Unusable card.");
    
    int timeout = CMD_TIMEOUT;
    uint8_t r1 = 0;
    printf("started 55/41\n");
    do
    {
        cmd(55,0,0x65,0,false,false);  
        r1=cmd(41,0x40000000, 0,0,true,false);
    } while (r1 != 0x00 && timeout--);
    if (timeout==0) fatalErr("Failed to initialise v2 card, timed out while waiting.");
    gpio_put(cs,1);
    FFClock();
    printf("ended 55/41 in %i cycles\n", CMD_TIMEOUT-timeout);
    
    cmd(58, 0, 0xfd, 4,true,false);
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & 0x00FF8000)) fatalErr("v2 OCR voltage out of range, Unusable card.");
    // //printf("v2 ocr voltage in range.\n");

    getCardSize(2);    
    addrMult = (((ocr >> 30) & 1)? 1 : 512);
    return;
}

void v1Init(){
    //printf("v1 card detected\n");

    
    int timeout = CMD_TIMEOUT;
    uint8_t r1;
    printf("started 55/41\n");
    do
    {
        cmd(55,0,0x65, 0,  false,false);  
        r1=cmd(41,0, 0, 0,true,false);
    } while ((r1 != 0x00) && timeout--);
    if (timeout==0) fatalErr("Failed to initialise v1 card, timed out while waiting.");
    gpio_put(cs,1);
    FFClock();
    printf("ended 55/41 in %i cycles\n", CMD_TIMEOUT-timeout);
    
    
    cmd(58, 0, 0xfd, 4,  true, false);
    uint32_t ocr = (response[0] << 24)|(response[1] << 16)|(response[2] << 8)|response[3]; //reconstructing ocr
    if (!(ocr & 0x00FF8000)) fatalErr("v1 OCR voltage out of range, Unusable card.");
    //printf("v1 ocr voltage in range.\n");
    
    cmd(16, 512, 0, 0, true, false);
    getCardSize(1);    
    addrMult = 512;
    return;    
}

int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes, bool release, bool skip1){
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

    if (cmd != 41 && cmd != 55) printf("cmd %u called \n", (unsigned int)cmd);
    
    memset(response, 0, 16);
    

    spi_write_blocking(spi, buf, 6);
    
    if(skip1){
        FFClock();
    }
    for (size_t i = 0; i < CMD_TIMEOUT; i++){
        uint8_t r1;
        spi_read_blocking(spi, FF_TOKEN, &r1, 1);
        if(!(r1&0x80)){ // checks the most significant bit of r1 is 1, which would indicate an error
            if (cmd != 41 && cmd != 55) printf("r1 read: 0x%02x\n",r1);
            if(extraResponseBytes != 0){
                spi_read_blocking(spi, FF_TOKEN,response, extraResponseBytes);
                FFClock(2);
                //printf("All extra response bytes finished\n");
                
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
    //printf("Timeout while waiting for cmd response \n");
    return -1;
}

extern "C" int readBlocks(uint8_t buf[], uint32_t blockAddr, unsigned int readNum){
    blockAddr *= addrMult;
    int readCmd = (readNum == 1)? 17 : 18;
    if(cmd(readCmd,blockAddr,0,0, false, false) != 0) fatalErr("I/O error for read cmd");
    uint8_t reply;    
    for (size_t i = 0; i < readNum; i++){
        int cmdtimeout = CMD_TIMEOUT;
        do{
            spi_read_blocking(spi, FF_TOKEN, &reply,1);
            cmdtimeout--;
        }while (reply != DATA_START & cmdtimeout > 0);
        spi_read_blocking(spi, FF_TOKEN, buf+(i*512), 512);
        FFClock(2);
    }
    gpio_put(cs,1);    
    if(readNum > 1){if(cmd(12,0,FF_TOKEN,0,true, true) == 0x01) fatalErr("I/O error for multiread cmd.");}
    return RES_OK;
}

extern "C" int writeBlocks(const uint8_t buf[], uint32_t blockAddr, unsigned int writeNum){
    blockAddr *= addrMult;
    int writeCmd = (writeNum==1)? 24 : 25;
    if(cmd(writeCmd,blockAddr,0,0,true,false) != 0) fatalErr("I/O error for write cmd");
    gpio_put(cs,0);
    uint8_t response;
    //printf("Scanning for start token\n");
    uint8_t r1;
    do{
        spi_read_blocking(spi, FF_TOKEN, &r1, 1);
        //printf("Token detected: 0x%02x\n",r1);
        sleep_ms(1);
    } while(r1 != FF_TOKEN);
    //printf("Start token detected\n");
    spi_write_read_blocking(spi, &DATA_START, &response, 1);
    spi_write_blocking(spi,buf,writeNum);
    FFClock(2);
    if (response & 0x05){
        //printf("Error while writing: %i\n", response);
    }
    gpio_put(cs,1);
    FFClock();

    gpio_put(cs,0);
    spi_write_blocking(spi,&STOP_TRAN,1);
    gpio_put(cs,1);
    FFClock();
    return RES_OK;
}

int FFClock(int clocks){
    for (size_t i = 0; i < clocks; i++)
    {
        spi_write_blocking(spi, &FF_TOKEN, 1);
    }
    // //printf("System clocked for %i byte(s)\n",clocks);
    return 1;
}

void fatalErr(const char* errMessage){
    gpio_put(cs,1);
    FFClock();
    throw std::runtime_error(errMessage);
}

extern "C" int sdIoctl(unsigned int cmd, void* buf){
    if (!initialised) return RES_NOTRDY;
    switch (cmd){
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(LBA_t*)buf = (capacity/512)+1;
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD*)buf=512;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD*)buf = 1;
        return RES_OK;
    case CTRL_TRIM:
        return RES_OK;
    }
    return RES_PARERR;
}

extern "C" int getStatus(){
    if(!initialised) return STA_NOINIT;
    if(gpio_get(cd) == 0) return STA_NODISK;
    return 0;
}

extern "C" DWORD get_fattime (void)
{
    time_t t;
    struct tm *stm;


    t = time(0);
    stm = localtime(&t);

    return (DWORD)(stm->tm_year - 80) << 25 |
           (DWORD)(stm->tm_mon + 1) << 21 |
           (DWORD)stm->tm_mday << 16 |
           (DWORD)stm->tm_hour << 11 |
           (DWORD)stm->tm_min << 5 |
           (DWORD)stm->tm_sec >> 1;
}