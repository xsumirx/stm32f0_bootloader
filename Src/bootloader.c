#include "bootloader.h"
#include "utils.h"
#include "stdio.h"
#include "main.h"
#include "uart_handle.h"
#include "w25qxx.h"
#include "internal_flash_driver.h"

static UartHandle_t uart;
extern UART_HandleTypeDef huart2;

typedef void (*pFunction)(void);

//*********** Internal Flash MAP *****************************************************************

#if !defined(FLASH_PAGE_SIZE)
#define FLASH_PAGE_SIZE         2048
#endif

// Bootloader
#define FLASH_BOOTLOADER_ADDR_BASE      0x08000000
#define FLASH_BOOTLOADER_SIZE_SIZE      20*1024         //20KB

// Main Firmware
#define FLASH_FW_ADDR_INFO              (FLASH_BOOTLOADER_ADDR_BASE + FLASH_BOOTLOADER_SIZE_SIZE)
#define FLASH_FW_SIZE_INFO              16       

#define FLASH_FW_ADDR_BOOT              (FLASH_FW_ADDR_INFO + FLASH_FW_SIZE_INFO)
#define FLASH_FW_SIZE_BOOT              ((76 * 1024) - 16)       //<76 KB

// Fail Safe Firmware 
#define FLASH_FAIL_ADDR_INFO            (FLASH_FW_ADDR_BOOT + FLASH_FW_SIZE_BOOT)
#define FLASH_FAIL_SIZE_INFO            16

#define FLASH_FAIL_ADDR_BOOT            (FLASH_FAIL_ADDR_INFO + FLASH_FAIL_SIZE_INFO)
#define FLASH_FAIL_SIZE_BOOT            ((32 * 1024) - 16)      // <32 KB

//**********************************************************************************************


//********************************* External Flash Firmware MAP ********************************

#define EXT_FLASH_FW_ADDR_INFO          0x00000000
#define EXT_FLASH_FW_SIZE_INFO          256

#define EXT_FLASH_FW_ADDR_DATA          (EXT_FLASH_FW_ADDR_INFO + EXT_FLASH_FW_SIZE_INFO)
#define EXT_FLASH_FW_SIZE_DATA          (76 * 1024)     //76 KB

#define FLASH_TRUE                  0x00
#define FLASH_FALSE                 0xFF

//*********************************************************************************************

//************ Local Flags *******************
#define PERIPHERAL_EXT_FLASH 0x01
#define FW_INFO_MAGIC         0x5B7A
#define MAX_BOOT_RETRY  10


//Firmware Update Infomation in External Flash
__packed struct ExtFwInfo {
  uint16_t      MAGIC_HASH;
  uint8_t       available;
  uint8_t       copied;
  uint32_t      hash;
  int32_t       size;
}__attribute__((packed));

__packed struct FwInfo {
  uint32_t      MAGIC_HASH;
  uint32_t      hash;
  int32_t       size;
};

__packed typedef struct
{
  uint16_t magic;
  uint16_t retryCount;
}SharedData_t;


//Whole Application Context
struct Context{
  uint8_t pStatus;
  struct ExtFwInfo updateInfo;
};


struct Context ctx = {.pStatus = 0xFF};

#pragma location="SHARED_DATA"
SharedData_t shared;


//Function Forward Declarations
bool updateAvailable();
bool copyExtFlashToFlash(uint8_t *src, uint8_t *dst, uint32_t size);
void setBackup(uint8_t val);
uint8_t getBackup();
bool isBackupValid();
void initBackup();
bool isRetryValid();
bool copyFwInfo(int32_t size, uint32_t crc);
bool markCopied();
bool copyUpdate();
bool copyFailSafe();
bool eraseBootImage();
void boot();

