#ifndef __UART_HANDLE_H
#define __UART_HANDLE_H

#include "stm32f0xx_hal.h"
#include "stdint.h"
#include "stdbool.h"
#include "queue_manager.h"

typedef struct _UartHandle {
  USART_TypeDef *Instance;
  Queue_t qReceive;
  Queue_t qTransmit;
}UartHandle_t;


void UartInit(UartHandle_t *uart);
void UartWrite(UartHandle_t *uart, uint8_t *buffer, uint32_t bufferSize);
bool UartAvailable(UartHandle_t *uart);
uint8_t UartRead(UartHandle_t *uart);
void UartCleanRx(UartHandle_t *uart);
void UartFlushTx(UartHandle_t *uart);
void UartISR(UartHandle_t *uart);

#endif