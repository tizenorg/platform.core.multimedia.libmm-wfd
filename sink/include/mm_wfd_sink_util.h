/*
 * libmm-wfd
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, ByungWook Jang <bw.jang@samsung.com>,
 * Maksym Ukhanov <m.ukhanov@samsung.com>, Hyunjun Ko <zzoon.ko@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _MM_WFD_SINK_UTIL_H_
#define _MM_WFD_SINK_UTIL_H_

#include <string.h>
#include <glib.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>

#include "mm_wfd_sink.h"

#define SAFE_FREE(src)      { if(src) {free(src); src = NULL;}}

gboolean _mm_wfd_sink_dump_pipeline_state(mm_wfd_sink_t *wfd_sink);

#endif

