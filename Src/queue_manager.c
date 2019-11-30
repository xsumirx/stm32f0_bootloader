#include "queue_manager.h"
#include "string.h"

void QueueInit(Queue_t *queue)
{
  queue->frontIndex = queue->rearIndex = -1;
}

bool QueueAvailable(Queue_t *queue)
{
  if (queue->frontIndex == -1) { 
      return false;
  }
  
  return true;
}

void QueuePush(Queue_t *queue, char data)
{
  if((queue->rearIndex == QUEUE_SIZE - 1 && queue->frontIndex == 0) ||
     (queue->rearIndex ==(queue->frontIndex -1))){
    //Queue is Full -- Pop Last Element
    QueuePop(queue);
  }
  
  if (queue->frontIndex == -1){ 
    queue->rearIndex = queue->frontIndex = 0; 
    queue->data[queue->rearIndex] = data;
  } else if (queue->rearIndex == QUEUE_SIZE - 1 
             && queue->frontIndex != 0){ 
    queue->rearIndex = 0; 
    queue->data[queue->rearIndex] = data; 
  } else { 
    queue->rearIndex++; 
    queue->data[queue->rearIndex] = data;
  }
}


//Push Data Into Queue
char QueuePop(Queue_t *queue){
  if (queue->frontIndex == -1) { 
      return 0;
  } 
  
  char data = queue->data[queue->frontIndex];
  if (queue->frontIndex == queue->rearIndex){ 
    queue->frontIndex = queue->rearIndex = -1;
  }else if (queue->frontIndex == QUEUE_SIZE - 1) 
    queue->frontIndex = 0; 
  else
    queue->frontIndex++; 
  
  return data;
}


void QueueClean(Queue_t *queue)
{
  queue->frontIndex = queue->rearIndex = -1;
  memset(queue->data, 0, QUEUE_SIZE);
}
