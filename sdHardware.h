// Copyright (c) 2026 gronktonkbabonk

#include "hardware/spi.h"
#include "pico/stdlib.h"

#ifndef SDHARDWARE_H
#define SDHARDWARE_H
    const int CMD_TIMEOUT = 100000000; // lol this was way too short before
    const uint8_t FF_TOKEN = 0xFF;
    const uint8_t R1_IDLE_STATE = 1;
    const uint8_t R1_ILLEGAL_COMMAND = 1<<2;
    const uint8_t DATA_START = 0xFE;
    const uint8_t STOP_TRAN = 0xFD;

    extern bool debug;
    extern spi_inst_t *spi;
    extern uint8_t response[16];
    extern uint64_t capacity;
    extern int addrMult;
    extern bool initialised;
    extern int cs;
    extern int cd;

    uint64_t bitSlicer(uint8_t buf[], size_t width, int startLoc, int arrSize);
    int getCardSize(int ver);
    int fatalErr(const char* errMessage, int err=9);
    int cmd(uint8_t cmd, uint32_t args, uint8_t crc, int extraResponseBytes=0, bool release=true, bool skip1=false);
    int FFClock(int clocks=1);
    int v1Init();
    int v2Init();
#endif