#include <stdio.h>
#include "hardware/spi.h"

class SDCard
{
private:
    int FFClock(int clocks=1);
    uint8_t dummybuf[0];
    uint8_t response[4];
    void err(const char* errMessage);
    int addrMult;
public:
    spi_inst_t *spi;
    int cs;
    SDCard(spi_inst_t *spiInp, int csInp);
    void sdCardInit();
    int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes =0, const uint8_t responseBytesArray[]={}, bool release = true, bool skip1 = true);
    void v1Init();
    void v2Init();
    int readBlocks(uint32_t blockAddr, size_t readNum, uint8_t buf[]);
};
