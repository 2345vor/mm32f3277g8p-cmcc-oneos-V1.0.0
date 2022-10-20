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
 * @file        strstr.c
 *
  * @brief       This file provides C library function strstr.
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
 * @brief           This function will return the first occurrence of a string.
 *
 * @param[in]       str1            The source string.
 * @param[in]       str2            The string to be found.
 *
 * @return          The first occurrence of a str2 in str1.
 * @retval          OS_NULL         No found str2 in str1.
 * @retval          else            The first occurrence of a str2 in str1.
 ***********************************************************************************************************************
 */
char *strstr(const char *str1, const char *str2)
{
    unsigned long str1_len;
    unsigned long str2_len;

    if (str2[0] == '\0')
    {
        return (char *)str1;
    }
    
    str1_len = strlen(str1);
    str2_len = strlen(str2);
    while (str1_len >= str2_len)
    {
        if (!memcmp(str1, str2, str2_len))
        {
            return (char *)str1;
        }
        
        str1++;
        str1_len--;
    }
    
    return OS_NULL;
}

