#include "hardware/spi.h"
#include "pico/stdlib.h"

#ifndef SDSOFTWARE_H
#define SDSOFTWARE_H


#ifdef __cplusplus
extern "C" {
#endif
int initialiseCard();
int readBlocks(uint8_t buf[], uint32_t blockAddr, unsigned int readNum);
int writeBlocks(const uint8_t buf[], uint32_t blockAddr, unsigned int writeNum);
int getStatus();
int sdIoctl(unsigned int cmd, void* buf);
// uint32_t fattime(void);
#ifdef __cplusplus
}
#endif
#endif