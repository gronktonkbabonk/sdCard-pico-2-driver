#include <stdio.h>
#include "hardware/spi.h"

class SDCard
{
private:
    int FFClock(int clocks=1);
    uint8_t response[16];
    void err(const char* errMessage);
    int addressMult;
public:
    spi_inst_t *spi;
    int cs;
    SDCard(spi_inst_t *spiInp, int csInp);
    void sdCardInit();
    int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBits=0, bool release = true, bool skip1 = true);
    uint8_t dummyCmd(uint8_t cmd);
    uint8_t dummyResponse(uint8_t cmd, int i);
    void v1Init();
    void v2Init();
};
