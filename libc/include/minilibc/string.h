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
 * @file        string.h
 *
 * @brief       Provides function declarations of C library, only use for kernel.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-20   OneOS Team      First version.
 ***********************************************************************************************************************
 */
#ifndef __STRING_H__
#define __STRING_H__

#include <os_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE sizeof(unsigned long)

extern void         *memset(void *dst, int val, unsigned long count);
extern void         *memcpy(void *dst, const void *src, unsigned long count);
extern void         *memmove(void *dst, const void *src, unsigned long count);
extern int           memcmp(const void *buf1, const void *buf2, unsigned long count);
extern char         *strcpy(char *dst, const char *src);
extern char         *strncpy(char *dst, const char *src, unsigned long count);
extern int           strcmp(const char *str1, const char *str2);
extern int           strncmp(const char *str1, const char *str2, unsigned long count);
extern unsigned long strlen(const char *str);
extern unsigned long strnlen(const char *str, unsigned long max_len);
extern char         *strchr(const char *str, int ch);
extern char         *strstr(const char *str1, const char *str2);
extern char         *strdup(const char *str);
extern char         *strrchr(const char *str, int ch);
extern char         *strcat(char *dst, const char *src);




#ifdef __cplusplus
}
#endif

#endif /* __OS_STRING_H__ */


