#include "internal_flash_driver.h"


bool FlashErasePage(uint32_t PageAddress)
{
  /* Proceed to erase the page */
  SET_BIT(FLASH->CR, FLASH_CR_PER);
  WRITE_REG(FLASH->AR, PageAddress);
  SET_BIT(FLASH->CR, FLASH_CR_STRT);
  while (FLASH->SR & FLASH_SR_BSY) continue;
  
  return true;
}

bool FlashWrite(volatile uint32_t *flash, const uint8_t *data, uint32_t count)
{
  uint32_t primask;
  const uint8_t *data_e;
  data_e = data + count;
  uint8_t *flashAddr = ((uint8_t*)((const void*)flash));
  if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET) return false;
  do
  {
    primask = __get_PRIMASK();
    __disable_irq();
    FLASH->CR = FLASH_CR_PG;
    *flashAddr = *data;
    flash += 1;
    data  += 1;
    __DMB();
    while (FLASH->SR & FLASH_SR_BSY) continue;
    __set_PRIMASK(primask);
    if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPERR)) return false;
  }
  while (data < data_e);
  
  return true;
}

bool FlashUnlock()
{
  if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET)
  {
    /* Authorize the FLASH Registers access */
    WRITE_REG(FLASH->KEYR, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR, FLASH_KEY2);
    /* Verify Flash is unlocked */
    if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != RESET) return false;
  }

  return true;
}

bool FlashLock()
{
  SET_BIT(FLASH->CR, FLASH_CR_LOCK);
  return true;
}