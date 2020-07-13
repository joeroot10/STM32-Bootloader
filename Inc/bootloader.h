/******************************************
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
**************************************************/



#ifndef         __BOOTLOADER__H_

#define         __BOOTLOADER__H_


#ifdef __cplusplus
 extern "C" {
#endif


#define         _IS_APP(addr)       (((*(__IO uint32_t*)addr) & 0x2FF00000/*0x2FFE0000*/ ) == 0x20000000)    



void _RunAppAddr(uint32_t ApplicationAddress );


void StartBootLoader(void);


void BootLoaderMenu(void);

//
#define         _BOOTLOAD_HEAD_STR                      "\r\n\r\n\
==============================================================\r\n\
=                                                            =\r\n\
=   In-Application Programming Application (Version 0.0.1)   =\r\n\
=                                                            =\r\n\
==============================================================\r\n\
\r\n\r\n\
========================== Main Menu =========================\r\n\r\n"

#define         _DWN_IMAGE_TO_INFLASH   " Download Image to the STM32 Internal Flash  =========== %d\r\n\r\n"
#define         _UPD_IMAGE_FR_INFLASH   " Upload Image From the STM32 Internal Flash  =========== %d\r\n\r\n"
#define         _DWN_PATCH_TO_INFLASH   " Download Patch to the STM32 Internal Flash  =========== %d\r\n\r\n"
#define         _UPD_PATCH_TO_INFLASH   " Bspatch  Patch to the STM32 Internal Flash  =========== %d\r\n\r\n"
#define         _EXECUTE_NEW_PROGRAM    " Execute The New Program  ================================== %d\r\n\r\n"
#define         _BOOTLOAD_TAIL_STR      "==============================================================\r\n\r\n"
#define         _INPUT_BOOTLOADER_STR   "\r\nPlease choose (%d~%d):"


static void dwn_image_to_inflash(void);
static void upd_image_to_inflash(void);
static void dwn_patch_to_inflash(void);
static void upd_patch_to_inflash(void);
static void execute_app(void);


static void WaitInputTimeout(void);
static void UsartRxTimer(void);






#define         _IS_DIGIT(c)            ((c) <= 0x39 && (c) >= 0x30 )



// flash addr
#define         _FLASH_BASE_ADDR           (0x08000000)
// bootloader 地址及大�?
#define         _BOOTLOADER_ADDR            _FLASH_BASE_ADDR
#define         _BOOTLOADER_SIZE           (1024*8)
// app 地址及大�?
#define         _APP_SIZE                  (1024*64)
#define         _APP_ADDR                  (_BOOTLOADER_ADDR+_BOOTLOADER_SIZE)

// image_magic
#define         _IMAGE_MAGIC_ADDR           ( _APP_ADDR + _APP_SIZE )
#define         _IMAGE_MAGIC_SIZE           (FLASH_PAGE_SIZE)
// image 
#define         _IMAGE_ADDR                 ( _IMAGE_MAGIC_ADDR + _IMAGE_MAGIC_SIZE )
#define         _IMAGE_SIZE                 (_APP_SIZE)

// patch_magic
#define         _PATCH_MAGIC_ADDR            ( _IMAGE_ADDR + _IMAGE_SIZE)
#define         _PATCH_MAGIC_SIZE            (FLASH_PAGE_SIZE)
// PATCH
#define         _PATCH_ADDR                  (_PATCH_MAGIC_ADDR+_PATCH_MAGIC_SIZE)
#define         _PATCH_SIZE                  (_APP_SIZE)

//
typedef void (*_keyHandler)(void);



#define         _FILE_LOCAL_IMAGE         (0x01)
#define         _FILE_LOCAL_PATCH         (0x02)

//
#define         _FILE_AUTO                (0x80)
#define         _FILE_MANUAL              (0x00)

#define         _MAGIC_SIZE               (8)

#define         _MAGIC_STR                 "STM_UPDA"

typedef struct
{
    uint8_t     file_magic[_MAGIC_SIZE];
    uint8_t     file_type ; // 文件类型
    uint32_t    file_size ; // 文件大小/
    uint8_t     file_md5[16] ;
}UpdataHeader , *pUpdataHeader;


uint8_t CreateUpImageMagic( uint32_t addr ,uint8_t t ,uint32_t size );

uint8_t ClearUpMagic(uint32_t addr );



//
uint8_t read_flash( uint32_t addr , uint32_t offset , uint8_t * , uint32_t size );
uint8_t write_flash( uint32_t addr , uint32_t offset , uint8_t * , uint32_t size );






#ifdef __cplusplus
}
#endif



#endif // __BOOTLOADER__H_
