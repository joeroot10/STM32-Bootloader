/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "main.h"
#include "ymodem.h"
#include "inflash.h"

#include "usrUsart.h"
#include "md5.h"
//#include "minilzo.h"
#include "inflash.h"
#include "bspatch.h"



static INT32 offtin(u_char *buf)
{
	INT32 y;

    y = buf[3] & 0x7F ;
    y = ( y << 8 ) | buf[2] ;
    y = ( y << 8 ) | buf[1] ;
    y = ( y << 8 ) | buf[0] ;

    if( buf[3] & 0x80 ) y = -y ;

	return y;
}



//030005020103
static UINT32 RLEUnCompress( u_char *in , UINT32 inlen , u_char *out , UINT32 outsize )
{
	UINT32 len = 0 , pos = 0;
	UINT8 rle = 0 ;
	if( !in || !out || !inlen || !outsize ) return 0;
	while( pos + 1 < inlen )
	{
		rle = in[pos++] ;	// 长度
		memset( out + len , in[pos++] , rle );
		len += rle ;
	}
	return len ;
}






#define         _CTRL_BLOCK_SIZE           (4*3)    // 一次读取的控制块大小

#define         _DATA_BLOCL_SIZE            (1024*8)   



static uint8_t   gCtrl_Block[_CTRL_BLOCK_SIZE] = {0x00};
static uint32_t  gCtrl_Read_Totle_len = 0 ; // Ctrl Block 已经读取的总长度

static uint8_t   gDiff_Block[_DATA_BLOCL_SIZE] = {0x00};    // 解压后放的数据
static uint8_t   gOld_Block[_DATA_BLOCL_SIZE] = {0x00};
static uint8_t   gNew_Block[_DATA_BLOCL_SIZE] = {0x00};
static uint8_t   gCompress_Block[_DATA_BLOCL_SIZE] = {0x00};
static uint8_t   gDecompress_Blcok[_DATA_BLOCL_SIZE] = {0x00};


#if 0
// miniLZO 文件解压
static uint8_t bspatch_read_diff( uint32_t diff_addr , uint32_t diff_size ,
            int32_t offset , uint8_t *pout , uint32_t outsize )
{
    UINT32 read_len = 0 , tmp = 0  , i = 0 ;
	UINT32 read_offset = 0x00 , outlen = 0  ;
	//
	lzo_uint comlen = 0 , decomlen = 0 ;

    UINT8 head[4] = {0x00};
    if (lzo_init() != LZO_E_OK)
    {
        return 1 ;
    }

    while( read_len + 4  < diff_size )
    {
        if( 0 != read_flash( diff_addr, read_len , head , 4 ) )
        {
            rt_kprintf("Read diff head error:%x-%d\r\n" , diff_addr , read_len );
            return 1 ;
        }
        decomlen = ( head[0] << 8 ) | head[1] ; // 解压后的数据长度
        comlen = ( head[2] << 8 ) | head[3] ;   // 压缩后的数据长度
        //
        read_offset += decomlen ;
        if( read_offset >= offset ) // 找到
        {
            // 读取数据
            if( 0 != read_flash( diff_addr, read_len + 4 , gCompress_Block , comlen ) )
            {
                rt_kprintf("Read diff compress error:%x-%d-%d\r\n" , diff_addr , read_len + 4 ,comlen);
                return 1 ;
            }
            // 解压 [此处一定要保存 decomlen < sizeof(gDecompress_Blcok) ]
            if( LZO_E_OK != lzo1x_decompress( gCompress_Block,comlen ,gDecompress_Blcok,&decomlen,NULL) )
            {
                rt_kprintf("decompress diff error:%x-%d-%d\r\n" , diff_addr , read_len + 4 ,comlen);
                return 1 ;
            }
            //
            memcpy( pout + outlen , gDecompress_Blcok + ( offset -( read_offset - decomlen ) ) , read_offset - offset );
			outlen += ( read_offset - offset );
            //
            if( outlen >= outsize ) return 0 ;

            // 准备读取下一包数据
            read_len += ( comlen + 4 );
            
            while( read_len + 4  < diff_size )
            {
                if( 0 != read_flash( diff_addr, read_len , head , 4 ) )
                {
                    rt_kprintf("2Read diff head error:%x-%d\r\n" , diff_addr , read_len );
                    return 1 ;
                }
                decomlen = ( head[0] << 8 ) | head[1] ; // 解压后的数据长度
                comlen = ( head[2] << 8 ) | head[3] ;   // 压缩后的数据长度
                // 读取之
                if( 0 != read_flash( diff_addr, read_len + 4 , gCompress_Block , comlen ) )
                {
                    rt_kprintf("2Read diff compress error:%x-%d-%d\r\n" , diff_addr , read_len + 4 ,comlen);
                    return 1 ;
                }
                // 解压之
                if( LZO_E_OK == lzo1x_decompress(gCompress_Block,comlen ,gDecompress_Blcok,&decomlen,NULL) )
				{
					if( decomlen + outlen < outsize )
					{
						memcpy( pout + outlen , gDecompress_Blcok , decomlen );
						outlen += decomlen ;
					}
					else
					{
						memcpy( pout + outlen , gDecompress_Blcok , outsize - outlen );
						outlen += ( outsize - outlen );
						return 0 ;
					}
				}
                read_len += ( comlen + 4 );
            }
        }
        else read_len += ( comlen + 4 );
    }
    return 1 ;
}

