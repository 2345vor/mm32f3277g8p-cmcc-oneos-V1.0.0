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
 * @file        strlen.c
 *
  * @brief       This file provides C library function strlen.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>


/**
 ***********************************************************************************************************************
 * @brief           This function will return the length of a string, which terminate will '\0' character.
 *
 * @param[in]       str             The string.
 *
 * @return          The length of string.
 ***********************************************************************************************************************
 */
unsigned long strlen(const char *str)
{
    const char *str_tmp;

    for (str_tmp = str; *str_tmp != '\0'; str_tmp++)
    {
        ;
    }
    
    return (str_tmp - str);
}

