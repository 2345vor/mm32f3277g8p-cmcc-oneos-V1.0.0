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
 * @file        console.h
 *
 * @brief       Define the macro for print.
 *
 * @revision
 * Date         Author          Notes
 * 2020-09-16   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

extern void        os_kprintf(const char *fmt, ...);

#ifdef __cplusplus
extern "C" {
#endif

#define console_printf(fmt, ...)   os_kprintf(fmt"\r\n", ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
