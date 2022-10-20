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
 * @file        strrchr.c
 *
  * @brief       This file provides C library function strrchr.
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
 * @brief           This function will find the last occurance of C in S.
 *
 * @param[in]       str            Pointer to the start of the string.
 * @param[in]       ch             The value wait to find
 *
 * @return          The found string pointer.
 * @retval          OS_NULL         Not found.
 * @retval          else            found.
 ***********************************************************************************************************************
 */
char *strrchr(const char *str, int ch)
{
    const char *found, *p;

    ch = (unsigned char) ch;

    found = OS_NULL;
    while ((p = strchr(str, ch)) != OS_NULL)
    {
        found = p;
        str = p + 1;
    }

    return (char *) found;
}

