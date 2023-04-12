#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "zstack.h"

#define LED_PIN GPIO_NUM_2

void blinkTask(void *arg)
{
    (void) arg;
    uint8_t status = 0;

    while(1)
    {
        status ^= 1;
        gpio_set_level(LED_PIN, status);
        vTaskDelay(500);
    }
}

void app_main(void)
{
    gpio_config_t gpio;

    gpio.pin_bit_mask = 1ULL << LED_PIN;
    gpio.mode = GPIO_MODE_OUTPUT;
    gpio.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&gpio);

    zstackInit();

    xTaskCreate(blinkTask, "BLINK", 4096, NULL, tskIDLE_PRIORITY, NULL);
    vTaskDelete(NULL);
}