//Bootloader State machine
void bootloaderSetup()
{
  uart.Instance = huart2.Instance;
  UartInit(&uart);
  LOGD("Hello From Bootloader.....\n");
  
  if(shared.magic != FW_INFO_MAGIC){
    LOGD("Booting from poweroff.....\n");
    shared.magic = FW_INFO_MAGIC;
    shared.retryCount = 0;
  }
  
  //Init External Flash
  if(!W25qxx_Init()){
    LOGD("Failed to init external flash\n");
    ctx.pStatus |= PERIPHERAL_EXT_FLASH;
  }
 
  while(1)
  {
    if(!updateAvailable()){
      LOGD("Update Not available.\n");
      if(!isBackupValid()){
        LOGD("Backup not valid.\n");
        initBackup();
      }
      
      if(isRetryValid()){
        boot();
      }
      
      LOGD("Max retry count reached.... Copying fail-safe.\n");
      if(copyFailSafe()){
        LOGD("Succes\n");
        initBackup();
        boot();
      }else{
        LOGD("Failed\n");
      }
      
    }else{
      LOGD("Update available.\n");
      LOGD("Updating....\n");
      if(copyUpdate() &&  markCopied()){
        initBackup();
        LOGD("Success\n");
        boot();
      }else{
        LOGD("Failed\n");
      }
    }
  }
}


bool updateAvailable(){
  W25qxx_ReadBytes((uint8_t *)&ctx.updateInfo,
                   EXT_FLASH_FW_ADDR_INFO,sizeof(struct ExtFwInfo));
  if(ctx.updateInfo.MAGIC_HASH != FW_INFO_MAGIC)
    return false;
  
  if(ctx.updateInfo.size > FLASH_FW_SIZE_BOOT || 
     ctx.updateInfo.size < 0 )
    return false;
  
  if(ctx.updateInfo.available == FLASH_FALSE ||
     ctx.updateInfo.copied == FLASH_TRUE)
    return false;
  
  return true;
}

bool copyExtFlashToFlash(uint8_t *src, uint8_t *dst, uint32_t size)
{
  
  return true;
}

bool isBackupValid(){

  return (shared.magic == FW_INFO_MAGIC);
}

void initBackup(){
  shared.magic = FW_INFO_MAGIC;
  shared.retryCount = 0;
}


bool isRetryValid(){
  return (shared.retryCount++ < MAX_BOOT_RETRY);
}


bool copyFwInfo(int32_t size, uint32_t crc)
{
  //Write code into flash
  struct FwInfo fwInfo;
  fwInfo.hash = ctx.updateInfo.hash;
  fwInfo.size = ctx.updateInfo.size;
  fwInfo.MAGIC_HASH = FW_INFO_MAGIC;
  
  return FlashWrite((volatile uint32_t *)((void *)FLASH_FW_ADDR_INFO), (const uint8_t *)((void *)&fwInfo), sizeof(struct FwInfo));
}

bool markCopied() {
  struct ExtFwInfo info;
  W25qxx_ReadBytes((uint8_t *)&info,
                   EXT_FLASH_FW_ADDR_INFO,sizeof(struct ExtFwInfo));
  if(info.MAGIC_HASH != FW_INFO_MAGIC)
    return false;
  
  info.copied = FLASH_TRUE;
  W25qxx_WritePage((uint8_t *)&info, EXT_FLASH_FW_ADDR_INFO, 0, sizeof(struct ExtFwInfo));
  
  return true;
}

