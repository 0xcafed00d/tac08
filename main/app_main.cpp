// ESP32 imports
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// #include "SDL.h"
// #include "config.h"
// #include "hal_fs.h"
#include "tac08.h"

// ESP32 Entrypoint
extern "C" void app_main()
{
	main(0, NULL);
	// TODO: Call main after reimplementing SDL functionality.
	// main(0, NULL);

    int i = 0;
    while (1) {
        printf("[%d] Hello world!\n", i);
        i++;
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
