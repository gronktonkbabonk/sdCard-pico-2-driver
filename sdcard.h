#include <stdio.h>
#include "hardware/spi.h"
#include "ff16/source/ff.h"
#include "ff16/source/diskio.h"

class SDCard
{
private:
    int FFClock(int clocks=1);
    uint8_t response[16];
    uint64_t capacity = 0;
    void fatalErr(const char* errMessage);
    int addrMult;
public:
    SDCard(const SDCard&) = delete;
    SDCard& operator=(const SDCard&) = delete;
    bool initialized = false;
    spi_inst_t *spi;
    int cs;
    SDCard(spi_inst_t *spiInp, int csInp);
    void getCardSize(int ver);
    uint8_t dummyCmd(uint8_t cmd);
    uint8_t dummyResponse(uint8_t cmd, int i);
    int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes, bool release, bool skip1);
    void v1Init();
    void v2Init();

    DSTATUS disk_initialize(BYTE pdrv);
    DRESULT disk_read(BYTE pdrv, BYTE* buf, LBA_t blockAddr, UINT readNum);
    DRESULT disk_write(BYTE pdrv, BYTE* buf, LBA_t blockAddr, UINT readNum);
    DSTATUS disk_status (BYTE pdrv);
    DRESULT disk_ioctl(int command, void *buff);
    DWORD get_fattime(void);
};
