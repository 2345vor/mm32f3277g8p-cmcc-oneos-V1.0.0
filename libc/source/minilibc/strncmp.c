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
 * @file        strncmp.c
 *
  * @brief       This file provides C library function strncmp.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>

/**
 ***********************************************************************************************************************
 * @brief           This function will compare two strings with specified length.
 *
 * @param[in]       str1            One string to be compared.
 * @param[in]       str2            Another string to be compared.
 * @param[in]       count           The maximum compare length.
 *
 * @return          The result.
 * @retval          0               Two strings are equal.
 * @retval          else            Two strings are not equal.
 ***********************************************************************************************************************
 */
int strncmp(const char *str1, const char *str2, unsigned long count)
{
    unsigned char value1;
    unsigned char value2;

    for ( ; count > 0; str1++, str2++, count--)
    {
        if ((*str1 != *str2) || (*str1 == '\0'))
        {
            value1 = *((const unsigned char *)str1);
            value2 = *((const unsigned char *)str2);

            return (value1 - value2);
        }
    }

    return 0;
}

