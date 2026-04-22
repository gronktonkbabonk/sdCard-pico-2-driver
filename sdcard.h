#include <stdio.h>
#include "hardware/spi.h"

class SDCard
{
private:
    int FFClock(int clocks=1);
    uint8_t response[16];
    uint64_t capacity = 0;
    void fatalErr(const char* errMessage);
    int addrMult;
public:
    spi_inst_t *spi;
    int cs;
    SDCard(spi_inst_t *spiInp, int csInp);
    void sdCardInit();
    void getCardSize(int ver);
    uint8_t dummyCmd(uint8_t cmd);
    uint8_t dummyResponse(uint8_t cmd, int i);
    int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes, bool release, bool skip1);
    void v1Init();
    void v2Init();
    void readBlocks(uint32_t blockAddr, size_t readNum, uint8_t buf[]);
    int writeBlocks(uint32_t blockAddr, size_t writeNum, uint8_t buf[]);
    int ioctl(int command, void *buff);
};
