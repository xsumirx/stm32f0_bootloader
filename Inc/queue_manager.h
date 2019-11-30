#ifndef __QUEUE_MANAGER_H
#define __QUEUE_MANAGER_H

#include "stdint.h"
#include "stdbool.h"

#define QUEUE_SIZE 512

typedef struct _Queue
{
  uint8_t      data[QUEUE_SIZE];
  int32_t      frontIndex;
  int32_t     rearIndex;
}Queue_t;


void QueueInit(Queue_t *queue);
void QueuePush(Queue_t *queue, char data);
char QueuePop(Queue_t *queue);
bool QueueAvailable(Queue_t *queue);
void QueueClean(Queue_t *queue);

#endif