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
 * @file        memcmp.c
 *
  * @brief       This file provides C library function memcmp.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>

/**
 ***********************************************************************************************************************
 * @brief           This function will compare two areas of memory.
 *
 * @param[in]       buf1           One area of memory.
 * @param[in]       buf1           Another area of memory.
 * @param[in]       count          The size of the area.
 *
 * @return          The compared result.
 * @retval          0               Two areas are equal.
 * @retval          else            Two areas are not equal.
 ***********************************************************************************************************************
 */
int memcmp(const void *buf1, const void *buf2, unsigned long count)
{
    const char *buf1_tmp;
    const char *buf2_tmp;
    const unsigned long *aligned_area1_tmp;
    const unsigned long *aligned_area2_tmp;
    unsigned int unaligned_bytes;
    unsigned int i;
    unsigned char block_mask = BLOCK_SIZE - 1;
    int       ret;
    unsigned long value;
    unsigned long value2;

    buf1_tmp = (const char *)buf1;
    buf2_tmp = (const char *)buf2;
    ret = 0;

    if (count >= BLOCK_SIZE)
    {
        while (((unsigned long)buf1_tmp & block_mask) && ((unsigned long)buf2_tmp & block_mask))
        {
            ret = *buf1_tmp - *buf2_tmp;
            if (ret)
            {
                return ret;
            }
            buf1_tmp++;
            buf2_tmp++;
            count--;
        }

        /* aligned buffer value move */
        if (((unsigned long)buf1_tmp & block_mask))
        {
            aligned_area2_tmp = (const unsigned long *)buf2_tmp;
            while (count >= BLOCK_SIZE)
            {
                value = 0;
                for (i = 0; i < BLOCK_SIZE/4; i++)
                {
                    value |= *buf1_tmp;
                    buf1_tmp++;
                    value |= *buf1_tmp << 8;
                    buf1_tmp++;
                    value |= *buf1_tmp << 16;
                    buf1_tmp++;
                    value |= *buf1_tmp << 24;
                    buf1_tmp++;
                }
                value2 = *aligned_area2_tmp;
                if (value != value2)
                {   
                    do
                    {
                        ret = (value & 7) - (value2 & 7);
                        value >>= 8;
                        value2 >>= 8;
                    }while (!ret);
                    
                    return ret;
                }
                aligned_area2_tmp++;
                count -= BLOCK_SIZE;
            }
            buf2_tmp = (const char *)aligned_area2_tmp; 
        }
        else if (((unsigned long)buf2_tmp & block_mask))
        {
            aligned_area1_tmp = (const unsigned long *)buf1_tmp;
            while (count >= BLOCK_SIZE)
            {
                value2 = 0;
                for (i = 0; i < BLOCK_SIZE/4; i++)
                {
                    value2 |= *buf2_tmp;
                    buf2_tmp++;
                    value2 |= *buf2_tmp << 8;
                    buf2_tmp++;
                    value2 |= *buf2_tmp << 16;
                    buf2_tmp++;
                    value2 |= *buf2_tmp << 24;
                    buf2_tmp++;
                }
                value = *aligned_area1_tmp;
                if (value != value2)
                {   
                    do
                    {
                        ret = (value & 7) - (value2 & 7);
                        value >>= 8;
                        value2 >>= 8;
                    }while (!ret);
                    
                    return ret;
                }
                aligned_area1_tmp++;
                count -= BLOCK_SIZE;
            }
            buf1_tmp = (const char *)aligned_area1_tmp; 
        }
        else 
        {
            aligned_area1_tmp = (const unsigned long *)buf1_tmp;
            aligned_area2_tmp = (const unsigned long *)buf2_tmp;
            while (count >= BLOCK_SIZE)
            {
                value = *aligned_area1_tmp;
                value2 = *aligned_area2_tmp;
                if (value != value2)
                {   
                    do
                    {
                        ret = (value & 7) - (value2 & 7);
                        value >>= 8;
                        value2 >>= 8;
                    }while (!ret);
                    
                    return ret;
                }
                aligned_area1_tmp++;
                aligned_area2_tmp++;
                count -= BLOCK_SIZE;
            }
            buf1_tmp = (const char *)aligned_area1_tmp;
            buf2_tmp = (const char *)aligned_area2_tmp;
        }
    }

    while ((count--) && (!ret))
    {
        ret = *buf1_tmp - *buf2_tmp;
        buf1_tmp++;
        buf2_tmp++;
    }

    return ret;
}

