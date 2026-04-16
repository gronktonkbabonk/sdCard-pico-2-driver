#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "sdcard.h"

//when in integration hell remember that the sd card must have power for 1 ms before commands can start being issued


// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19



int main()
{
    stdio_init_all();

    // SPI initialisation. This example will use SPI at 1MHz.
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    SDCard sd(spi0, PIN_CS);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
