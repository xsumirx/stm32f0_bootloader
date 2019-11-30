#include "utils.h"
#include "stdio.h"
#include "stdarg.h"


void HAL_Delay(__IO uint32_t Delay)
{
  uint32_t tickstart = HAL_GetTick();
  uint32_t wait = Delay;
  
  /* Add a period to guarantee minimum wait */
  if (wait < HAL_MAX_DELAY)
  {
     wait++;
  }
  
  while((HAL_GetTick() - tickstart) < wait)
  {
    FEED_DOG;
  }
}

//Log
void log(enum LogLevel level, char *log, ...)
{
  switch(level){
  case LOG_DEBUG: break;
  case LOG_INFO: break;
  case LOG_ERROR: break;
  case LOG_WARNING: break;
  default:break;
  }
  
  va_list argptr;
  va_start(argptr, log);
  vprintf(log, argptr);
  va_end(argptr);

}