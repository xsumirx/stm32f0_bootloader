#include "uart_handle.h"

#define MAX_BUFFER QUEUE_SIZE

//Clear and disable all the interrupt
static void UartDisableAndClearAllInterrupt(UartHandle_t *uart)
{
  uart->Instance->CR1 &= 
    ~(USART_CR1_RXNEIE |
      USART_CR1_TXEIE | 
      USART_CR1_CMIE |
      USART_CR1_PEIE |
      USART_CR1_TCIE |
      USART_CR1_RTOIE |
      USART_CR1_IDLEIE);
  
   uart->Instance->CR3 &= 
    ~(USART_CR3_EIE |
      USART_CR3_CTSIE);
  
   
   //Clear Flag
   uart->Instance->ICR |=
     (USART_ICR_CMCF |
      USART_ICR_CTSCF |
      USART_ICR_RTOCF |
      USART_ICR_TCCF |
      USART_ICR_IDLECF |
      USART_ICR_ORECF |
      USART_ICR_NCF |
      USART_ICR_FECF |
      USART_ICR_PECF);
  
}

//Enable/Disbale Error Interrupt
static void UartErrorInterruptEnable(UartHandle_t *uart, 
                                       bool enable)
{
  if(enable){
    uart->Instance->CR3 |= (USART_CR3_EIE );
  }else {
    uart->Instance->CR3 &= ~(USART_CR3_EIE);
  }
}


//Enable/Disbale Receive Interrupt
static void UartReceiveInterruptEnable(UartHandle_t *uart, 
                                       bool enable)
{
  if(enable){
    uart->Instance->CR1 |= (USART_CR1_RXNEIE );
  }else {
    uart->Instance->CR1 &= ~(USART_CR1_RXNEIE);
  }
}

//Enable/Disable Transmit Interrupt 
static void UartTransmitInterruptEnable(UartHandle_t *uart,
                                        bool enable)
{
  if(enable){
    uart->Instance->CR1 |= (USART_CR1_TXEIE );
  }else {
    uart->Instance->CR1 &= ~(USART_CR1_TXEIE);
  }
}

void UartInit(UartHandle_t *uart)
{
  
  //Enable All the Interrupt
  UartDisableAndClearAllInterrupt(uart);
  UartErrorInterruptEnable(uart, true);
  UartReceiveInterruptEnable(uart, true);
  //Init Queue
  QueueInit(&uart->qReceive);
  QueueInit(&uart->qTransmit);
}

void UartCleanRx(UartHandle_t *uart)
{
  UartReceiveInterruptEnable(uart, false);
  QueueClean(&uart->qReceive);
  UartReceiveInterruptEnable(uart, true);
}

void UartFlushTx(UartHandle_t *uart)
{
  UartTransmitInterruptEnable(uart, false);
  QueueClean(&uart->qTransmit);
}

//Write data into Queue for Scheduling transmission
void UartWrite(UartHandle_t *uart, uint8_t *buffer, uint32_t bufferSize)
{
  
  //printf("-> ");
  //HexDump(buffer, bufferSize);
  for(int i=0; (i<bufferSize) && (i < MAX_BUFFER); i++)
    QueuePush(&uart->qTransmit, *(buffer + i));
  
  //Raise Transmission Interupt
  UartTransmitInterruptEnable(uart, true);
}


//If data Available for Reading
bool UartAvailable(UartHandle_t *uart)
{
  return QueueAvailable(&uart->qReceive);
}

//Read Data from the Queue
uint8_t UartRead(UartHandle_t *uart)
{
  return (uint8_t)QueuePop(&uart->qReceive);
}

//Handle Interrupt
char x = 0;
void UartISR(UartHandle_t *uart)
{
  if(uart->Instance->ISR & USART_ISR_RXNE)
  {
    //Read and Queue Data
    QueuePush(&uart->qReceive, uart->Instance->RDR);
  }else if (uart->Instance->ISR & USART_ISR_TXE){
    //Transmission Ready to Take More
    if(!QueueAvailable(&uart->qTransmit)){
      UartTransmitInterruptEnable(uart, false);
    }else{
      uart->Instance->TDR = QueuePop(&uart->qTransmit);
    }
  } 

  //Clear Flag
   uart->Instance->ICR |=
     (USART_ICR_CMCF |
      USART_ICR_CTSCF |
      USART_ICR_RTOCF |
      USART_ICR_TCCF |
      USART_ICR_IDLECF |
      USART_ICR_ORECF |
      USART_ICR_NCF |
      USART_ICR_FECF |
      USART_ICR_PECF);
}

