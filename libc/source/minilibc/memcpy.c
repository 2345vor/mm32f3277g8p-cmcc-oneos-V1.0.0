/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with 
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on 
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 *
 * @file        memcpy.c
 *
 * @brief       This file provides C library function memcpy.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>

/**
 ***********************************************************************************************************************
 * @brief           This function will copy memory content from source address to destination address.
 *
 * @param[out]      dst             The address of destination memory.
 * @param[in]       src             The address of source memory.
 * @param[in]       count           The copied length.
 *
 * @return          The address of destination memory.
 ***********************************************************************************************************************
 */
void *memcpy(void *dst, const void *src, unsigned long count)
{
    char                *dst_tmp;
    const char          *src_tmp;
    unsigned long        value;
    unsigned long       *aligned_dst;
    const unsigned long *aligned_src;
    unsigned char  block_mask = BLOCK_SIZE - 1;
    int i;

    dst_tmp = (char *)dst;
    src_tmp = (const char *)src;

    if (count >= BLOCK_SIZE)
    {
        while (((unsigned long)src_tmp & block_mask) && ((unsigned long)dst_tmp & block_mask))
        {
            *dst_tmp = *src_tmp;
            dst_tmp++;
            src_tmp++;
            count--;
        }

        /* aligned buffer value copy */
        if (((unsigned long)dst_tmp & block_mask))
        {
            aligned_src = (const unsigned long *)src_tmp;
            while (count >= BLOCK_SIZE)
            {
                value = *aligned_src;
                for (i = 0; i < BLOCK_SIZE/4; i++)
                {
                    *dst_tmp++ = value;
                    value = value >> 8;
                    *dst_tmp++ = value;
                    value = value >> 8;
                    *dst_tmp++ = value;
                    value = value >> 8;
                    *dst_tmp++ = value;
                    value = value >> 8;
                }
                aligned_src++;
                count -= BLOCK_SIZE;
            }
            src_tmp = (const char *)aligned_src; 
        }
        else if (((unsigned long)src_tmp & block_mask))
        {
            aligned_dst = (unsigned long *)dst_tmp;
            while (count >= BLOCK_SIZE)
            {
                value = 0;
                for (i = 0; i < BLOCK_SIZE/4; i++)
                {
                    // not support 64bit platform
                    value |= *src_tmp;
                    src_tmp++;
                    value |= *src_tmp << 8;
                    src_tmp++;
                    value |= *src_tmp << 16;
                    src_tmp++;
                    value |= *src_tmp << 24;
                    src_tmp++;
                }
                *aligned_dst = value;
                aligned_dst++;
                count -= BLOCK_SIZE;
            }
            dst_tmp = (char *)aligned_dst;
        }
        else 
        {
            aligned_src = (const unsigned long *)src_tmp;
            aligned_dst = (unsigned long *)dst_tmp;
            while (count >= BLOCK_SIZE)
            {
                *aligned_dst = *aligned_src;
                aligned_dst++;
                aligned_src++;
                count -= BLOCK_SIZE;
            }
            dst_tmp = (char *)aligned_dst;
            src_tmp = (const char *)aligned_src; 
        }
        
    }
    
    /* copy left bytes value */
    while (count--)
    {
        *dst_tmp = *src_tmp;
        dst_tmp++;
        src_tmp++;
    }

    return dst;
}
