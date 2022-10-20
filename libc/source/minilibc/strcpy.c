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
 * @file        strcpy.c
 *
  * @brief       This file provides C library function strcpy.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>


/**
 ***********************************************************************************************************************
 * @brief           Copy a '\0' terminated string.
 *
 * @param[out]      dst             The address of destination string where to copy the string to.
 * @param[in]       src             The address of source memory where to copy the string from.
 *
 * @return          TThe address of destination string. 
 ***********************************************************************************************************************
 */
char *strcpy(char *dst, const char *src)
{
    char *dst_tmp;

    dst_tmp = dst;
    
	while ((*dst_tmp = *src) != '\0') {
		src++;
		dst_tmp++;
	}

    return dst;
}

