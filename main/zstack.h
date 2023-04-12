#ifndef ZSTACK_H
#define ZSTACK_H

#define ZSTACK_UART             UART_NUM_2
#define ZSTACK_RX_PIN           GPIO_NUM_18
#define ZSTACK_TX_PIN           GPIO_NUM_19
#define ZSTACK_RX_BUFFER_SIZE   1024

void zstackInit(void);

#endif