#else
// RLE 文件解压
static uint8_t bspatch_read_diff( uint32_t diff_addr , uint32_t diff_size ,
            int32_t offset , uint8_t *pout , uint32_t outsize )
{
	uint32_t read_len = 0 , tmp = 0  , i = 0 ;
	uint32_t read_offset = 0x00 , outlen = 0  ;

    //if( !pout ) return 0 ;

	while( read_len < diff_size )
	{
		tmp = diff_size - read_len > _DATA_BLOCL_SIZE ? 
            _DATA_BLOCL_SIZE : diff_size - read_len ;

        if( 0 != read_flash( diff_addr, read_len , gCompress_Block , tmp ) ) // 从0开始 
        {
            rt_kprintf("Read diff error:%x-%d-%d\r\n" , diff_addr , read_len , tmp );
            return 1 ;
        }        
		read_len += tmp ;
		// 解压
		for( i = 0 ; i < tmp ; i+= 2 )
		{
			read_offset += gCompress_Block[i] ;		// 只是长度
			if( read_offset >= offset )
			{
				//printf("Diff Find offset:%d-%d\r\n" , offset , read_offset ) ;

				memset( pout + outlen , gCompress_Block[i+1] , read_offset - offset );	// 最多255,所以不存在超的可能
				outlen += ( read_offset - offset ) ;

				//printf("diff-read1:%d[%d]\r\n" , outlen ,outsize );

				// 把余下的数据处理掉
				for( i += 2 ; i < tmp && outlen + gCompress_Block[i+0] <= outsize ; i += 2 )
				{
					memset( pout + outlen , gCompress_Block[i+1] , gCompress_Block[i+0] );
					outlen += gCompress_Block[i+0] ;
				}
				//
				if( i < tmp && outlen < outsize )	// 肯定是 outlen + gCompress_Block[i+0] > outsize 
				{
					//printf("===i:%d,tmp:%d,outlen:%d,outsize:%d,gCompress_Block[i+0]:%d\r\n",
					//	i , tmp , outlen , outsize ,gCompress_Block[i+0] );
					memset( pout + outlen , gCompress_Block[i+1] , outsize - outlen );
					outlen = outsize ;					
				}
				if( outlen >= outsize ) return 0 ;

				while( read_len < diff_size && outlen < outsize )
				{
					tmp = diff_size - read_len > _DATA_BLOCL_SIZE ? _DATA_BLOCL_SIZE : diff_size - read_len ;
					//memcpy( gCompress_Block , pdiff + read_len , tmp );		// 从0开始
					if( 0 != read_flash( diff_addr, read_len , gCompress_Block , tmp ) ) 
                    {
                        rt_kprintf("Read diff error:%x-%d-%d\r\n" , diff_addr , read_len , tmp );
                        return 1 ;
                    }
                    
                    read_len += tmp ;

					for( i = 0 ; i < tmp && outlen + gCompress_Block[i+0] <= outsize ; i+= 2 )
					{
						memset( pout + outlen , gCompress_Block[i+1] , gCompress_Block[i+0] );
						outlen += gCompress_Block[i+0] ;
					}
					//printf("i:%d,tmp:%d,outlen:%d,outsize:%d,l:%d\r\n" ,
					//	i , tmp , outlen , outsize , gCompress_Block[i+0] ) ;
					if( i < tmp && outlen < outsize )
					{
						memset( pout + outlen , gCompress_Block[i+1] , outsize - outlen );
						outlen = outsize ;
					}
					//
					if( outlen >= outsize ) return 0 ;
				}
				return 0 ;
			}
		}
	}
	return 1 ;
}
#endif // 




