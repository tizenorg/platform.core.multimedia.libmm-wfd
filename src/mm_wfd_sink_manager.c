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


#include "mm_wfd_sink_manager.h"


static gpointer __mm_wfd_sink_manager_thread(gpointer data);

int _mm_wfd_sink_init_manager(mm_wfd_sink_t *wfd_sink)
{
	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* create manager mutex */
	g_mutex_init(&(wfd_sink->manager_thread_mutex));

	/* create capture cond */
	g_cond_init(&(wfd_sink->manager_thread_cond));

	wfd_sink->manager_thread_cmd = NULL;
	wfd_sink->manager_thread_exit = FALSE;

	/* create manager thread */
	wfd_sink->manager_thread =
	    g_thread_new("__mm_wfd_sink_manager_thread", __mm_wfd_sink_manager_thread, (gpointer)wfd_sink);
	if (wfd_sink->manager_thread == NULL) {
		wfd_sink_error("failed to create manager thread\n");
		goto failed_to_init;
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

failed_to_init:
	g_mutex_clear(&(wfd_sink->manager_thread_mutex));
	g_cond_clear(&(wfd_sink->manager_thread_cond));

	return MM_ERROR_WFD_INTERNAL;
}

int _mm_wfd_sink_release_manager(mm_wfd_sink_t *wfd_sink)
{
	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* release manager thread */
	if (wfd_sink->manager_thread) {
		WFD_SINK_MANAGER_APPEND_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_EXIT);
		WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink);

		wfd_sink_debug("waitting for manager thread exit");
		g_thread_join(wfd_sink->manager_thread);
		g_mutex_clear(&(wfd_sink->manager_thread_mutex));
		g_cond_clear(&(wfd_sink->manager_thread_cond));
		wfd_sink_debug("manager thread released");
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static gpointer
__mm_wfd_sink_manager_thread(gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *) data;
	WFDSinkManagerCMDType cmd = WFD_SINK_MANAGER_CMD_NONE;
	GList *walk = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, NULL);

	if (wfd_sink->manager_thread_exit) {
		wfd_sink_debug("exit manager thread...");
		return NULL;
	}

	wfd_sink_debug("manager thread started. waiting for signal");

	while (TRUE) {
		WFD_SINK_MANAGER_LOCK(wfd_sink);
		WFD_SINK_MANAGER_WAIT_CMD(wfd_sink);

		for (walk = wfd_sink->manager_thread_cmd; walk; walk = g_list_next(walk)) {
			cmd = GPOINTER_TO_INT(walk->data);

			wfd_sink_debug("got command %d", cmd);

			switch (cmd) {
				case WFD_SINK_MANAGER_CMD_LINK_A_DECODEBIN:
					wfd_sink_debug("try to link audio decodebin.");
					if (MM_ERROR_NONE != __mm_wfd_sink_link_audio_decodebin(wfd_sink)) {
						wfd_sink_error("failed to link audio decodebin.....\n");
						goto EXIT;
					}
					break;
				case WFD_SINK_MANAGER_CMD_LINK_V_DECODEBIN:
					wfd_sink_debug("try to link video decodebin.");
					if (MM_ERROR_NONE != __mm_wfd_sink_link_video_decodebin(wfd_sink)) {
						wfd_sink_error("failed to link video decodebin.....\n");
						goto EXIT;
					}
					break;
				case WFD_SINK_MANAGER_CMD_PREPARE_A_PIPELINE:
					wfd_sink_debug("try to prepare audio pipeline.");
					if (MM_ERROR_NONE != __mm_wfd_sink_prepare_audio_pipeline(wfd_sink)) {
						wfd_sink_error("failed to prepare audio pipeline.....\n");
						goto EXIT;
					}
					break;
				case WFD_SINK_MANAGER_CMD_PREPARE_V_PIPELINE:
					wfd_sink_debug("try to prepare video pipeline.");
					if (MM_ERROR_NONE != __mm_wfd_sink_prepare_video_pipeline(wfd_sink)) {
						wfd_sink_error("failed to prepare video pipeline.....\n");
						goto EXIT;
					}
					break;
				case WFD_SINK_MANAGER_CMD_EXIT:
					wfd_sink_debug("exiting manager thread");
					goto EXIT;
					break;
				default:
					break;
			}
		}

		g_list_free(wfd_sink->manager_thread_cmd);
		wfd_sink->manager_thread_cmd = NULL;

		WFD_SINK_MANAGER_UNLOCK(wfd_sink);
	}

	wfd_sink_debug_fleave();

	return NULL;

EXIT:
	wfd_sink->manager_thread_exit = TRUE;
	g_list_free(wfd_sink->manager_thread_cmd);
	wfd_sink->manager_thread_cmd = NULL;
	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	return NULL;
}

