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
 * @file        strchr.c
 *
  * @brief       This file provides C library function strchr.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>
#include <os_stddef.h>


/**
 ***********************************************************************************************************************
 * @brief           This function will return the first occurrence of a char.
 *
 * @param[in]       str             The source string.
 * @param[in]       ch              The char to be found.
 *
 * @return          The first occurrence of a char(ch) in str.
 * @retval          OS_NULL         No found a char(ch) in str.
 * @retval          else            The first occurrence of a char(ch) in str.
 ***********************************************************************************************************************
 */
char *strchr(const char *str, int ch)
{
    ch = (unsigned char)ch;
    for ( ; (*str != ch) && (*str != '\0'); str++)
    {
        ;
    }

    if (*str == '\0')
    {
        return OS_NULL;
    }

    return (char *)str;
}