/*
    通过 old 文件, diff 文件 生成 new 文件

    new_addr : 新文件启始地址, new_addr_offset 新文件偏移
    diff_addr : diff 启始地址 , diff_addr_offset diff 偏移
    old_addr : 旧文件启始地址 , old_addr_offset old 偏移
    ctrl_len : 要处理的new/diff 文件大小
*/
static uint8_t Bspatch_Read_diff_string(
    uint32_t new_addr /*pnew*/ , int32_t new_addr_offset /*newpos*/,
    uint32_t diff_addr /*pbb*/ , int32_t diff_size /*bzdatalen*/ ,int32_t diff_addr_offset /*diffpos*/,
    uint32_t old_addr /*pold*/, uint32_t old_size /*oldsize*/ ,int32_t old_addr_offset /*oldpos*/,
    int32_t ctrl_len /*ctrl[0]*/)
#if 1
{
   uint32_t read_len = 0 , tmp_len = 0  , i = 0 ;
    uint32_t read_old_len = 0 , read_old_len_t = 0 , old_tmp = 0 ;
    //
    read_old_len_t = old_size - old_addr_offset > ctrl_len ? 
        ctrl_len : old_size - old_addr_offset ;
    //
    
    while( read_len < ctrl_len )
    {
        tmp_len = ctrl_len - read_len >  _DATA_BLOCL_SIZE ?
            _DATA_BLOCL_SIZE : ctrl_len - read_len ;

        // read old file
        old_tmp = 0x00 ;		
		if(  read_old_len < read_old_len_t )
		{
			old_tmp = read_old_len_t - read_old_len > tmp_len ? 
				tmp_len : read_old_len_t - read_old_len ;
            if( 0 != read_flash( old_addr , old_addr_offset + read_old_len , 
                    gOld_Block , old_tmp ) )
            {
                rt_kprintf("Read Old file error :%x-%d\r\n" ,
                    old_addr_offset + read_len ,tmp_len );
                return 1 ;
            }            
			//memcpy( gOld_Block , pold_addr + old_offset + read_old_len , old_tmp ) ;		// 获取old文件
			read_old_len += old_tmp  ;
		}
        // diff 文件[需要解压缩]
        if( 0 != bspatch_read_diff( diff_addr, diff_size ,
            diff_addr_offset + read_len, gDiff_Block , tmp_len ) )
        {
            rt_kprintf("uncompress diff file error :%x-%d-%d-%d\r\n",
                diff_addr , diff_size ,diff_addr_offset + read_len ,tmp_len );
            return 1 ;
        }

        for( i = 0 ; i < tmp_len ; i++ )
		{
			if( /*diff_addr_offset + i + read_len >= 0 && 
                diff_addr_offset + i + read_len < diff_size*/ i < old_tmp )
			{
				gNew_Block[ i ] = gOld_Block[ i ] + gDiff_Block[ i ] ;
			}
			else
			{
				gNew_Block[ i ] = gDiff_Block[ i ];
			}
		}
        // write new file
        // Flash 好像有对地址必须为4or8的整数倍
        if( 0 != write_flash( new_addr , new_addr_offset + read_len ,
            gNew_Block , tmp_len ) )
        {
            rt_kprintf("Write New File error:%x-%d\r\n" , 
                new_addr_offset + read_len  , tmp_len );
            return 1 ;
        } 
        read_len += tmp_len ;
        
    }

    return 0 ;
}
#else
{
    uint32_t read_len = 0x00 ;
    uint32_t tmp_len = 0 ;
    uint32_t i = 0 ;
    uint32_t new_len = 0 ;
    
    // 一次读取不了那么多的Ctrl[0]
    while( read_len < ctrl_len )
    {
        tmp_len = ctrl_len - read_len > _DATA_BLOCL_SIZE ? _DATA_BLOCL_SIZE : ctrl_len - read_len ;
        // old 文件
        if( 0 != read_flash( old_addr , old_addr_offset + read_len , gOld_Block , tmp_len ) )
        {
            rt_kprintf("Read Old file error :%x-%d\r\n" ,
                old_addr_offset + read_len ,tmp_len );
            return 1 ;
        }
        // diff 文件[需要解压缩]
        if( 0 != bspatch_read_diff( diff_addr, diff_size ,
            diff_addr_offset + read_len, gDiff_Block , tmp_len ) )
        {
            rt_kprintf("uncompress diff file error :%x-%d-%d-%d\r\n",
                diff_addr , diff_size ,diff_addr_offset + read_len ,tmp_len );
            return 1 ;
        }
        //
        for( i = 0 ; i < tmp_len ; i++ )
        {
            if (( old_addr_offset + read_len + i >= 0 ) && 
                ( old_addr_offset + read_len + i < old_size ))
			{
				gNew_Block[i] = gOld_Block[i] + gDiff_Block[ i ];
			}
			else
			{
				gNew_Block[i] = gDiff_Block[ i ];
			}			
        }
        // 写入 new file
        if( 0 != write_flash( new_addr , new_addr_offset + read_len ,
            gNew_Block , tmp_len ) )
        {
            rt_kprintf("Write New File error:%x-%d\r\n" , 
                new_addr_offset + read_len  , tmp_len );
            return 1 ;
        }        
        read_len += tmp_len ;
    }

    return 0 ;
}
#endif //


