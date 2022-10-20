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
 * @file        strnlen.c
 *
  * @brief       This file provides C library function strnlen.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>


/**
 ***********************************************************************************************************************
 * @brief           The os_strnlen() function returns the number of characters in the string pointed to by str, 
 *                  excluding the terminating null byte ('\0'), but at most maxlen. In doing this, os_strnlen() looks 
 *                  only at the first maxlen characters in the string pointed to by str and never beyond (str + max_len).
 *
 * @param[in]       str             The string.
 * @param[in]       max_len         The max size.
 *
 * @return          The length of string.
 ***********************************************************************************************************************
 */
unsigned long strnlen(const char *str, unsigned long max_len)
{
    const char *str_tmp;
    const char *end_str;

    
    end_str = str + max_len;
    for (str_tmp = str; (*str_tmp != '\0') && (str_tmp < end_str); str_tmp++)
    {
        ;
    }
    
    return (str_tmp - str);
}

