#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "sdcard.h"

#define SPI_PORT spi0
#define PIN_MISO 0
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_CS   1
#define ONBLED 25



int main()
{
    stdio_init_all();

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
    
    printf("Console connected, starting driver...\n");


    SDCard sd(spi0, PIN_CS);

    while(true){
        gpio_put(ONBLED,1);
        sleep_ms(250);
        gpio_put(ONBLED,0);
        sleep_ms(250);

    }
}