/*
    将 exfile 写入 newfile中
*/
static uint8_t Bspatch_Read_extra_string( 
        uint32_t new_file_addr , uint32_t new_offset_addr ,
        uint32_t ex_file_addr , uint32_t ex_offset_addr , uint32_t size )
{
    uint32_t tmp = 0 , read_len = 0 ;
    while( read_len < size )
    {
        tmp = size - read_len > sizeof(gNew_Block) ? sizeof(gNew_Block) : size - read_len ;

        read_flash( ex_file_addr, read_len + ex_offset_addr, gNew_Block, tmp ) ;

        write_flash( new_file_addr, read_len + new_offset_addr , gNew_Block, tmp );

        read_len += tmp ;
    }
    return 0 ;
}

/*
File format:
	0	8	"BSDIFF40"
	8	4	X
	12	4	Y
	16	4	sizeof(newfile)
	
	20	X	bzip2(control block)
	20+X	Y	bzip2(diff block)
	20+X+Y	???	bzip2(extra block)
with control block a set of triples (x,y,z) meaning "add x bytes
from oldfile to x bytes from the diff block; copy y bytes from the
extra block; seek forwards in oldfile by z bytes".
*/
// 返回新文件的大小
int32_t bspatch( uint32_t old_file_addr , uint32_t new_file_addr ,
            uint32_t patch_file_addr )
{
    //// magic[8] + ctrlen[4] + bzdatalen[4] + 
    // newfilesize[4] + oldfilesize[4] + extrelen[4] + new file md5[16] + old file md5[16]
    uint8_t header[24+16+16] = {0x00}; 
    uint32_t bzctrllen = 0 ,bzdatalen = 0 , newsize = 0 , oldsize = 0 ;
    int32_t newpos = 0 , diffpos = 0 , oldpos = 0 , expos = 0  ;
    int32_t ctrl[3] = {0x00};

    uint32_t read_len = 0 ;
    // read header
    if( 0 != read_flash( patch_file_addr , 0 , header , sizeof( header ) ) )
    {
        rt_kprintf("read header error--%x\r\n" , patch_file_addr );
        return _PATCH_READ_ERROR ;
    }
    /* Check for appropriate magic */
	if (memcmp(header, "BSDIFF40", 8) != 0)
	{
        rt_kprintf("magic error\r\n");
        return _PATCH_FORMAT_ERROR ;
	}
    bzctrllen=offtin(header+8); // 控制块大小
	bzdatalen=offtin(header+12); // diff块大小
	newsize=offtin(header+16); // 新文件大小
	oldsize=offtin(header+20);

    // check old file md5

    {
        uint8_t md5[16] = {0x00};
        md5_flash( old_file_addr, oldsize , md5 );
        if( 0 != memcmp(  md5 , header + 24 + 16 , 16 ) )
        {
            rt_kprintf("old file md5 check error\r\n");
            return _PATCH_OLD_FILE_MD5_ERROR;
        }
    }


    //rt_kprintf("ctrl size:%d,diff size:%d,new size:%d\r\n", bzctrllen , bzdatalen , newsize );

    gCtrl_Read_Totle_len = 0x00 ;
    while( newpos < newsize )
    {
        // 每次读取3个
        read_len = bzctrllen  - sizeof(gCtrl_Block) > sizeof(gCtrl_Block) ? 
            sizeof(gCtrl_Block) : bzctrllen  - gCtrl_Read_Totle_len ;
        
        if( 0 != read_flash( patch_file_addr + sizeof(header)  , gCtrl_Read_Totle_len ,
            gCtrl_Block , read_len ) )
        {
            rt_kprintf("Read Ctrl Block Error:%d-%d\r\n", gCtrl_Read_Totle_len , read_len );
            return _PATCH_DATA_ERROR ;
        }
        gCtrl_Read_Totle_len += read_len ;
        //
        ctrl[0] = offtin( gCtrl_Block + 0 );
        ctrl[1] = offtin( gCtrl_Block + 4 );
        ctrl[2] = offtin( gCtrl_Block + 8 );

        //rt_kprintf("x:%d,y:%d,z:%d\r\n" , ctrl[0] , ctrl[1] , ctrl[2] );
        
        if( newpos+ctrl[0]>newsize )
        {
            rt_kprintf("0-Corrupt patch-%d,%d-%d\r\n" ,newpos , ctrl[0] , newsize );
            return _PATCH_DATA_ERROR ;
        }
        // 更新new数据
        // oldsize 文件大小
        if( 0 != Bspatch_Read_diff_string( new_file_addr , newpos ,
            patch_file_addr + sizeof(header) + bzctrllen , bzdatalen ,diffpos ,
            old_file_addr , oldsize , oldpos , ctrl[0] ) )
        {
            rt_kprintf("bspatch read diff string error\r\n");
            return _PATCH_READ_ERROR ;
        }

        diffpos += ctrl[0];
        newpos += ctrl[0];
        oldpos += ctrl[0];

        if (newpos + ctrl[1] > newsize)
		{
            rt_kprintf("1-Corrupt patch-%d,%d-%d\r\n" , newpos , ctrl[1] , newsize);
            return _PATCH_DATA_ERROR ;
		}
        /* Read extra string */
        if( 0 != Bspatch_Read_extra_string( new_file_addr , newpos ,
            patch_file_addr + sizeof(header) + bzctrllen + bzdatalen ,
            expos , ctrl[1] ) )
        {
            rt_kprintf("bspatch read extra string error\r\n");
            return _PATCH_DATA_ERROR ;
        }
        expos += ctrl[1] ;

        /* Adjust pointers */
		newpos += ctrl[1];
		oldpos += ctrl[2];

        // 进度
        rt_kprintf(".");        
    }

    // check new file md5
    {
        uint8_t new_md5[16] = { 0x00 };
        md5_flash( new_file_addr, newsize, new_md5 );
        if( 0 != memcmp( new_md5 , header + 24 , 16 ) )
        {
            rt_kprintf("\r\nnew file md5 error\r\n");
            return _PATCH_NEW_FILE_MD5_ERROR ;
        }
    }
    
    return newsize ;
}

