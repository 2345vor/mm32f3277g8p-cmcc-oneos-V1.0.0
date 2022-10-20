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
 * @file        strncpy.c
 *
  * @brief       This file provides C library function strncpy.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>


/**
 ***********************************************************************************************************************
 * @brief           This function will copy string no more than count bytes.
 *
 * @param[out]      dst             The address of destination string where to copy the string to.
 * @param[in]       src             The address of source memory where to copy the string from.
 * @param[in]       count           The maximum copied length.
 *
 * @return          TThe address of destination string. 
 ***********************************************************************************************************************
 */
char *strncpy(char *dst, const char *src, unsigned long count)
{
    char *dst_tmp;
    
    
    for (dst_tmp = dst; (count > 0) && (*src != '\0'); count--)
    {
        *dst_tmp = *src;
        dst_tmp++;
        src++;
    }
    
    memset(dst_tmp, '\0', count);

    return dst;
}

