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
 * @file        memset.c
 *
 * @brief       This file provides C library function memset.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>

/**
 ***********************************************************************************************************************
 * @brief           This function will set the content of memory to specified value.
 *
 * @param[out]      buff            Pointer to the start of the buffer.
 * @param[in]       val             The value to fill the buffer with
 * @param[in]       count           The size of the buffer.
 *
 * @return          The address of the buffer filled with specified value.
 ***********************************************************************************************************************
 */
void *memset(void *dst, int val, unsigned long count)
{
    unsigned char  *dst_buf;
    unsigned long  *aligned_addr;
    unsigned long   block_val;
    unsigned int    i;
    unsigned char   block_mask = BLOCK_SIZE - 1;

    dst_buf = (unsigned char *)dst;
    /* aligned buffer value set */
    if (count >= BLOCK_SIZE)
    {
        while ((unsigned long)dst_buf & block_mask)
        /* bytes set to make address alignment */
        {
            *dst_buf = val;
            dst_buf++;
            count--;
        }
        
        aligned_addr = (unsigned long*)dst_buf;
        block_val = 0;
        for (i = 0; i < BLOCK_SIZE/4; i++)
        {
            block_val = (block_val << 8) | (unsigned char)val;   
            block_val = (block_val << 8) | (unsigned char)val;   
            block_val = (block_val << 8) | (unsigned char)val;   
            block_val = (block_val << 8) | (unsigned char)val;   
        }
        while (count >= BLOCK_SIZE)
        {
            *aligned_addr = block_val;
            aligned_addr++;
            count -= BLOCK_SIZE;
        }
        dst_buf = (unsigned char*)aligned_addr;
    }
    /* set left bytes value */
    while (count--)
    {
        *dst_buf = val;
        dst_buf++;
    }

    return dst;
}

