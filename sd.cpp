#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "sdHardware.h"
#include "ff.h"
#include <cstring>

#define SPI_PORT spi0
#define PIN_MISO 0
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_CS   1
#define PIN_CD 22
#define ONBLED 25

FATFS fat;

void initialise(){
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);

    gpio_init(PIN_CD);
    gpio_set_dir(PIN_CD,GPIO_IN);
    gpio_pull_up(PIN_CD);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(ONBLED);
    gpio_set_dir(ONBLED,GPIO_OUT);
    
    gpio_put(ONBLED,1);

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    
    printf("\n\n=====================START SESSION=====================\n\n\nConsole connected. Waiting for card insertion.\n");

    while(!(gpio_get(PIN_CD)==0)){
        sleep_ms(100);
    }

    printf("Card inserted. Initialising...\n");

    debug = true;
    spi = SPI_PORT;
    cs = PIN_CS;
    cd = PIN_CD;
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs,1);
}

const char* getfres(FRESULT res) {
    switch (res) {
        case FR_OK: return "FR_OK (Succeeded)";
        case FR_DISK_ERR: return "FR_DISK_ERR (A hard error occurred in the low level disk I/O layer)";
        case FR_INT_ERR: return "FR_INT_ERR (Assertion failed)";
        case FR_NOT_READY: return "FR_NOT_READY (The physical drive cannot work)";
        case FR_NO_FILE: return "FR_NO_FILE (File not found)";
        case FR_NO_PATH: return "FR_NO_PATH (Path not found)";
        case FR_INVALID_NAME: return "FR_INVALID_NAME (Invalid pathname)";
        case FR_DENIED: return "FR_DENIED (Access denied)";
        case FR_EXIST: return "FR_EXIST (File already exists)";
        case FR_INVALID_OBJECT: return "FR_INVALID_OBJECT (Invalid file/directory object)";
        case FR_WRITE_PROTECTED: return "FR_WRITE_PROTECTED (Write protected)";
        case FR_INVALID_DRIVE: return "FR_INVALID_DRIVE (Invalid drive number)";
        case FR_NOT_ENABLED: return "FR_NOT_ENABLED (Filesystem not enabled)";
        case FR_NO_FILESYSTEM: return "FR_NO_FILESYSTEM (No valid filesystem)";
        case FR_MKFS_ABORTED: return "FR_MKFS_ABORTED (Format aborted)";
        case FR_TIMEOUT: return "FR_TIMEOUT (Operation timeout)";
        case FR_LOCKED: return "FR_LOCKED (File locked)";
        case FR_NOT_ENOUGH_CORE: return "FR_NOT_ENOUGH_CORE (Insufficient memory)";
        case FR_TOO_MANY_OPEN_FILES: return "FR_TOO_MANY_OPEN_FILES (Too many open files)";
        case FR_INVALID_PARAMETER: return "FR_INVALID_PARAMETER (Invalid parameter)";
        default: return "UNKNOWN FRESULT";
    }
}

int main()
{
    initialise();

    DIR dir;
    FILINFO fno;
    FIL fil;
    char write[10000];
    FRESULT fr;

    fr = f_mount(&fat, "", 0);
    if (fr == FR_OK) printf("Mount OK!\n");
    else printf("Mount failed: %i\n", fr);
    
    // if (f_opendir(&dir, "/") == FR_OK) {
    //     while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
    //         printf("Found: %s\n", fno.fname);
    //     }
    //     f_closedir(&dir);
    // }
    
    size_t i = 0;
    char file[50];
    UINT bw;

    fr = f_open(&fil, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
    f_write(&fil, "Hello SD card!\r\n", 17, &bw);
    f_close(&fil);
    printf("file closed\n");    
    printf(getfres(f_mount(NULL, "", 1)));
    printf("Card unmounted \n");
}