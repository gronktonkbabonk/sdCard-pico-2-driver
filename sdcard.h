#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"

#ifndef DRIVER_FUNCTIONS
#define DRIVER_FUNCTIONS
constexpr int CMD_TIMEOUT = 1000;
constexpr uint8_t FF_TOKEN = 0xFF;
constexpr char R1_IDLE_STATE = 1;
constexpr char R1_ILLEGAL_COMMAND = 1<<2;
constexpr uint8_t DATA_START = 0xFE;
constexpr uint8_t STOP_TRAN = 0xFD;

extern spi_inst_t *spi;
extern uint8_t response[16];
extern uint64_t capacity;
extern int addrMult;
extern bool initialised;
extern int cs;
extern int cd;

void getCardSize(int ver);
void fatalErr(const char* errMessage);
int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes, bool release, bool skip1);
int FFClock(int clocks=1);
void v1Init();
void v2Init();

DSTATUS disk_initialize(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE* buf, LBA_t blockAddr, UINT readNum);
DRESULT disk_write(BYTE pdrv, BYTE* buf, LBA_t blockAddr, UINT readNum);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buf);
DWORD get_fattime(void);
#endif