#ifndef __UTILS_H__
#define __UTILS_H__

#include "stdint.h"
#include "stm32f0xx_hal.h"


#define FEED_DOG                WRITE_REG(((IWDG_TypeDef *)(IWDG_BASE))->KR, IWDG_KEY_RELOAD)

enum LogLevel{
  LOG_DEBUG,
  LOG_INFO,
  LOG_ERROR,
  LOG_WARNING,
};

//logger
void log(enum LogLevel level, char * log, ...);

#define LOGD(x, ...) log(LOG_DEBUG, x, ##__VA_ARGS__)

#endif