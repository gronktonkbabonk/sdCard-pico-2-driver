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
    
    printf("Console connected. Waiting for card insertion.\n");

    while(!(gpio_get(PIN_CD)==0)){
        sleep_ms(100);
    }

    printf("Card inserted. Initialising...\n");


    spi = SPI_PORT;
    cs = PIN_CS;
    cd = PIN_CD;
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs,1);
}

int main()
{
    initialise();

    FIL fil;        /* File object */
    char line[100]; /* Line buffer */
    FRESULT fr;     /* FatFs return code */


    /* Give a work area to the default drive */
    f_mount(&fat, "", 0);

    /* Open a text file */
    fr = f_open(&fil, "slasher mix.m3u", FA_READ);
    if (fr) return (int)fr;

    /* Read every line and display it */
    while (f_gets(line, sizeof line, &fil)) {
        printf("%s", line);
    }

    /* Close the file */
    f_close(&fil);
}
