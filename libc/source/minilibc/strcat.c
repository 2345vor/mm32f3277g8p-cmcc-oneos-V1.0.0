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
 * @file        strcat.c
 *
  * @brief       This file provides C library function strcat.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */

#include <string.h>

/**
 ***********************************************************************************************************************
 * @brief           This function will catch the src string to dst.
 *
 * @param[in]       dst            Pointer to the start of the dst string.
 * @param[in]       src            Pointer to the start of the src string.
 *
 * @return          Start pointer of caught string.
 * @retval          pointer
 ***********************************************************************************************************************
 */
char *strcat(char *dst, const char *src)
{
  strcpy(dst + strlen(dst), src);

  return dst;
}

