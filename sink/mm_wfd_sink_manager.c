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
	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* create capture mutex */
	wfd_sink->manager_thread_mutex = g_mutex_new();
	if (wfd_sink->manager_thread_mutex == NULL)
	{
		debug_error("failed to create manager thread mutex");
		goto failed_to_init;
	}

	/* create capture cond */
	wfd_sink->manager_thread_cond = g_cond_new();
	if (wfd_sink->manager_thread_cond == NULL)
	{
		debug_error("failed to create manger thread cond");
		goto failed_to_init;
	}

	wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_NONE;

	/* create video capture thread */
	wfd_sink->manager_thread =
		g_thread_create (__mm_wfd_sink_manager_thread, (gpointer)wfd_sink, TRUE, NULL);
	if (wfd_sink->manager_thread == NULL)
	{
		debug_error ("failed to create manager thread\n");
		goto failed_to_init;
	}

	debug_fleave();

	return MM_ERROR_NONE;

failed_to_init:
	if (wfd_sink->manager_thread_mutex)
		g_mutex_free (wfd_sink->manager_thread_mutex);
	wfd_sink->manager_thread_mutex = NULL;

	if (wfd_sink->manager_thread_cond)
		g_cond_free (wfd_sink->manager_thread_cond);
	wfd_sink->manager_thread_cond = NULL;

	return MM_ERROR_WFD_INTERNAL;
}

int _mm_wfd_sink_release_manager(mm_wfd_sink_t* wfd_sink)
{
	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* release capture thread */
	if ( wfd_sink->manager_thread_cond &&
		 wfd_sink->manager_thread_mutex &&
		 wfd_sink->manager_thread )
	{
		WFD_SINK_MANAGER_LOCK(wfd_sink);
		WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_EXIT);
		WFD_SINK_MANAGER_UNLOCK(wfd_sink);

		debug_log("waitting for manager thread exit");
		g_thread_join (wfd_sink->manager_thread);
		g_mutex_free (wfd_sink->manager_thread_mutex);
		g_cond_free (wfd_sink->manager_thread_cond);
		debug_log("manager thread released");
	}

	debug_fleave();

	return MM_ERROR_NONE;
}

static gpointer
__mm_wfd_sink_manager_thread(gpointer data)
{
	mm_wfd_sink_t* wfd_sink = (mm_wfd_sink_t*) data;
	gboolean link_auido_bin = FALSE;
	gboolean link_video_bin = FALSE;
	gboolean set_ready_audio_bin = FALSE;
	gboolean set_ready_video_bin = FALSE;


	debug_fenter();

	return_val_if_fail (wfd_sink, NULL);

	if (wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_EXIT)
		goto EXIT;

	debug_log("manager thread started. waiting for signal");

	while (TRUE)
	{
		WFD_SINK_MANAGER_LOCK(wfd_sink);
		WFD_SINK_MANAGER_WAIT_CMD(wfd_sink);

		debug_error("No-error:got command %x", wfd_sink->manager_thread_cmd);

		if (wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_EXIT)
		{
			debug_log("exiting manager thread");
			goto EXIT;
		}

		/* check command */
		link_auido_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_LINK_A_BIN;
		link_video_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_LINK_V_BIN;
		set_ready_audio_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_PREPARE_A_BIN;
		if (set_ready_audio_bin && !link_auido_bin && !wfd_sink->audio_bin_is_linked)
		{
			debug_error("audio bin is not linked... wait for command for linking audiobin");
			WFD_SINK_MANAGER_UNLOCK(wfd_sink);
			continue;
		}
		set_ready_video_bin = wfd_sink->manager_thread_cmd & WFD_SINK_MANAGER_CMD_PREPARE_V_BIN;
		if (set_ready_video_bin && !link_video_bin && !wfd_sink->video_bin_is_linked)
		{
			debug_error("video bin is not linked... wait for command for linking videobin.");
			WFD_SINK_MANAGER_UNLOCK(wfd_sink);
			continue;
		}

		/* link audio bin*/
		if (link_auido_bin)
		{
			debug_error("No-error : try to link audiobin.");
			if (MM_ERROR_NONE !=__mm_wfd_sink_link_audiobin(wfd_sink))
			{
				debug_error ("failed to link audiobin.....\n");
				goto EXIT;
			}
		}

		/* link video bin*/
		if (link_video_bin)
		{
		}

		if (set_ready_audio_bin)
		{
			debug_error("No-error : try to prepare audiobin.");
			if (MM_ERROR_NONE !=__mm_wfd_sink_prepare_audiobin(wfd_sink))
			{
				debug_error ("failed to prepare audiobin.....\n");
				goto EXIT;
			}
		}

		if (set_ready_video_bin)
		{
			debug_error("No-error : try to prepare videobin.");
			if (MM_ERROR_NONE !=__mm_wfd_sink_prepare_videobin(wfd_sink))
			{
				debug_error ("failed to prepare videobin.....\n");
				goto EXIT;
			}
		}

		wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_NONE;

		WFD_SINK_MANAGER_UNLOCK(wfd_sink);
	}

	debug_fleave();

	return NULL;

EXIT:
	wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_EXIT;

	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	return NULL;
}

