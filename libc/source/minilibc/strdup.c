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
 * @file        strdup.c
 *
  * @brief       This file provides C library function strdup.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#ifdef OS_USING_SYS_HEAP
#include <os_memory.h>
#include <string.h>

/**
 ***********************************************************************************************************************
 * @brief           This function will duplicate a string.
 *
 * @param[in]       str             The string to be duplicated
 *
 * @return          The duplicated string pointer.
 * @retval          OS_NULL         Duplicate string failed.
 * @retval          else            Duplicate string success.
 ***********************************************************************************************************************
 */
char *strdup(const char *str)
{
    char      *str_tmp;

    str_tmp = (char *)os_malloc(strlen(str) + 1);
    if (str_tmp)
    {
        strcpy(str_tmp, str);
    }

    return str_tmp;
}

#endif /* OS_USING_SYS_HEAP */

