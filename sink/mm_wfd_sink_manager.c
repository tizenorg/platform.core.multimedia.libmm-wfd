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

	/* create capture mutex */
	g_mutex_init(&(wfd_sink->manager_thread_mutex));

	/* create capture cond */
	g_cond_init(&(wfd_sink->manager_thread_cond));

	wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_NONE;

	/* create video capture thread */
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

	/* release capture thread */
	if (wfd_sink->manager_thread) {
		WFD_SINK_MANAGER_LOCK(wfd_sink);
		WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_EXIT);
		WFD_SINK_MANAGER_UNLOCK(wfd_sink);

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
	gboolean link_auido_bin = FALSE;
	gboolean link_video_bin = FALSE;
	gboolean set_ready_audio_bin = FALSE;
	gboolean set_ready_video_bin = FALSE;


	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, NULL);

	if (wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_EXIT) {
		wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_EXIT;
		return NULL;
	}

	wfd_sink_debug("manager thread started. waiting for signal");

	while (TRUE) {
		WFD_SINK_MANAGER_LOCK(wfd_sink);
		WFD_SINK_MANAGER_WAIT_CMD(wfd_sink);

		wfd_sink_debug("got command %x", wfd_sink->manager_thread_cmd);

		if (wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_EXIT) {
			wfd_sink_debug("exiting manager thread");
			goto EXIT;
		}

		/* check command */
		link_auido_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_LINK_A_BIN;
		link_video_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_LINK_V_BIN;
		set_ready_audio_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_PREPARE_A_BIN;
		if (set_ready_audio_bin && !link_auido_bin && !wfd_sink->audio_bin_is_linked) {
			wfd_sink_error("audio bin is not linked... wait for command for linking audiobin");
			WFD_SINK_MANAGER_UNLOCK(wfd_sink);
			continue;
		}
		set_ready_video_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_PREPARE_V_BIN;
		if (set_ready_video_bin && !link_video_bin && !wfd_sink->video_bin_is_linked) {
			wfd_sink_error("video bin is not linked... wait for command for linking videobin.");
			WFD_SINK_MANAGER_UNLOCK(wfd_sink);
			continue;
		}

		/* link audio bin*/
		if (link_auido_bin) {
			wfd_sink_debug("try to link audiobin.");
			if (MM_ERROR_NONE != __mm_wfd_sink_link_audiobin(wfd_sink)) {
				wfd_sink_error("failed to link audiobin.....\n");
				goto EXIT;
			}
		}

		/* link video bin*/
		if (link_video_bin) {
			wfd_sink_debug("try to link videobin.");
		}

		if (set_ready_audio_bin) {
			wfd_sink_debug("try to prepare audiobin.");
			if (MM_ERROR_NONE != __mm_wfd_sink_prepare_audiobin(wfd_sink)) {
				wfd_sink_error("failed to prepare audiobin.....\n");
				goto EXIT;
			}
		}

		if (set_ready_video_bin) {
			wfd_sink_debug("try to prepare videobin.");
			if (MM_ERROR_NONE != __mm_wfd_sink_prepare_videobin(wfd_sink)) {
				wfd_sink_error("failed to prepare videobin.....\n");
				goto EXIT;
			}
		}

		wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_NONE;

		WFD_SINK_MANAGER_UNLOCK(wfd_sink);
	}

	wfd_sink_debug_fleave();

	return NULL;

EXIT:
	wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_EXIT;

	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	return NULL;
}

