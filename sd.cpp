#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "sdcard.h"
#include <cstring>

#define SPI_PORT spi0
#define PIN_MISO 0
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_CS   1
#define CD 22
#define ONBLED 25



int main()
{
    stdio_init_all();

    gpio_init(CD);
    gpio_set_dir(CD,GPIO_IN);
    gpio_pull_up(CD);
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

    while(!(gpio_get(CD)==0)){
        sleep_ms(100);
    }

    printf("Card inserted. Initialising...\n");
    SDCard sd(SPI_PORT, PIN_CS);

    // SDCard* sd = nullptr;
    // bool inserted = false;


    while(true){
        // if (inserted == false && gpio_get(CD) == 0){
        //     inserted = true;
        //     printf("Card inserted. Initialising...\n");
        //     sd = new SDCard(SPI_PORT, PIN_CS);
        // }
        // if(gpio_get(CD) == 1 && inserted == true){
        //     inserted = false;
        //     printf("Card removed.\n");
        //     delete sd;
        //     sd = nullptr;
        // }
        gpio_put(ONBLED,1);
        sleep_ms(250);
        gpio_put(ONBLED,0);
        sleep_ms(250);
    }
}
