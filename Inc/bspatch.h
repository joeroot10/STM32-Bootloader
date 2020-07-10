/**************************************************
**
**
**
**
**
**
**
**
**
*************************************************************/




#ifndef         __BSPATCH__H_

#define         __BSPATCH__H_ 


#ifdef __cplusplus
 extern "C" {
#endif


typedef unsigned int UINT32 ;
typedef signed int INT32 ;
typedef unsigned short UINT16 ;
typedef signed short INT16 ;
typedef unsigned char UINT8 ;
typedef unsigned char u_char ;




/*
#define         INTERNAL_FLASH      0x01
#define         EXTERNAL_FLASH      0x02

#define         CHECK_INTERNAL_FLASH(x)           ((x) & (0x08000000))
*/

/*
> 0 ,�ɹ�,���ļ���С

-1  :������Ч��patch�ļ�
-1  :old file md5ʧ��
-2  :new file md5 ʧ��
-3  :����
*/

#define         _PATCH_FORMAT_ERROR                 (-1)    // ������Ч���ļ�
#define         _PATCH_DATA_ERROR                   (-2)    // �ļ����ݴ���
#define         _PATCH_OLD_FILE_MD5_ERROR           (-3)    // ���ļ�md5ʧ��
#define         _PATCH_NEW_FILE_MD5_ERROR           (-4)    // ���ļ�md5ʧ��
#define         _PATCH_READ_ERROR                   (-5)    // ��ȡ�ļ�ʧ��
#define         _PATCH_UNKNOWN_ERROR                (-6)    // 

int32_t bspatch( uint32_t old_file_addr , uint32_t new_file_addr , uint32_t patch_file_addr );

static INT32 offtin(u_char *buf);
static UINT32 RLEUnCompress( u_char *in , UINT32 inlen , u_char *out , UINT32 outsize );

uint32_t compare_flash( uint32_t saddr , uint32_t daddr , uint32_t size );

static UINT8 md5_flash( uint32_t addr , uint32_t size , uint8_t md5[16] );




#ifdef __cplusplus
 }
#endif




#endif //








