/*
 * libmm-wfd
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, YeJin Cho <cho.yejin@samsung.com>,
 * Seungbae Shin <seungbae.shin@samsung.com>, YoungHwan An <younghwan_.an@samsung.com>
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

#ifndef __MM_WFD_SINK_MANAGER_H__
#define __MM_WFD_SINK_MANAGER_H__

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WFD_SINK_MANAGER_LOCK(wfd_sink) \
	do {\
		if (wfd_sink) {\
			g_mutex_lock(&((wfd_sink)->manager_thread_mutex));\
		}\
	} while (0);

#define WFD_SINK_MANAGER_UNLOCK(wfd_sink) \
	do {\
		if (wfd_sink) {\
			g_mutex_unlock(&((wfd_sink)->manager_thread_mutex));\
		}\
	} while (0);

#define WFD_SINK_MANAGER_WAIT_CMD(wfd_sink) \
	do {\
		wfd_sink_debug("manager thread is waiting for command signal\n");\
		wfd_sink->waiting_cmd = TRUE; \
		g_cond_wait(&((wfd_sink)->manager_thread_cond), &((wfd_sink)->manager_thread_mutex)); \
		wfd_sink->waiting_cmd = FALSE; \
	} while (0);

#define WFD_SINK_MANAGER_APPEND_CMD(wfd_sink, cmd) \
	do {\
		WFD_SINK_MANAGER_LOCK(wfd_sink);\
		if (cmd == WFD_SINK_MANAGER_CMD_EXIT) {\
			g_list_free(wfd_sink->manager_thread_cmd);\
		}\
		wfd_sink->manager_thread_cmd = g_list_append(wfd_sink->manager_thread_cmd, GINT_TO_POINTER(cmd)); \
		WFD_SINK_MANAGER_UNLOCK(wfd_sink);\
	} while (0);

#define WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink) \
	do {\
		WFD_SINK_MANAGER_LOCK(wfd_sink);\
		if (wfd_sink->waiting_cmd) {\
			if (wfd_sink->manager_thread_cmd) {\
				wfd_sink_debug("send command signal to manager thread \n");\
				g_cond_signal(&((wfd_sink)->manager_thread_cond));\
			}\
		}\
		WFD_SINK_MANAGER_UNLOCK(wfd_sink);\
	} while (0);

/**
 * This function is to initialize manager
 *
 * @param[in]	handle		Handle of wfd_sink.
 * @return	This function returns zero on success, or negative value with errors.
 * @remarks
 * @see
 *
 */
int _mm_wfd_sink_init_manager(mm_wfd_sink_t *wfd_sink);
/**
 * This function is to release manager
 *
 * @param[in]	handle		Handle of wfd_sink.
 * @return	This function returns zero on success, or negative value with errors.
 * @remarks
 * @see
 *
 */
int _mm_wfd_sink_release_manager(mm_wfd_sink_t *wfd_sink);


#ifdef __cplusplus
}
#endif

#endif
