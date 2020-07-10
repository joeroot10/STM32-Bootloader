/*********************************************
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
**************************************************/



#ifndef         __INFLASH__H__

#define         __INFLASH__H__

#ifdef __cplusplus
 extern "C" {
#endif

void inflash_init(void);
void dwn_image_inflash(void);       // 下载
void up_image_inflash(void);
void dwn_patch_inflash(void);
void up_patch_inflash(void);

uint8_t inflash_writes( uint32_t addr , uint8_t *in , uint32_t size);
uint8_t inflash_erases(uint32_t addr , uint32_t size);
uint32_t inflash_reads(uint32_t Addr , uint8_t *pOutBuff , uint32_t len);

static unsigned char ymode_inflash( pYmodemMSG );

static HAL_StatusTypeDef inflash_erase( uint32_t pageAddr ,  uint32_t size );
static HAL_StatusTypeDef inflash_write( uint32_t Addr , uint8_t *pInBuff , uint32_t size );
static uint32_t inflash_read( uint32_t Addr , uint8_t *pOutBuff , uint32_t len );



#ifdef __cplusplus
}
#endif

#endif // __INFLASH__H__