#define TEMP_BUF_SIZE 96
uint8_t tmpBuffer[TEMP_BUF_SIZE] = {0};
bool copyUpdate(){
  
  bool status = true;
  int32_t updateSize = ctx.updateInfo.size;
  uint32_t updateProgress = 0;
  
  LOGD("Unlocking Flash...\n");
  if(!FlashUnlock()){
    LOGD("Unlocking Flash Failed\n");
    return false;
  }
  LOGD("Erasing flash...\n");
  eraseBootImage();
  LOGD("Programming....\n");
  do{
    
    int32_t tempSize = (updateSize > TEMP_BUF_SIZE) ? TEMP_BUF_SIZE:updateSize;
    //Read From External Flash
    W25qxx_ReadBytes(tmpBuffer, EXT_FLASH_FW_ADDR_DATA + updateProgress, tempSize);
    
    //Write Into Internal
    if(!FlashWrite((volatile uint32_t *)(FLASH_FW_ADDR_BOOT + updateProgress), (const uint8_t *)tmpBuffer, tempSize))
    {
      LOGD("Flash Write Failed\n");
      status = false;
      break;
    }
    
    updateSize -= tempSize;
    updateProgress += tempSize;
  
    LOGD("Progress - %d\n", ((updateProgress /ctx.updateInfo.size) * 100));
    
  }while(updateSize > 0);
  
  //Copy Fw Info
  if(!copyFwInfo(ctx.updateInfo.size, ctx.updateInfo.hash))
  {
    LOGD("Copy Info Failed\n");
    status = false;
  }
  
  LOGD("Success\n");
  FlashLock();
  return status;
}

bool copyFailSafe(){

  struct FwInfo *info = (struct FwInfo *)((void *)FLASH_FAIL_ADDR_INFO);
  
  if(info->MAGIC_HASH != FW_INFO_MAGIC || 
     (info->size < 0 && info->size > (FLASH_FAIL_SIZE_BOOT + FLASH_FAIL_SIZE_INFO)))
   {
      //Haha.....:) There is no fail safe firmware in the device. You are f**ked !
     int fuckedMessageCount = 10;
     while(fuckedMessageCount -- < 0){
       LOGD("There is no fail safe firmware :(\n");
       HAL_Delay(1000);
       shared.retryCount = 0;
       NVIC_SystemReset();
     }
   }
  
  
  bool status = true;
  
  LOGD("Unlocking Flash...\n");
  if(!FlashUnlock()){
    LOGD("Failed\n");
    return false;
  }
  LOGD("Erasing flash...\n");
  eraseBootImage();
  LOGD("Programing.....\n");
  if(!FlashWrite((volatile uint32_t *)((void *)FLASH_FW_ADDR_INFO),
                 (const uint8_t *)((void *)FLASH_FAIL_ADDR_INFO), 
                 FLASH_FAIL_SIZE_INFO + FLASH_FAIL_SIZE_BOOT))
  {
    LOGD("Flash Write Failed\n");
    status = false;
  }
  
  
  FlashLock();
  LOGD("Success\n");
  return status;
}


bool eraseBootImage(){
  
  uint32_t startPageAddress = FLASH_FW_ADDR_INFO;
  uint32_t noOfPages = (FLASH_FW_SIZE_INFO + FLASH_FW_SIZE_BOOT) / FLASH_PAGE_SIZE;
  
  //Erase Each Page
  for(int i=0; i<noOfPages; i++){
    FlashErasePage(startPageAddress + (i * FLASH_PAGE_SIZE));
  }
  
  return true;
}

void boot(){
  LOGD("Booting.....\n");
  pFunction appEntry;
  uint32_t appStack;
  appStack = (uint32_t) *((__IO uint32_t *) FLASH_FW_ADDR_BOOT);

  //Get Program Count -> ResetHandler
  /* Get the application entry point (Second entry in the application vector table)*/
  appEntry = (pFunction) *(__IO uint32_t*) (FLASH_FW_ADDR_BOOT + 4);

  /* Relocate the vector table */
  volatile uint32_t *VectorTable = (volatile uint32_t *)0x20000000;
  for(int i = 0; i < 48; i++)
  {
    VectorTable[i] = *(__IO uint32_t*)(FLASH_FW_ADDR_BOOT + (i<<2));
  }
  __HAL_RCC_AHB_FORCE_RESET();
 __HAL_RCC_SYSCFG_CLK_ENABLE();
 __HAL_RCC_AHB_RELEASE_RESET();
  __DSB();
  __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
  __DSB();
  __ISB();
  
  /* Set the application stack pointer */
  __set_MSP(appStack);

  /* Start the application */
  appEntry();
}



//System Function
int fputc(int ch, FILE *f)
{
  UartWrite(&uart, (uint8_t *)ch, 1);
  return ch;
}