/*
....
*/

static uint32_t mem_cmp( uint8_t * ps , uint32_t sl , uint8_t *pd , uint32_t dl )
{
    uint32_t l = 0 ;
    for( l = 0 ; l < sl && l < dl ; l++ )
    {
        if( ps[l] != pd[l] ) break ;
    }
    return l ;
}

/*
    返回有多少个字节相同
*/
uint32_t compare_flash( uint32_t saddr , uint32_t daddr , uint32_t size )
{
    uint32_t read_len = 0 , tmp = 0 , cmp_l = 0 , cmp_t = 0 ;
    while( read_len < size )
    {
        tmp = size - read_len > _DATA_BLOCL_SIZE ? _DATA_BLOCL_SIZE : size - read_len ;        
        //
        if( 0 != read_flash( saddr, read_len, gOld_Block, tmp ) )
        {
            rt_kprintf("Read sfalsh error:%x-%x-%d\r\n", saddr , read_len , tmp );
            return read_len ;
        }
        if( 0 != read_flash( daddr, read_len, gNew_Block, tmp ) )
        {
            rt_kprintf("Read dfalsh error:%x-%x-%d\r\n", daddr , read_len , tmp );
            return read_len ;
        }
        read_len += tmp ;
        cmp_l = mem_cmp( gOld_Block , tmp , gNew_Block , tmp ) ;
        cmp_t += cmp_l ;
        
        if( cmp_l != tmp ) break ;
    }

    return cmp_t ;
}


static UINT8 md5_flash( uint32_t addr , uint32_t file_size , uint8_t md5[16] )
{
    uint8_t buff[512] = {0x00};     //超过256计算会出错 
    uint32_t rlen = 0 ,totle_len = 0 ;
    applib_md5_ctx md5_context = {0x00};

    applib_md5_init( &md5_context );
    
    while( totle_len < file_size  )
    {
        rlen = file_size - totle_len > sizeof(buff) ? sizeof(buff) : file_size - totle_len ;

        if( 0 != read_flash( addr , totle_len , buff , rlen ) ) return 1 ;

        applib_md5_update( &md5_context , buff , rlen ) ;

        totle_len += rlen ;
    }
    applib_md5_final( md5 , &md5_context );

    return 0 ;
}





