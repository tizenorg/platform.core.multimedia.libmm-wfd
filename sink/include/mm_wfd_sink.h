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

#ifndef _MM_WFD_SINK_H_
#define _MM_WFD_SINK_H_

#include <string.h>
#include <glib.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>

/**
 *  * Enumerations of wifi-display state.
 *   */
typedef enum {
	MM_WFD_SINK_STATE_NULL,				/**< wifi-display is created, but not realized yet */
	MM_WFD_SINK_STATE_READY,			/**< wifi-display is ready to play media */
	MM_WFD_SINK_STATE_PLAYING,			/**< wifi-display is now playing media */
	MM_WFD_SINK_STATE_PAUSED,			/**< wifi-display is paused while playing media */
	MM_WFD_SINK_STATE_NONE,				/**< wifi-display is not created yet */
	MM_WFD_SINK_STATE_NUM,				/**< Number of wifi-display states */
} MMWfdSinkStateType;


int mm_wfd_sink_create(MMHandleType *wfd_sink, const char *uri);
int mm_wfd_sink_start(MMHandleType wfd_sink);
int mm_wfd_sink_stop(MMHandleType wfd_sink);
int mm_wfd_sink_unrealize(MMHandleType wfd_sink);
int mm_wfd_sink_destroy(MMHandleType wfd_sink);


#endif

