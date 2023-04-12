#include <string.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include "zstack.h"

static void zstackInputTask(void *arg)
{
    (void) arg;

    char buffer[ZSTACK_RX_BUFFER_SIZE];

    while (1)
    {
        uint16_t length = uart_read_bytes(ZSTACK_UART, buffer, sizeof(buffer), 20);

        if (length)
        {
            buffer[length] = 0;
            printf("received: %s\n", buffer);
        }   
    }
}

void zstackInit(void)
{
    uart_config_t uart;

    memset(&uart, 0, sizeof(uart));

    uart.baud_rate = 115200;
    uart.data_bits = UART_DATA_8_BITS;
    uart.parity = UART_PARITY_DISABLE;
    uart.stop_bits = UART_STOP_BITS_1;
    uart.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    uart_driver_install(ZSTACK_UART, ZSTACK_RX_BUFFER_SIZE, 0, 0, NULL, 0);
    uart_param_config(ZSTACK_UART, &uart);
    uart_set_pin(ZSTACK_UART, ZSTACK_TX_PIN, ZSTACK_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(zstackInputTask, "ZSTACK INPUT", 4096, NULL, tskIDLE_PRIORITY, NULL);
}
