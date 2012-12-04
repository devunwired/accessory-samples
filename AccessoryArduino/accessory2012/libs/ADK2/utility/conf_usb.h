/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __CONF_USB_H
#define __CONF_USB_H

#define DISABLED 0
#define ENABLED 1

#define TRACE_USB 1

#if TRACE_USB
#define LOG_STR(...)                    (dbgPrintf(__VA_ARGS__),dbgPrintf("\n\r"))
#define TRACE_OTG(fmt, ...)                  do{ dbgPrintf("%s %ld: " fmt, __func__, millis(), ##__VA_ARGS__); } while (0)
#define TRACE_OTG_NONL(fmt, ...)                  do{ dbgPrintf(fmt, ##__VA_ARGS__); } while (0)
#else
#define LOG_STR(...)                    ()
#define TRACE_OTG(fmt, ...)                  do{ } while (0)
#define TRACE_OTG_NONL(fmt, ...)                  do{ } while (0)
#endif

#define USB_HOST_FEATURE ENABLED
#define USB_DEVICE_FEATURE DISABLED

#define USB_HOST_PIPE_INTERRUPT_TRANSFER  ENABLE

#define CHIP_USB_NUMENDPOINTS 10

#define halt() do { for(;;); } while (0)
#define ASSERT(x) do { if (!(x)) { dbgPrintf("%s:%d ASSERT failed: %s\n", __FILE__, __LINE__, #x); halt(); } } while (0)
#define panic(fmt, ...) do{ dbgPrintf("panic at %s: " fmt, __func__, ##__VA_ARGS__); halt(); } while (0)

#undef printf
#define printf(x...) ERROR_DONT_USE_PRINTF

#endif

