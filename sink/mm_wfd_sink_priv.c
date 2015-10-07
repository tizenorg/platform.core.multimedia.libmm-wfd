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

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <Elementary.h>

#include "mm_wfd_sink_util.h"
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_manager.h"
#include "mm_wfd_sink_dlog.h"
#include "mm_wfd_sink_wfd_enum.h"


/* gstreamer */
static int __mm_wfd_sink_init_gstreamer(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_pipeline(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_audio_decodebin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_video_decodebin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_destroy_audio_decodebin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_destroy_video_decodebin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_video_sinkbin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_audio_sinkbin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_destroy_pipeline(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_set_pipeline_state(mm_wfd_sink_t *wfd_sink, GstState state, gboolean async);

/* state */
static int __mm_wfd_sink_check_state(mm_wfd_sink_t *wfd_sink, MMWFDSinkCommandType cmd);
static int __mm_wfd_sink_set_state(mm_wfd_sink_t *wfd_sink, MMWFDSinkStateType state);

/* util */
static void __mm_wfd_sink_dump_pipeline_state(mm_wfd_sink_t *wfd_sink);
static void __mm_wfd_sink_prepare_video_resolution(gint resolution, guint *CEA_resolution,
                                                   guint *VESA_resolution, guint *HH_resolution);

int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	mm_wfd_sink_t *new_wfd_sink = NULL;

	/* create handle */
	new_wfd_sink = g_malloc0(sizeof(mm_wfd_sink_t));
	if (!new_wfd_sink) {
		wfd_sink_error("failed to allocate memory for wi-fi display sink");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* Initialize gstreamer related */
	new_wfd_sink->attrs = 0;

	new_wfd_sink->pipeline = NULL;
	new_wfd_sink->audio_decodebin_is_linked = FALSE;
	new_wfd_sink->video_decodebin_is_linked = FALSE;

	/* Initialize timestamp compensation related */
	new_wfd_sink->need_to_reset_basetime = FALSE;
	new_wfd_sink->clock = NULL;
	new_wfd_sink->video_buffer_count = 0LL;
	new_wfd_sink->video_average_gap = 0LL;
	new_wfd_sink->video_accumulated_gap = 0LL;
	new_wfd_sink->audio_buffer_count = 0LL;
	new_wfd_sink->audio_average_gap = 0LL;
	new_wfd_sink->audio_accumulated_gap = 0LL;
	new_wfd_sink->last_buffer_timestamp = GST_CLOCK_TIME_NONE;

	/* Initialize all states */
	MMWFDSINK_CURRENT_STATE(new_wfd_sink) = MM_WFD_SINK_STATE_NONE;
	MMWFDSINK_PREVIOUS_STATE(new_wfd_sink) =  MM_WFD_SINK_STATE_NONE;
	MMWFDSINK_PENDING_STATE(new_wfd_sink) =  MM_WFD_SINK_STATE_NONE;

	/* initialize audio/video information */
	new_wfd_sink->stream_info.audio_stream_info.codec = MM_WFD_SINK_AUDIO_CODEC_NONE;
	new_wfd_sink->stream_info.audio_stream_info.channels = 0;
	new_wfd_sink->stream_info.audio_stream_info.sample_rate = 0;
	new_wfd_sink->stream_info.audio_stream_info.bitwidth = 0;
	new_wfd_sink->stream_info.video_stream_info.codec = MM_WFD_SINK_VIDEO_CODEC_NONE;
	new_wfd_sink->stream_info.video_stream_info.width = 0;
	new_wfd_sink->stream_info.video_stream_info.height = 0;
	new_wfd_sink->stream_info.video_stream_info.frame_rate = 0;

	/* Initialize command */
	new_wfd_sink->cmd = MM_WFD_SINK_COMMAND_CREATE;
	new_wfd_sink->waiting_cmd = FALSE;

	/* Initialize manager related */
	new_wfd_sink->manager_thread = NULL;
	new_wfd_sink->manager_thread_cmd = NULL;
	new_wfd_sink->manager_thread_exit = FALSE;

	/* Initialize video resolution */
	new_wfd_sink->supportive_resolution = MM_WFD_SINK_RESOLUTION_UNKNOWN;

	/* construct attributes */
	new_wfd_sink->attrs = _mmwfd_construct_attribute((MMHandleType)new_wfd_sink);
	if (!new_wfd_sink->attrs) {
		MMWFDSINK_FREEIF(new_wfd_sink);
		wfd_sink_error("failed to set attribute");
		return MM_ERROR_WFD_INTERNAL;
	}

	/* load ini for initialize */
	result = mm_wfd_sink_ini_load(&new_wfd_sink->ini);
	if (result != MM_ERROR_NONE) {
		wfd_sink_error("failed to load ini file");
		goto fail_to_load_ini;
	}
	new_wfd_sink->need_to_reset_basetime = new_wfd_sink->ini.enable_reset_basetime;

	/* initialize manager */
	result = _mm_wfd_sink_init_manager(new_wfd_sink);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to init manager : %d", result);
		goto fail_to_init;
	}

	/* initialize gstreamer */
	result = __mm_wfd_sink_init_gstreamer(new_wfd_sink);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to init gstreamer : %d", result);
		goto fail_to_init;
	}

	/* set state */
	__mm_wfd_sink_set_state(new_wfd_sink,  MM_WFD_SINK_STATE_NULL);

	/* now take handle */
	*wfd_sink = new_wfd_sink;

	wfd_sink_debug_fleave();

	return result;

	/* ERRORS */
fail_to_init:
	mm_wfd_sink_ini_unload(&new_wfd_sink->ini);
fail_to_load_ini:
	_mmwfd_deconstruct_attribute(new_wfd_sink->attrs);
	MMWFDSINK_FREEIF(new_wfd_sink);

	*wfd_sink = NULL;

	return result;
}

int _mm_wfd_sink_prepare(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_PREPARE);

	/* construct pipeline */
	/* create main pipeline */
	result = __mm_wfd_sink_create_pipeline(wfd_sink);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to create pipeline : %d", result);
		goto fail_to_create;
	}

	/* create video decodebin */
	result = __mm_wfd_sink_create_video_decodebin(wfd_sink);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to create video decodebin %d", result);
		goto fail_to_create;
	}

	/* create video sinkbin */
	result = __mm_wfd_sink_create_video_sinkbin(wfd_sink);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to create video sinkbin %d", result);
		goto fail_to_create;
	}

	/* create audio decodebin */
	result = __mm_wfd_sink_create_audio_decodebin(wfd_sink);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("fail to create audio decodebin : %d", result);
		goto fail_to_create;
	}

	/* create audio sinkbin */
	result = __mm_wfd_sink_create_audio_sinkbin(wfd_sink);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("fail to create audio sinkbin : %d", result);
		goto fail_to_create;
	}

	/* set pipeline READY state */
	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_READY, TRUE);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to set state : %d", result);
		goto fail_to_create;
	}

	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_PREPARED);

	wfd_sink_debug_fleave();

	return result;

	/* ERRORS */
fail_to_create:
	/* need to destroy pipeline already created */
	__mm_wfd_sink_destroy_pipeline(wfd_sink);
	return result;
}

int _mm_wfd_sink_connect(mm_wfd_sink_t *wfd_sink, const char *uri)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(uri && strlen(uri) > strlen("rtsp://"),
	                            MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_DEPAY].gst &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_DEMUX].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_CONNECT);

	wfd_sink_debug("try to connect to %s.....", GST_STR_NULL(uri));

	/* set uri to wfdsrc */
	g_object_set(G_OBJECT(wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst), "location", uri, NULL);

	/* set pipeline PAUSED state */
	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_PAUSED, TRUE);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to set state : %d", result);
		return result;
	}

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_START);

	WFD_SINK_MANAGER_LOCK(wfd_sink) ;
	wfd_sink_debug("check pipeline is ready to start");
	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_PLAYING, TRUE);
	if (result < MM_ERROR_NONE) {
		wfd_sink_error("failed to set state : %d", result);
		return result;
	}

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_pause(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_PAUSE);

	g_signal_emit_by_name(wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst, "pause", NULL);

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_resume(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_RESUME);

	g_signal_emit_by_name(wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst, "resume", NULL);

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_disconnect(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_DISCONNECT);

	WFD_SINK_MANAGER_APPEND_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_EXIT);
	WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink);

	g_signal_emit_by_name(wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst, "close", NULL);

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_unprepare(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_UNPREPARE);

	WFD_SINK_MANAGER_APPEND_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_EXIT);
	WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink);

	/* release pipeline */
	result =  __mm_wfd_sink_destroy_pipeline(wfd_sink);
	if (result != MM_ERROR_NONE) {
		wfd_sink_error("failed to destory pipeline");
		return MM_ERROR_WFD_INTERNAL;
	} else {
		wfd_sink_debug("success to destory pipeline");
	}

	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_NULL);

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE(wfd_sink, MM_WFD_SINK_COMMAND_DESTROY);

	/* unload ini */
	mm_wfd_sink_ini_unload(&wfd_sink->ini);

	/* release attributes */
	_mmwfd_deconstruct_attribute(wfd_sink->attrs);

	if (MM_ERROR_NONE != _mm_wfd_sink_release_manager(wfd_sink)) {
		wfd_sink_error("failed to release manager");
		return MM_ERROR_WFD_INTERNAL;
	}


	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_NONE);

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_set_message_callback(mm_wfd_sink_t *wfd_sink, MMWFDMessageCallback callback, void *user_data)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	wfd_sink->msg_cb = callback;
	wfd_sink->msg_user_data = user_data;

	wfd_sink_debug_fleave();

	return result;
}

static int __mm_wfd_sink_init_gstreamer(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	gint *argc = NULL;
	gchar **argv = NULL;
	static const int max_argc = 50;
	GError *err = NULL;
	gint i = 0;

	wfd_sink_debug_fenter();

	/* alloc */
	argc = calloc(1, sizeof(gint));
	argv = calloc(max_argc, sizeof(gchar *));
	if (!argc || !argv) {
		wfd_sink_error("failed to allocate memory for wfdsink");

		MMWFDSINK_FREEIF(argv);
		MMWFDSINK_FREEIF(argc);

		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* we would not do fork for scanning plugins */
	argv[*argc] = g_strdup("--gst-disable-registry-fork");
	(*argc)++;

	/* check disable registry scan */
	argv[*argc] = g_strdup("--gst-disable-registry-update");
	(*argc)++;

	/* check disable segtrap */
	argv[*argc] = g_strdup("--gst-disable-segtrap");
	(*argc)++;

	/* check ini */
	for (i = 0; i < 5; i++) {
		if (strlen(wfd_sink->ini.gst_param[i]) > 2) {
			wfd_sink_debug("set %s", wfd_sink->ini.gst_param[i]);
			argv[*argc] = g_strdup(wfd_sink->ini.gst_param[i]);
			(*argc)++;
		}
	}

	wfd_sink_debug("initializing gstreamer with following parameter");
	wfd_sink_debug("argc : %d", *argc);

	for (i = 0; i < *argc; i++) {
		wfd_sink_debug("argv[%d] : %s", i, argv[i]);
	}

	/* initializing gstreamer */
	if (!gst_init_check(argc, &argv, &err)) {
		wfd_sink_error("failed to initialize gstreamer: %s",
		               err ? err->message : "unknown error occurred");
		if (err)
			g_error_free(err);

		result = MM_ERROR_WFD_INTERNAL;
	}

	/* release */
	for (i = 0; i < *argc; i++) {
		MMWFDSINK_FREEIF(argv[i]);
	}
	MMWFDSINK_FREEIF(argv);
	MMWFDSINK_FREEIF(argc);

	wfd_sink_debug_fleave();

	return result;
}

static GstBusSyncReply
_mm_wfd_bus_sync_callback(GstBus *bus, GstMessage *message, gpointer data)
{
	GstBusSyncReply ret = GST_BUS_PASS;

	wfd_sink_return_val_if_fail(message &&
	                            GST_IS_MESSAGE(message) &&
	                            GST_MESSAGE_SRC(message),
	                            GST_BUS_DROP);

	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_TAG:
			break;
		case GST_MESSAGE_DURATION:
			break;
		case GST_MESSAGE_STATE_CHANGED: {
				/* we only handle state change messages from pipeline */
				if (!GST_IS_PIPELINE(GST_MESSAGE_SRC(message)))
					ret = GST_BUS_DROP;
			}
			break;
		case GST_MESSAGE_ASYNC_DONE: {
				if (!GST_IS_PIPELINE(GST_MESSAGE_SRC(message)))
					ret = GST_BUS_DROP;
			}
			break;
		default:
			break;
	}

	return ret;
}

static gboolean
_mm_wfd_sink_msg_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *) data;
	const GstStructure *message_structure = gst_message_get_structure(msg);
	gboolean ret = TRUE;

	wfd_sink_return_val_if_fail(wfd_sink, FALSE);
	wfd_sink_return_val_if_fail(msg && GST_IS_MESSAGE(msg), FALSE);

	wfd_sink_debug("got %s from %s",
	               GST_STR_NULL(GST_MESSAGE_TYPE_NAME(msg)),
	               GST_STR_NULL(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg))));

	switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ERROR: {
				GError *error = NULL;
				gchar *debug = NULL;

				/* get error code */
				gst_message_parse_error(msg, &error, &debug);

				wfd_sink_error("error : %s", error->message);
				wfd_sink_error("debug : %s", debug);

				MMWFDSINK_FREEIF(debug);
				g_error_free(error);
			}
			break;

		case GST_MESSAGE_WARNING: {
				char *debug = NULL;
				GError *error = NULL;

				gst_message_parse_warning(msg, &error, &debug);

				wfd_sink_warning("warning : %s", error->message);
				wfd_sink_warning("debug : %s", debug);

				MMWFDSINK_FREEIF(debug);
				g_error_free(error);
			}
			break;

		case GST_MESSAGE_STATE_CHANGED: {
				const GValue *voldstate, *vnewstate, *vpending;
				GstState oldstate, newstate, pending;
				const GstStructure *structure;

				/* we only handle messages from pipeline */
				if (msg->src != (GstObject *)wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst)
					break;

				/* get state info from msg */
				structure = gst_message_get_structure(msg);
				if (structure == NULL)
					break;

				voldstate = gst_structure_get_value(structure, "old-state");
				vnewstate = gst_structure_get_value(structure, "new-state");
				vpending = gst_structure_get_value(structure, "pending-state");
				if (voldstate == NULL || vnewstate == NULL || vpending == NULL)
					break;

				oldstate = (GstState)voldstate->data[0].v_int;
				newstate = (GstState)vnewstate->data[0].v_int;
				pending = (GstState)vpending->data[0].v_int;

				wfd_sink_debug("state changed [%s] : %s--->%s final : %s",
				               GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)),
				               gst_element_state_get_name((GstState)oldstate),
				               gst_element_state_get_name((GstState)newstate),
				               gst_element_state_get_name((GstState)pending));

				if (oldstate == newstate) {
					wfd_sink_debug("pipeline reports state transition to old state");
					break;
				}

				switch (newstate) {
					case GST_STATE_VOID_PENDING:
					case GST_STATE_NULL:
					case GST_STATE_READY:
					case GST_STATE_PAUSED:
					case GST_STATE_PLAYING:
					default:
						break;
				}
			}
			break;

		case GST_MESSAGE_CLOCK_LOST: {
				GstClock *clock = NULL;
				gst_message_parse_clock_lost(msg, &clock);
				wfd_sink_debug("The current clock[%s] as selected by the pipeline became unusable.",
				               (clock ? GST_OBJECT_NAME(clock) : "NULL"));
			}
			break;

		case GST_MESSAGE_NEW_CLOCK: {
				GstClock *clock = NULL;
				gst_message_parse_new_clock(msg, &clock);
				if (!clock)
					break;

				if (wfd_sink->clock) {
					if (wfd_sink->clock != clock)
						wfd_sink_debug("clock is changed! [%s] -->[%s]",
						               GST_STR_NULL(GST_OBJECT_NAME(wfd_sink->clock)),
						               GST_STR_NULL(GST_OBJECT_NAME(clock)));
					else
						wfd_sink_debug("same clock is selected again! [%s]",
						               GST_STR_NULL(GST_OBJECT_NAME(clock)));
				} else {
					wfd_sink_debug("new clock [%s] was selected in the pipeline",
					               (GST_STR_NULL(GST_OBJECT_NAME(clock))));
				}

				wfd_sink->clock = clock;
			}
			break;

		case GST_MESSAGE_APPLICATION: {
				const gchar *message_structure_name;

				message_structure_name = gst_structure_get_name(message_structure);
				if (!message_structure_name)
					break;

				wfd_sink_debug("message name : %s", GST_STR_NULL(message_structure_name));
			}
			break;

		case GST_MESSAGE_ELEMENT: {
				const gchar *structure_name = NULL;

				structure_name = gst_structure_get_name(message_structure);
				if (structure_name) {
					wfd_sink_debug("got element specific message[%s]", GST_STR_NULL(structure_name));
					if (g_strrstr(structure_name, "GstUDPSrcTimeout")) {
						wfd_sink_error("Got %s, post error message", GST_STR_NULL(structure_name));
						MMWFDSINK_POST_MESSAGE(wfd_sink,
						                       MM_ERROR_WFD_INTERNAL,
						                       MMWFDSINK_CURRENT_STATE(wfd_sink));
					}
				}
			}
			break;

		case GST_MESSAGE_PROGRESS: {
				GstProgressType type = GST_PROGRESS_TYPE_ERROR;
				gchar *category = NULL, *text = NULL;

				gst_message_parse_progress(msg, &type, &category, &text);
				wfd_sink_debug("%s : %s ", GST_STR_NULL(category), GST_STR_NULL(text));

				switch (type) {
					case GST_PROGRESS_TYPE_START:
						break;
					case GST_PROGRESS_TYPE_COMPLETE:
						if (category && !strcmp(category, "open"))
							__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_CONNECTED);
						else if (category && !strcmp(category, "play")) {
							__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_PLAYING);
							/*_mm_wfd_sink_correct_pipeline_latency (wfd_sink); */
						} else if (category && !strcmp(category, "pause"))
							__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_PAUSED);
						else if (category && !strcmp(category, "close"))
							__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_DISCONNECTED);
						break;
					case GST_PROGRESS_TYPE_CANCELED:
						break;
					case GST_PROGRESS_TYPE_ERROR:
						if (category && !strcmp(category, "open")) {
							wfd_sink_error("got error : %s", GST_STR_NULL(text));
							/*_mm_wfd_sink_disconnect (wfd_sink); */
							MMWFDSINK_POST_MESSAGE(wfd_sink,
							                       MM_ERROR_WFD_INTERNAL,
							                       MMWFDSINK_CURRENT_STATE(wfd_sink));
						} else if (category && !strcmp(category, "play")) {
							wfd_sink_error("got error : %s", GST_STR_NULL(text));
							/*_mm_wfd_sink_disconnect (wfd_sink); */
							MMWFDSINK_POST_MESSAGE(wfd_sink,
							                       MM_ERROR_WFD_INTERNAL,
							                       MMWFDSINK_CURRENT_STATE(wfd_sink));
						} else if (category && !strcmp(category, "pause")) {
							wfd_sink_error("got error : %s", GST_STR_NULL(text));
							/*_mm_wfd_sink_disconnect (wfd_sink); */
							MMWFDSINK_POST_MESSAGE(wfd_sink,
							                       MM_ERROR_WFD_INTERNAL,
							                       MMWFDSINK_CURRENT_STATE(wfd_sink));
						} else if (category && !strcmp(category, "close")) {
							wfd_sink_error("got error : %s", GST_STR_NULL(text));
							/*_mm_wfd_sink_disconnect (wfd_sink); */
							MMWFDSINK_POST_MESSAGE(wfd_sink,
							                       MM_ERROR_WFD_INTERNAL,
							                       MMWFDSINK_CURRENT_STATE(wfd_sink));
						} else {
							wfd_sink_error("got error : %s", GST_STR_NULL(text));
						}
						break;
					default:
						wfd_sink_error("progress message has no type");
						return ret;
				}

				MMWFDSINK_FREEIF(category);
				MMWFDSINK_FREEIF(text);
			}
			break;
		case GST_MESSAGE_ASYNC_START:
			wfd_sink_debug("GST_MESSAGE_ASYNC_START : %s", gst_element_get_name(GST_MESSAGE_SRC(msg)));
			break;
		case GST_MESSAGE_ASYNC_DONE:
			wfd_sink_debug("GST_MESSAGE_ASYNC_DONE : %s", gst_element_get_name(GST_MESSAGE_SRC(msg)));
			break;
		case GST_MESSAGE_UNKNOWN:
		case GST_MESSAGE_INFO:
		case GST_MESSAGE_TAG:
		case GST_MESSAGE_BUFFERING:
		case GST_MESSAGE_EOS:
		case GST_MESSAGE_STATE_DIRTY:
		case GST_MESSAGE_STEP_DONE:
		case GST_MESSAGE_CLOCK_PROVIDE:
		case GST_MESSAGE_STRUCTURE_CHANGE:
		case GST_MESSAGE_STREAM_STATUS:
		case GST_MESSAGE_SEGMENT_START:
		case GST_MESSAGE_SEGMENT_DONE:
		case GST_MESSAGE_DURATION:
		case GST_MESSAGE_LATENCY:
		case GST_MESSAGE_REQUEST_STATE:
		case GST_MESSAGE_STEP_START:
		case GST_MESSAGE_QOS:
		case GST_MESSAGE_ANY:
			break;
		default:
			wfd_sink_debug("unhandled message");
			break;
	}

	return ret;
}

static int
__mm_wfd_sink_gst_element_add_bucket_to_bin(GstBin *bin, GList *element_bucket, gboolean need_prepare)
{
	GList *bucket = element_bucket;
	MMWFDSinkGstElement *element = NULL;
	int successful_add_count = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(element_bucket, 0);
	wfd_sink_return_val_if_fail(bin, 0);

	for (; bucket; bucket = bucket->next) {
		element = (MMWFDSinkGstElement *)bucket->data;

		if (element && element->gst) {
			if (need_prepare)
				gst_element_set_state(GST_ELEMENT(element->gst), GST_STATE_READY);

			if (!gst_bin_add(GST_BIN(bin), GST_ELEMENT(element->gst))) {
				wfd_sink_error("failed to add element [%s] to bin [%s]",
				               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(element->gst))),
				               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT_CAST(bin))));
				return 0;
			}

			wfd_sink_debug("add element [%s] to bin [%s]",
			               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(element->gst))),
			               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT_CAST(bin))));

			successful_add_count++;
		}
	}

	wfd_sink_debug_fleave();

	return successful_add_count;
}

static int
__mm_wfd_sink_gst_element_link_bucket(GList *element_bucket)
{
	GList *bucket = element_bucket;
	MMWFDSinkGstElement *element = NULL;
	MMWFDSinkGstElement *prv_element = NULL;
	gint successful_link_count = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(element_bucket, -1);

	prv_element = (MMWFDSinkGstElement *)bucket->data;
	bucket = bucket->next;

	for (; bucket; bucket = bucket->next) {
		element = (MMWFDSinkGstElement *)bucket->data;

		if (element && element->gst) {
			if (gst_element_link(GST_ELEMENT(prv_element->gst), GST_ELEMENT(element->gst))) {
				wfd_sink_debug("linking [%s] to [%s] success",
				               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(prv_element->gst))),
				               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(element->gst))));
				successful_link_count++;
			} else {
				wfd_sink_error("linking [%s] to [%s] failed",
				               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(prv_element->gst))),
				               GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(element->gst))));
				return -1;
			}
		}

		prv_element = element;
	}

	wfd_sink_debug_fleave();

	return successful_link_count;
}

static int
__mm_wfd_sink_check_state(mm_wfd_sink_t *wfd_sink, MMWFDSinkCommandType cmd)
{
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_PRINT_STATE(wfd_sink);

	cur_state = MMWFDSINK_CURRENT_STATE(wfd_sink);

	switch (cmd) {
		case MM_WFD_SINK_COMMAND_CREATE: {
				if (cur_state != MM_WFD_SINK_STATE_NONE)
					goto invalid_state;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NULL;
			}
			break;

		case MM_WFD_SINK_COMMAND_PREPARE: {
				if (cur_state == MM_WFD_SINK_STATE_PREPARED)
					goto no_operation;
				else if (cur_state != MM_WFD_SINK_STATE_NULL)
					goto invalid_state;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PREPARED;
			}
			break;

		case MM_WFD_SINK_COMMAND_CONNECT: {
				if (cur_state == MM_WFD_SINK_STATE_CONNECTED)
					goto no_operation;
				else if (cur_state != MM_WFD_SINK_STATE_PREPARED)
					goto invalid_state;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_CONNECTED;
			}
			break;

		case MM_WFD_SINK_COMMAND_START: {
				if (cur_state == MM_WFD_SINK_STATE_PLAYING)
					goto no_operation;
				else if (cur_state != MM_WFD_SINK_STATE_CONNECTED)
					goto invalid_state;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PLAYING;
			}
			break;

		case MM_WFD_SINK_COMMAND_PAUSE: {
				if (cur_state == MM_WFD_SINK_STATE_PAUSED)
					goto no_operation;
				else if (cur_state != MM_WFD_SINK_STATE_PLAYING)
					goto invalid_state;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PAUSED;
			}
			break;

		case MM_WFD_SINK_COMMAND_RESUME: {
				if (cur_state == MM_WFD_SINK_STATE_PLAYING)
					goto no_operation;
				else if (cur_state != MM_WFD_SINK_STATE_PAUSED)
					goto invalid_state;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PLAYING;
			}
			break;

		case MM_WFD_SINK_COMMAND_DISCONNECT: {
				if (cur_state == MM_WFD_SINK_STATE_NONE ||
				    cur_state == MM_WFD_SINK_STATE_NULL ||
				    cur_state == MM_WFD_SINK_STATE_PREPARED ||
				    cur_state == MM_WFD_SINK_STATE_DISCONNECTED)
					goto no_operation;
				else if (cur_state != MM_WFD_SINK_STATE_PLAYING &&
				         cur_state != MM_WFD_SINK_STATE_CONNECTED &&
				         cur_state != MM_WFD_SINK_STATE_PAUSED)
					goto invalid_state;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_DISCONNECTED;
			}
			break;

		case MM_WFD_SINK_COMMAND_UNPREPARE: {
				if (cur_state == MM_WFD_SINK_STATE_NONE ||
				    cur_state == MM_WFD_SINK_STATE_NULL)
					goto no_operation;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NULL;
			}
			break;

		case MM_WFD_SINK_COMMAND_DESTROY: {
				if (cur_state == MM_WFD_SINK_STATE_NONE)
					goto no_operation;

				MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NONE;
			}
			break;

		default:
			break;
	}

	wfd_sink->cmd = cmd;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

no_operation:
	wfd_sink_debug("already %s state, nothing to do.", MMWFDSINK_STATE_GET_NAME(cur_state));
	return MM_ERROR_WFD_NO_OP;

	/* ERRORS */
invalid_state:
	wfd_sink_error("current state is invalid.", MMWFDSINK_STATE_GET_NAME(cur_state));
	return MM_ERROR_WFD_INVALID_STATE;
}

static int __mm_wfd_sink_set_state(mm_wfd_sink_t *wfd_sink, MMWFDSinkStateType state)
{
	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	if (MMWFDSINK_CURRENT_STATE(wfd_sink) == state) {
		wfd_sink_error("already state(%s)", MMWFDSINK_STATE_GET_NAME(state));
		MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NONE;
		return MM_ERROR_NONE;
	}

	/* update wi-fi display state */
	MMWFDSINK_PREVIOUS_STATE(wfd_sink) = MMWFDSINK_CURRENT_STATE(wfd_sink);
	MMWFDSINK_CURRENT_STATE(wfd_sink) = state;

	if (MMWFDSINK_CURRENT_STATE(wfd_sink) == MMWFDSINK_PENDING_STATE(wfd_sink))
		MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NONE;

	/* poset state message to application */
	MMWFDSINK_POST_MESSAGE(wfd_sink,
	                       MM_ERROR_NONE,
	                       MMWFDSINK_CURRENT_STATE(wfd_sink));

	/* print state */
	MMWFDSINK_PRINT_STATE(wfd_sink);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int
__mm_wfd_sink_set_pipeline_state(mm_wfd_sink_t *wfd_sink, GstState state, gboolean async)
{
	GstStateChangeReturn result = GST_STATE_CHANGE_FAILURE;
	GstState cur_state = GST_STATE_VOID_PENDING;
	GstState pending_state = GST_STATE_VOID_PENDING;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	wfd_sink_return_val_if_fail(state > GST_STATE_VOID_PENDING,
	                            MM_ERROR_WFD_INVALID_ARGUMENT);

	wfd_sink_debug("try to set %s state ", gst_element_state_get_name(state));

	result = gst_element_set_state(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst, state);
	if (result == GST_STATE_CHANGE_FAILURE) {
		wfd_sink_error("fail to set %s state....", gst_element_state_get_name(state));
		return MM_ERROR_WFD_INTERNAL;
	}

	if (!async) {
		wfd_sink_debug("wait for changing state is completed ");

		result = gst_element_get_state(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
		                               &cur_state, &pending_state, wfd_sink->ini.state_change_timeout * GST_SECOND);
		if (result == GST_STATE_CHANGE_FAILURE) {
			wfd_sink_error("fail to get state within %d seconds....", wfd_sink->ini.state_change_timeout);

			__mm_wfd_sink_dump_pipeline_state(wfd_sink);

			return MM_ERROR_WFD_INTERNAL;
		} else if (result == GST_STATE_CHANGE_NO_PREROLL) {
			wfd_sink_debug("successfully changed state but is not able to provide data yet");
		}

		wfd_sink_debug("cur state is %s, pending state is %s",
		               gst_element_state_get_name(cur_state),
		               gst_element_state_get_name(pending_state));
	}


	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static void
_mm_wfd_sink_reset_basetime(mm_wfd_sink_t *wfd_sink)
{
	GstClockTime base_time = GST_CLOCK_TIME_NONE;
	int i;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail(wfd_sink &&
	                        wfd_sink->pipeline &&
	                        wfd_sink->pipeline->mainbin &&
	                        wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);
	wfd_sink_return_if_fail(wfd_sink->need_to_reset_basetime);


	if (wfd_sink->clock)
		base_time = gst_clock_get_time(wfd_sink->clock);

	if (GST_CLOCK_TIME_IS_VALID(base_time)) {

		wfd_sink_debug("set pipeline base_time as now [%"GST_TIME_FORMAT"]", GST_TIME_ARGS(base_time));

		for (i = 0; i < WFD_SINK_M_NUM; i++) {
			if (wfd_sink->pipeline->mainbin[i].gst)
				gst_element_set_base_time(GST_ELEMENT_CAST(wfd_sink->pipeline->mainbin[i].gst), base_time);
		}

		if (wfd_sink->pipeline->v_decodebin) {
			for (i = 0; i < WFD_SINK_V_D_NUM; i++) {
				if (wfd_sink->pipeline->v_decodebin[i].gst)
					gst_element_set_base_time(GST_ELEMENT_CAST(wfd_sink->pipeline->v_decodebin[i].gst), base_time);
			}
		}

		if (wfd_sink->pipeline->v_sinkbin) {
			for (i = 0; i < WFD_SINK_V_S_NUM; i++) {
				if (wfd_sink->pipeline->v_sinkbin[i].gst)
					gst_element_set_base_time(GST_ELEMENT_CAST(wfd_sink->pipeline->v_sinkbin[i].gst), base_time);
			}
		}

		if (wfd_sink->pipeline->a_decodebin) {
			for (i = 0; i < WFD_SINK_A_D_NUM; i++) {
				if (wfd_sink->pipeline->a_decodebin[i].gst)
					gst_element_set_base_time(GST_ELEMENT_CAST(wfd_sink->pipeline->a_decodebin[i].gst), base_time);
			}
		}

		if (wfd_sink->pipeline->a_sinkbin) {
			for (i = 0; i < WFD_SINK_A_S_NUM; i++) {
				if (wfd_sink->pipeline->a_sinkbin[i].gst)
					gst_element_set_base_time(GST_ELEMENT_CAST(wfd_sink->pipeline->a_sinkbin[i].gst), base_time);
			}
		}

		wfd_sink->need_to_reset_basetime = FALSE;
	}

	wfd_sink_debug_fleave();

	return;
}

int
__mm_wfd_sink_prepare_video_pipeline(mm_wfd_sink_t *wfd_sink)
{
	GstElement *bin = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* check video decodebin is linked */
	if (!wfd_sink->video_decodebin_is_linked) {
		/* check video decodebin is created */
		if (wfd_sink->pipeline->v_decodebin == NULL) {
			if (MM_ERROR_NONE != __mm_wfd_sink_create_video_decodebin(wfd_sink)) {
				wfd_sink_error("failed to create video decodebin....");
				goto ERROR;
			}
		}

		if (MM_ERROR_NONE != __mm_wfd_sink_link_video_decodebin(wfd_sink)) {
			wfd_sink_error("failed to link video decodebin.....");
			goto ERROR;
		}
	}

	/* check video sinkbin is created */
	if (wfd_sink->pipeline->v_sinkbin == NULL) {
		if (MM_ERROR_NONE != __mm_wfd_sink_create_video_sinkbin(wfd_sink)) {
			wfd_sink_error("failed to create video sinkbin....");
			goto ERROR;
		}
	}

	/* set video decodebin state as READY */
	if (wfd_sink->pipeline->v_decodebin && wfd_sink->pipeline->v_decodebin[WFD_SINK_V_D_BIN].gst) {
		bin = wfd_sink->pipeline->v_decodebin[WFD_SINK_V_D_BIN].gst;
		if (GST_STATE(bin) <= GST_STATE_NULL) {
			if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(bin, GST_STATE_READY)) {
				wfd_sink_error("failed to set state(READY) to video decodebin");
				goto ERROR;
			}
		}
	} else {
		wfd_sink_warning("going on without video decodebin....");
	}

	/* set video sinkbin state as READY */
	if (wfd_sink->pipeline->v_sinkbin && wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_BIN].gst) {
		bin = wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_BIN].gst;
		if (GST_STATE(bin) <= GST_STATE_NULL) {
			if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(bin, GST_STATE_READY)) {
				wfd_sink_error("failed to set state(READY) to video sinkbin");
				goto ERROR;
			}
		}
	} else {
		wfd_sink_warning("going on without video sinkbin....");
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

	/* ERRORS */
ERROR:
	/* need to notify to app */
	MMWFDSINK_POST_MESSAGE(wfd_sink,
	                       MM_ERROR_WFD_INTERNAL,
	                       MMWFDSINK_CURRENT_STATE(wfd_sink));

	return MM_ERROR_WFD_INTERNAL;
}

int
__mm_wfd_sink_prepare_audio_pipeline(mm_wfd_sink_t *wfd_sink)
{
	GstElement *bin  = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* check audio decodebin is linked */
	if (!wfd_sink->audio_decodebin_is_linked) {
		/* check audio decodebin is created */
		if (wfd_sink->pipeline->a_decodebin == NULL) {
			if (MM_ERROR_NONE != __mm_wfd_sink_create_audio_decodebin(wfd_sink)) {
				wfd_sink_error("failed to create audio decodebin....");
				goto ERROR;
			}
		}

		if (MM_ERROR_NONE != __mm_wfd_sink_link_audio_decodebin(wfd_sink)) {
			wfd_sink_error("failed to link audio decodebin.....");
			goto ERROR;
		}
	}

	/* check audio sinkbin is created */
	if (wfd_sink->pipeline->a_sinkbin == NULL) {
		if (MM_ERROR_NONE != __mm_wfd_sink_create_audio_sinkbin(wfd_sink)) {
			wfd_sink_error("failed to create audio sinkbin....");
			goto ERROR;
		}
	}

	/* set audio decodebin state as READY */
	if (wfd_sink->pipeline->a_decodebin && wfd_sink->pipeline->a_decodebin[WFD_SINK_A_D_BIN].gst) {
		bin  = wfd_sink->pipeline->a_decodebin[WFD_SINK_A_D_BIN].gst;
		if (GST_STATE(bin) <= GST_STATE_NULL) {
			if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(bin, GST_STATE_READY)) {
				wfd_sink_error("failed to set state(READY) to audio decodebin");
				goto ERROR;
			}
		}
	} else {
		wfd_sink_warning("going on without audio decodebin....");
	}

	/* set audio sinkbin state as READY */
	if (wfd_sink->pipeline->a_sinkbin && wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_BIN].gst) {
		bin = wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_BIN].gst;
		if (GST_STATE(bin) <= GST_STATE_NULL) {
			if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(bin , GST_STATE_READY)) {
				wfd_sink_error("failed to set state(READY) to audio sinkbin");
				goto ERROR;
			}
		}
	} else {
		wfd_sink_warning("going on without audio sinkbin....");
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

	/* ERRORS */
ERROR:
	/* need to notify to app */
	MMWFDSINK_POST_MESSAGE(wfd_sink,
	                       MM_ERROR_WFD_INTERNAL,
	                       MMWFDSINK_CURRENT_STATE(wfd_sink));

	return MM_ERROR_WFD_INTERNAL;
}

#define COMPENSATION_CRETERIA_VALUE 1000000 /* 1 msec */
#define COMPENSATION_CHECK_PERIOD (30*GST_SECOND)  /* 30 sec */

static GstPadProbeReturn
_mm_wfd_sink_check_running_time(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)u_data;
	GstClockTime current_time = GST_CLOCK_TIME_NONE;
	GstClockTime start_time = GST_CLOCK_TIME_NONE;
	GstClockTime running_time = GST_CLOCK_TIME_NONE;
	GstClockTime base_time = GST_CLOCK_TIME_NONE;
	GstClockTime render_time = GST_CLOCK_TIME_NONE;
	GstClockTimeDiff diff = GST_CLOCK_TIME_NONE;
	GstBuffer *buffer = NULL;
	gint64 ts_offset = 0LL;

	wfd_sink_return_val_if_fail(info, FALSE);
	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
	                            GST_PAD_PROBE_DROP);

	if (!wfd_sink->clock) {
		wfd_sink_warning("pipeline did not select clock, yet");
		return GST_PAD_PROBE_OK;
	}

	if (wfd_sink->need_to_reset_basetime)
		_mm_wfd_sink_reset_basetime(wfd_sink);

	/* calculate current runninig time */
	current_time = gst_clock_get_time(wfd_sink->clock);
	if (g_strrstr(GST_OBJECT_NAME(pad), "video"))
		base_time = gst_element_get_base_time(wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_BIN].gst);
	else if (g_strrstr(GST_OBJECT_NAME(pad), "audio"))
		base_time = gst_element_get_base_time(wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_BIN].gst);
	start_time = gst_element_get_start_time(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);
	if (GST_CLOCK_TIME_IS_VALID(current_time) &&
	    GST_CLOCK_TIME_IS_VALID(start_time) &&
	    GST_CLOCK_TIME_IS_VALID(base_time)) {
		running_time = current_time - (start_time + base_time);
	} else {
		wfd_sink_debug("current time %"GST_TIME_FORMAT", start time %"GST_TIME_FORMAT
		               "  base time %"GST_TIME_FORMAT"", GST_TIME_ARGS(current_time),
		               GST_TIME_ARGS(start_time), GST_TIME_ARGS(base_time));
		return GST_PAD_PROBE_OK;
	}

	/* calculate this buffer rendering time */
	buffer = gst_pad_probe_info_get_buffer(info);
	if (!GST_BUFFER_TIMESTAMP_IS_VALID(buffer)) {
		wfd_sink_warning("buffer timestamp is invalid.");
		return GST_PAD_PROBE_OK;
	}

	if (g_strrstr(GST_OBJECT_NAME(pad), "audio")) {
		if (wfd_sink->pipeline && wfd_sink->pipeline->a_sinkbin && wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_SINK].gst)
			g_object_get(G_OBJECT(wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_SINK].gst), "ts-offset", &ts_offset, NULL);
	} else if (g_strrstr(GST_OBJECT_NAME(pad), "video")) {
		if (wfd_sink->pipeline && wfd_sink->pipeline->v_sinkbin && wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_SINK].gst)
			g_object_get(G_OBJECT(wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_SINK].gst), "ts-offset", &ts_offset, NULL);
	}

	render_time = GST_BUFFER_TIMESTAMP(buffer);
	render_time += ts_offset;

	/* chekc this buffer could be rendered or not */
	if (GST_CLOCK_TIME_IS_VALID(running_time) && GST_CLOCK_TIME_IS_VALID(render_time)) {
		diff = GST_CLOCK_DIFF(running_time, render_time);
		if (diff < 0) {
			/* this buffer could be NOT rendered */
			wfd_sink_debug("%s : diff time : -%" GST_TIME_FORMAT "",
			               GST_STR_NULL((GST_OBJECT_NAME(pad))),
			               GST_TIME_ARGS(GST_CLOCK_DIFF(render_time, running_time)));
		} else {
			/* this buffer could be rendered */
			/*wfd_sink_debug ("%s :diff time : %" GST_TIME_FORMAT "\n", */
			/*	GST_STR_NULL((GST_OBJECT_NAME(pad))), */
			/*	GST_TIME_ARGS(diff)); */
		}
	}

	/* update buffer count and gap */
	if (g_strrstr(GST_OBJECT_NAME(pad), "video")) {
		wfd_sink->video_buffer_count++;
		wfd_sink->video_accumulated_gap += diff;
	} else if (g_strrstr(GST_OBJECT_NAME(pad), "audio")) {
		wfd_sink->audio_buffer_count++;
		wfd_sink->audio_accumulated_gap += diff;
	} else {
		wfd_sink_warning("invalid buffer type.. ");
		return GST_PAD_PROBE_DROP;
	}

	if (GST_CLOCK_TIME_IS_VALID(wfd_sink->last_buffer_timestamp)) {
		/* fisrt 60sec, just calculate the gap between source device and sink device */
		if (GST_BUFFER_TIMESTAMP(buffer) < 60 * GST_SECOND)
			return GST_PAD_PROBE_OK;

		/* every 10sec, calculate the gap between source device and sink device */
		if (GST_CLOCK_DIFF(wfd_sink->last_buffer_timestamp, GST_BUFFER_TIMESTAMP(buffer))
		    > COMPENSATION_CHECK_PERIOD) {
			gint64 audio_avgrage_gap = 0LL;
			gint64 video_avgrage_gap = 0LL;
			gint64 audio_avgrage_gap_diff = 0LL;
			gint64 video_avgrage_gap_diff = 0LL;
			gboolean video_minus_compensation = FALSE;
			gboolean audio_minus_compensation = FALSE;
			gint64 avgrage_gap_diff = 0LL;
			gboolean minus_compensation = FALSE;

			/* check video */
			if (wfd_sink->video_buffer_count > 0) {
				video_avgrage_gap = wfd_sink->video_accumulated_gap / wfd_sink->video_buffer_count;

				if (wfd_sink->video_average_gap != 0) {
					if (video_avgrage_gap > wfd_sink->video_average_gap) {
						video_avgrage_gap_diff = video_avgrage_gap - wfd_sink->video_average_gap;
						video_minus_compensation = TRUE;
					} else {
						video_avgrage_gap_diff = wfd_sink->video_average_gap - video_avgrage_gap;
						video_minus_compensation = FALSE;
					}
				} else {
					wfd_sink_debug("first update video average gap(%lld) ", video_avgrage_gap);
					wfd_sink->video_average_gap = video_avgrage_gap;
				}
			} else {
				wfd_sink_debug("there is no video buffer flow during %"GST_TIME_FORMAT
				               " ~ %" GST_TIME_FORMAT"",
				               GST_TIME_ARGS(wfd_sink->last_buffer_timestamp),
				               GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buffer)));
			}

			/* check audio */
			if (wfd_sink->audio_buffer_count > 0) {
				audio_avgrage_gap = wfd_sink->audio_accumulated_gap / wfd_sink->audio_buffer_count;

				if (wfd_sink->audio_average_gap != 0) {
					if (audio_avgrage_gap > wfd_sink->audio_average_gap) {
						audio_avgrage_gap_diff = audio_avgrage_gap - wfd_sink->audio_average_gap;
						audio_minus_compensation = TRUE;
					} else {
						audio_avgrage_gap_diff = wfd_sink->audio_average_gap - audio_avgrage_gap;
						audio_minus_compensation = FALSE;
					}
				} else {
					wfd_sink_debug("first update audio average gap(%lld) ", audio_avgrage_gap);
					wfd_sink->audio_average_gap = audio_avgrage_gap;
				}
			} else {
				wfd_sink_debug("there is no audio buffer flow during %"GST_TIME_FORMAT
				               " ~ %" GST_TIME_FORMAT"",
				               GST_TIME_ARGS(wfd_sink->last_buffer_timestamp),
				               GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buffer)));
			}

			/* selecet average_gap_diff between video and audio */
			/*  which makes no buffer drop in the sink elements */
			if (video_avgrage_gap_diff && audio_avgrage_gap_diff) {
				if (!video_minus_compensation && !audio_minus_compensation) {
					minus_compensation = FALSE;
					if (video_avgrage_gap_diff > audio_avgrage_gap_diff)
						avgrage_gap_diff = video_avgrage_gap_diff;
					else
						avgrage_gap_diff = audio_avgrage_gap_diff;
				} else if (video_minus_compensation && audio_minus_compensation) {
					minus_compensation = TRUE;
					if (video_avgrage_gap_diff > audio_avgrage_gap_diff)
						avgrage_gap_diff = audio_avgrage_gap_diff;
					else
						avgrage_gap_diff = video_avgrage_gap_diff;
				} else {
					minus_compensation = FALSE;
					if (!video_minus_compensation)
						avgrage_gap_diff = video_avgrage_gap_diff;
					else
						avgrage_gap_diff = audio_avgrage_gap_diff;
				}
			} else if (video_avgrage_gap_diff) {
				minus_compensation = video_minus_compensation;
				avgrage_gap_diff = video_avgrage_gap_diff;
			} else if (audio_avgrage_gap_diff) {
				minus_compensation = audio_minus_compensation;
				avgrage_gap_diff = audio_avgrage_gap_diff;
			}

			wfd_sink_debug("average diff gap difference beween audio:%s%lld and video:%s%lld ",
			               audio_minus_compensation ? "-" : "", audio_avgrage_gap_diff,
			               video_minus_compensation ? "-" : "", video_avgrage_gap_diff);


			/* if calculated gap diff is larger than 1ms. need to compensate buffer timestamp */
			if (avgrage_gap_diff >= COMPENSATION_CRETERIA_VALUE) {
				if (minus_compensation)
					ts_offset -= avgrage_gap_diff;
				else
					ts_offset += avgrage_gap_diff;

				wfd_sink_debug("do timestamp compensation : %s%lld (ts-offset : %"
				               GST_TIME_FORMAT") at(%" GST_TIME_FORMAT")",
				               minus_compensation ? "-" : "", avgrage_gap_diff,
				               GST_TIME_ARGS(ts_offset), GST_TIME_ARGS(running_time));

				if (wfd_sink->pipeline && wfd_sink->pipeline->a_sinkbin && wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_SINK].gst)
					g_object_set(G_OBJECT(wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_SINK].gst), "ts-offset", (gint64)ts_offset, NULL);
				if (wfd_sink->pipeline && wfd_sink->pipeline->v_sinkbin && wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_SINK].gst)
					g_object_set(G_OBJECT(wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_SINK].gst), "ts-offset", (gint64)ts_offset, NULL);
			} else {
				wfd_sink_debug("don't need to do timestamp compensation : %s%lld (ts-offset : %"GST_TIME_FORMAT ")",
				               minus_compensation ? "-" : "", avgrage_gap_diff, GST_TIME_ARGS(ts_offset));
			}

			/* reset values*/
			wfd_sink->video_buffer_count = 0;
			wfd_sink->video_accumulated_gap = 0LL;
			wfd_sink->audio_buffer_count = 0;
			wfd_sink->audio_accumulated_gap = 0LL;
			wfd_sink->last_buffer_timestamp = GST_BUFFER_TIMESTAMP(buffer);
		}
	} else {
		wfd_sink_debug("first update last buffer timestamp :%" GST_TIME_FORMAT,
		               GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buffer)));
		wfd_sink->last_buffer_timestamp = GST_BUFFER_TIMESTAMP(buffer);
	}

	return GST_PAD_PROBE_OK;
}


static void
__mm_wfd_sink_demux_pad_added(GstElement *ele, GstPad *pad, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
	gchar *name = gst_pad_get_name(pad);
	GstElement *pipeline = NULL;
	GstElement *decodebin = NULL;
	GstElement *sinkbin = NULL;
	GstPad *sinkpad = NULL;
	GstPad *srcpad = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->mainbin &&
	                            wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);

	pipeline = wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst;

	/* take decodebin/sinkbin */
	if (name[0] == 'v') {
		wfd_sink_debug("=========== >>>>>>>>>> Received VIDEO pad...");

		MMWFDSINK_PAD_PROBE(wfd_sink, pad, NULL,  NULL);

		gst_pad_add_probe(pad,
		                  GST_PAD_PROBE_TYPE_BUFFER,
		                  _mm_wfd_sink_check_running_time,
		                  (gpointer)wfd_sink,
		                  NULL);

		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_video_pipeline(wfd_sink)) {
			wfd_sink_error("failed to prepare video pipeline....");
			goto ERROR;
		}

		if (wfd_sink->pipeline->v_decodebin && wfd_sink->pipeline->v_decodebin[WFD_SINK_V_D_BIN].gst)
			decodebin = wfd_sink->pipeline->v_decodebin[WFD_SINK_V_D_BIN].gst;
		if (wfd_sink->pipeline->v_sinkbin && wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_BIN].gst)
			sinkbin = wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_BIN].gst;	
	} else if (name[0] == 'a') {
		wfd_sink_debug("=========== >>>>>>>>>> Received AUDIO pad...");

		MMWFDSINK_PAD_PROBE(wfd_sink, pad, NULL,  NULL);

		gst_pad_add_probe(pad,
		                  GST_PAD_PROBE_TYPE_BUFFER,
		                  _mm_wfd_sink_check_running_time,
		                  (gpointer)wfd_sink,
		                  NULL);

		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_audio_pipeline(wfd_sink)) {
			wfd_sink_error("failed to prepare audio pipeline....");
			goto ERROR;
		}

		if (wfd_sink->pipeline->a_decodebin && wfd_sink->pipeline->a_decodebin[WFD_SINK_A_D_BIN].gst)
			decodebin = wfd_sink->pipeline->a_decodebin[WFD_SINK_A_D_BIN].gst;
		if (wfd_sink->pipeline->a_sinkbin && wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_BIN].gst)
			sinkbin = wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_BIN].gst;
	} else {
		wfd_sink_error("unexceptable pad is added!!!");
		return;
	}

	srcpad = gst_object_ref(pad);

	/* add decodebin and link */
	if (decodebin) {
		if (!gst_bin_add(GST_BIN(pipeline), decodebin)) {
			wfd_sink_error("failed to add %s to pipeline",
					GST_STR_NULL(GST_ELEMENT_NAME(decodebin)));
			goto ERROR;
		}

		sinkpad = gst_element_get_static_pad(decodebin, "sink");
		if (!sinkpad) {
			wfd_sink_error("failed to get sink pad from %s",
					GST_STR_NULL(GST_ELEMENT_NAME(decodebin)));
			goto ERROR;
		}

		if (GST_PAD_LINK_OK != gst_pad_link(srcpad, sinkpad)) {
			wfd_sink_error("failed to link %s and %s",
					GST_STR_NULL(GST_PAD_NAME(srcpad)),
					GST_STR_NULL(GST_PAD_NAME(sinkpad)));
			goto ERROR;
		}
		gst_object_unref(GST_OBJECT(srcpad));
		srcpad = NULL;
		gst_object_unref(GST_OBJECT(sinkpad));
		sinkpad = NULL;

		srcpad = gst_element_get_static_pad(decodebin, "src");
		if (!srcpad) {
			wfd_sink_error("failed to get src pad from %s",
					GST_STR_NULL(GST_ELEMENT_NAME(decodebin)));
			goto ERROR;
		}
	} else {
		wfd_sink_warning("going on without decodebin...");
	}

	/* add sinkbin and link */
	if (sinkbin) {
		if (!gst_bin_add(GST_BIN(pipeline), sinkbin)) {
			wfd_sink_error("failed to add %s to pipeline",
					GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));
			goto ERROR;
		}

		sinkpad = gst_element_get_static_pad(sinkbin, "sink");
		if (!sinkpad) {
			wfd_sink_error("failed to get sink pad from %s",
					GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));
			goto ERROR;
		}

		if (GST_PAD_LINK_OK != gst_pad_link(srcpad, sinkpad)) {
			wfd_sink_error("failed to link %s and %s",
					GST_STR_NULL(GST_PAD_NAME(srcpad)),
					GST_STR_NULL(GST_PAD_NAME(sinkpad)));
			goto ERROR;
		}
		gst_object_unref(GST_OBJECT(srcpad));
		srcpad = NULL;
		gst_object_unref(GST_OBJECT(sinkpad));
		sinkpad = NULL;
	} else {
		wfd_sink_error("there is no sinkbin...");
		goto ERROR;
	}


	/* run */
	if (decodebin) {
		if (!gst_element_sync_state_with_parent(GST_ELEMENT_CAST(decodebin))) {
			wfd_sink_error("failed to sync %s state with parent",
				GST_STR_NULL(GST_PAD_NAME(decodebin)));
			goto ERROR;
		}
	}

	if (sinkbin) {
		if (!gst_element_sync_state_with_parent(GST_ELEMENT_CAST(sinkbin))) {
			wfd_sink_error("failed to sync %s state with parent",
				GST_STR_NULL(GST_PAD_NAME(sinkbin)));
			goto ERROR;
		}
	}

	if (name[0] == 'v') {
		MMWFDSINK_GENERATE_DOT_IF_ENABLED(wfd_sink, "video-pad-added-pipeline");
	} else if (name[0] == 'a') {
		MMWFDSINK_GENERATE_DOT_IF_ENABLED(wfd_sink, "audio-pad-added-pipeline");
	}

	MMWFDSINK_FREEIF(name);

	wfd_sink_debug_fleave();

	return;

	/* ERRORS */
ERROR:
	MMWFDSINK_FREEIF(name);

	if (srcpad)
		gst_object_unref(GST_OBJECT(srcpad));
	srcpad = NULL;

	if (sinkpad)
		gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;

	/* need to notify to app */
	MMWFDSINK_POST_MESSAGE(wfd_sink,
	                       MM_ERROR_WFD_INTERNAL,
	                       MMWFDSINK_CURRENT_STATE(wfd_sink));

	return;
}

static void
__mm_wfd_sink_change_av_format(GstElement *wfdsrc, gpointer *need_to_flush, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail(wfd_sink);
	wfd_sink_return_if_fail(need_to_flush);

	if (MMWFDSINK_CURRENT_STATE(wfd_sink) == MM_WFD_SINK_STATE_PLAYING) {
		wfd_sink_debug("need to flush pipeline");
		*need_to_flush = (gpointer) TRUE;
	} else {
		wfd_sink_debug("don't need to flush pipeline");
		*need_to_flush = (gpointer) FALSE;
	}


	wfd_sink_debug_fleave();
}


static void
__mm_wfd_sink_update_stream_info(GstElement *wfdsrc, GstStructure *str, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
	MMWFDSinkStreamInfo *stream_info = NULL;
	gint is_valid_audio_format = FALSE;
	gint is_valid_video_format = FALSE;
	gint audio_codec = MM_WFD_SINK_AUDIO_CODEC_NONE;
	gint video_codec = MM_WFD_SINK_VIDEO_CODEC_NONE;
	gchar *audio_format;
	gchar *video_format;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail(str && GST_IS_STRUCTURE(str));
	wfd_sink_return_if_fail(wfd_sink);

	stream_info = &wfd_sink->stream_info;

	audio_codec = wfd_sink->stream_info.audio_stream_info.codec;
	video_codec = wfd_sink->stream_info.video_stream_info.codec;

	if (gst_structure_has_field(str, "audio_format")) {
		is_valid_audio_format = TRUE;
		audio_format = g_strdup(gst_structure_get_string(str, "audio_format"));
		if (g_strrstr(audio_format, "AAC"))
			stream_info->audio_stream_info.codec = MM_WFD_SINK_AUDIO_CODEC_AAC;
		else if (g_strrstr(audio_format, "AC3"))
			stream_info->audio_stream_info.codec = MM_WFD_SINK_AUDIO_CODEC_AC3;
		else if (g_strrstr(audio_format, "LPCM"))
			stream_info->audio_stream_info.codec = MM_WFD_SINK_AUDIO_CODEC_LPCM;
		else {
			wfd_sink_error("invalid audio format(%s)...", audio_format);
			is_valid_audio_format = FALSE;
		}

		if (is_valid_audio_format == TRUE) {
			if (gst_structure_has_field(str, "audio_rate"))
				gst_structure_get_int(str, "audio_rate", &stream_info->audio_stream_info.sample_rate);
			if (gst_structure_has_field(str, "audio_channels"))
				gst_structure_get_int(str, "audio_channels", &stream_info->audio_stream_info.channels);
			if (gst_structure_has_field(str, "audio_bitwidth"))
				gst_structure_get_int(str, "audio_bitwidth", &stream_info->audio_stream_info.bitwidth);

			if (audio_codec != MM_WFD_SINK_AUDIO_CODEC_NONE) {
				if (audio_codec != stream_info->audio_stream_info.codec) {
					wfd_sink_debug("audio codec is changed...need to change audio decodebin");
				}
			} else {
				WFD_SINK_MANAGER_APPEND_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_PREPARE_A_PIPELINE);
			}

			wfd_sink_debug("audio_format : %s \n \t rate :	%d \n \t channels :  %d \n \t bitwidth :  %d \n \t",
			               audio_format,
			               stream_info->audio_stream_info.sample_rate,
			               stream_info->audio_stream_info.channels,
			               stream_info->audio_stream_info.bitwidth);
		}
	}

	if (gst_structure_has_field(str, "video_format")) {
		is_valid_video_format = TRUE;
		video_format = g_strdup(gst_structure_get_string(str, "video_format"));
		if (!g_strrstr(video_format, "H264")) {
			wfd_sink_error("invalid video format(%s)...", video_format);
			is_valid_video_format = FALSE;
		}

		if (is_valid_video_format == TRUE) {
			stream_info->video_stream_info.codec = MM_WFD_SINK_VIDEO_CODEC_H264;

			if (gst_structure_has_field(str, "video_width"))
				gst_structure_get_int(str, "video_width", &stream_info->video_stream_info.width);
			if (gst_structure_has_field(str, "video_height"))
				gst_structure_get_int(str, "video_height", &stream_info->video_stream_info.height);
			if (gst_structure_has_field(str, "video_framerate"))
				gst_structure_get_int(str, "video_framerate", &stream_info->video_stream_info.frame_rate);

			if (video_codec != MM_WFD_SINK_AUDIO_CODEC_NONE) {
				if (video_codec != stream_info->video_stream_info.codec) {
					wfd_sink_debug("video codec is changed...need to change video decodebin");
				}
			} else {
				WFD_SINK_MANAGER_APPEND_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_PREPARE_V_PIPELINE);
			}

			wfd_sink_debug("video_format : %s \n \t width :  %d \n \t height :  %d \n \t frame_rate :  %d \n \t",
			               video_format,
			               stream_info->video_stream_info.width,
			               stream_info->video_stream_info.height,
			               stream_info->video_stream_info.frame_rate);
		}
	}

	WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink);

	wfd_sink_debug_fleave();
}

static int __mm_wfd_sink_prepare_source(mm_wfd_sink_t *wfd_sink, GstElement *wfdsrc)
{
	GstStructure *audio_param = NULL;
	GstStructure *video_param = NULL;
	GstStructure *hdcp_param = NULL;
	gint hdcp_version = 0;
	gint hdcp_port = 0;
	guint CEA_resolution = 0;
	guint VESA_resolution = 0;
	guint HH_resolution = 0;
	GObjectClass *klass;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(wfdsrc, MM_ERROR_WFD_NOT_INITIALIZED);

	klass = G_OBJECT_GET_CLASS (G_OBJECT (wfdsrc));

	g_object_set(G_OBJECT(wfdsrc), "debug", wfd_sink->ini.set_debug_property, NULL);
	g_object_set(G_OBJECT(wfdsrc), "enable-pad-probe", wfd_sink->ini.enable_wfdsrc_pad_probe, NULL);
	if (g_object_class_find_property (klass, "udp-buffer-size"))
		g_object_set(G_OBJECT(wfdsrc), "udp-buffer-size", 2097152, NULL);
	if (g_object_class_find_property (klass, "do-request"))
		g_object_set(G_OBJECT(wfdsrc), "do-request", wfd_sink->ini.enable_retransmission, NULL);
	if (g_object_class_find_property (klass, "latency"))
		g_object_set(G_OBJECT(wfdsrc), "latency", wfd_sink->ini.jitter_buffer_latency, NULL);

	audio_param = gst_structure_new("audio_param",
	                                "audio_codec", G_TYPE_UINT, wfd_sink->ini.audio_codec,
	                                "audio_latency", G_TYPE_UINT, wfd_sink->ini.audio_latency,
	                                "audio_channels", G_TYPE_UINT, wfd_sink->ini.audio_channel,
	                                "audio_sampling_frequency", G_TYPE_UINT, wfd_sink->ini.audio_sampling_frequency,
	                                NULL);

	CEA_resolution = wfd_sink->ini.video_cea_support;
	VESA_resolution = wfd_sink->ini.video_vesa_support;
	HH_resolution =  wfd_sink->ini.video_hh_support;

	__mm_wfd_sink_prepare_video_resolution(wfd_sink->supportive_resolution,
	                                       &CEA_resolution, &VESA_resolution, &HH_resolution);

	wfd_sink_debug("set video resolution CEA[%x] VESA[%x] HH[%x]", CEA_resolution, VESA_resolution, HH_resolution);

	video_param = gst_structure_new("video_param",
	                                "video_codec", G_TYPE_UINT, wfd_sink->ini.video_codec,
	                                "video_native_resolution", G_TYPE_UINT, wfd_sink->ini.video_native_resolution,
	                                "video_cea_support", G_TYPE_UINT, CEA_resolution,
	                                "video_vesa_support", G_TYPE_UINT, VESA_resolution,
	                                "video_hh_support", G_TYPE_UINT, HH_resolution,
	                                "video_profile", G_TYPE_UINT, wfd_sink->ini.video_profile,
	                                "video_level", G_TYPE_UINT, wfd_sink->ini.video_level,
	                                "video_latency", G_TYPE_UINT, wfd_sink->ini.video_latency,
	                                "video_vertical_resolution", G_TYPE_INT, wfd_sink->ini.video_vertical_resolution,
	                                "video_horizontal_resolution", G_TYPE_INT, wfd_sink->ini.video_horizontal_resolution,
	                                "video_minimum_slicing", G_TYPE_INT, wfd_sink->ini.video_minimum_slicing,
	                                "video_slice_enc_param", G_TYPE_INT, wfd_sink->ini.video_slice_enc_param,
	                                "video_framerate_control_support", G_TYPE_INT, wfd_sink->ini.video_framerate_control_support,
	                                NULL);

	mm_attrs_get_int_by_name(wfd_sink->attrs, "hdcp_version", &hdcp_version);
	mm_attrs_get_int_by_name(wfd_sink->attrs, "hdcp_port", &hdcp_port);
	wfd_sink_debug("set hdcp version %d with %d port", hdcp_version, hdcp_port);

	hdcp_param = gst_structure_new("hdcp_param",
	                               "hdcp_version", G_TYPE_INT, hdcp_version,
	                               "hdcp_port_no", G_TYPE_INT, hdcp_port,
	                               NULL);

	g_object_set(G_OBJECT(wfdsrc), "audio-param", audio_param, NULL);
	g_object_set(G_OBJECT(wfdsrc), "video-param", video_param, NULL);
	g_object_set(G_OBJECT(wfdsrc), "hdcp-param", hdcp_param, NULL);

	g_signal_connect(wfdsrc, "update-media-info",
	                 G_CALLBACK(__mm_wfd_sink_update_stream_info), wfd_sink);

	g_signal_connect(wfdsrc, "change-av-format",
	                 G_CALLBACK(__mm_wfd_sink_change_av_format), wfd_sink);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_prepare_demux(mm_wfd_sink_t *wfd_sink, GstElement *demux)
{
	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(demux, MM_ERROR_WFD_NOT_INITIALIZED);

	g_signal_connect(demux, "pad-added",
	                 G_CALLBACK(__mm_wfd_sink_demux_pad_added),	wfd_sink);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static void __mm_wfd_sink_queue_overrun(GstElement *element, gpointer u_data)
{
	wfd_sink_debug_fenter();

	return_if_fail(element);

	wfd_sink_warning("%s is overrun",
	                 GST_STR_NULL(GST_ELEMENT_NAME(element)));

	wfd_sink_debug_fleave();

	return;
}

static void __mm_wfd_sink_prepare_queue(mm_wfd_sink_t *wfd_sink, GstElement *queue)
{
	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail(wfd_sink);
	wfd_sink_return_if_fail(queue);

	/* set maximum buffer size of queue as 3sec */
	g_object_set(G_OBJECT(queue), "max-size-bytes", 0, NULL);
	g_object_set(G_OBJECT(queue), "max-size-buffers", 0, NULL);
	g_object_set(G_OBJECT(queue), "max-size-time", (guint64)3000000000ULL, NULL);
	g_signal_connect(queue, "overrun",
	                 G_CALLBACK(__mm_wfd_sink_queue_overrun), wfd_sink);

	wfd_sink_debug_fleave();

	return;
}


static int __mm_wfd_sink_create_pipeline(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *mainbin = NULL;
	GList *element_bucket = NULL;
	GstBus	*bus = NULL;
	int i;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);

	/* Create pipeline */
	wfd_sink->pipeline = (MMWFDSinkGstPipelineInfo *) g_malloc0(sizeof(MMWFDSinkGstPipelineInfo));
	if (wfd_sink->pipeline == NULL)
		goto CREATE_ERROR;

	memset(wfd_sink->pipeline, 0, sizeof(MMWFDSinkGstPipelineInfo));

	/* create mainbin */
	mainbin = (MMWFDSinkGstElement *) g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_M_NUM);
	if (mainbin == NULL)
		goto CREATE_ERROR;

	memset(mainbin, 0, sizeof(MMWFDSinkGstElement) * WFD_SINK_M_NUM);

	/* create pipeline */
	mainbin[WFD_SINK_M_PIPE].id = WFD_SINK_M_PIPE;
	mainbin[WFD_SINK_M_PIPE].gst = gst_pipeline_new("wfdsink");
	if (!mainbin[WFD_SINK_M_PIPE].gst) {
		wfd_sink_error("failed to create pipeline");
		goto CREATE_ERROR;
	}

	/* create wfdsrc */
	MMWFDSINK_CREATE_ELEMENT(mainbin, WFD_SINK_M_SRC, wfd_sink->ini.name_of_source, "wfdsink_source", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, mainbin[WFD_SINK_M_SRC].gst,  "src");
	if (mainbin[WFD_SINK_M_SRC].gst) {
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_source(wfd_sink, mainbin[WFD_SINK_M_SRC].gst)) {
			wfd_sink_error("failed to prepare wfdsrc...");
			goto CREATE_ERROR;
		}
	}

	/* create rtpmp2tdepay */
	MMWFDSINK_CREATE_ELEMENT(mainbin, WFD_SINK_M_DEPAY, "rtpmp2tdepay", "wfdsink_depay", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, mainbin[WFD_SINK_M_DEPAY].gst, "src");
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, mainbin[WFD_SINK_M_DEPAY].gst, "sink");

	MMWFDSINK_TS_DATA_DUMP(wfd_sink, mainbin[WFD_SINK_M_DEPAY].gst, "src");

	/* create tsdemuxer*/
	MMWFDSINK_CREATE_ELEMENT(mainbin, WFD_SINK_M_DEMUX, wfd_sink->ini.name_of_tsdemux, "wfdsink_demux", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, mainbin[WFD_SINK_M_DEMUX].gst, "sink");
	if (mainbin[WFD_SINK_M_DEMUX].gst) {
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_demux(wfd_sink, mainbin[WFD_SINK_M_DEMUX].gst)) {
			wfd_sink_error("failed to prepare demux...");
			goto CREATE_ERROR;
		}
	}

	/* adding created elements to pipeline */
	if (!__mm_wfd_sink_gst_element_add_bucket_to_bin(GST_BIN_CAST(mainbin[WFD_SINK_M_PIPE].gst), element_bucket, FALSE)) {
		wfd_sink_error("failed to add elements");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	if (__mm_wfd_sink_gst_element_link_bucket(element_bucket) == -1) {
		wfd_sink_error("failed to link elements");
		goto CREATE_ERROR;
	}

	/* connect bus callback */
	bus = gst_pipeline_get_bus(GST_PIPELINE(mainbin[WFD_SINK_M_PIPE].gst));
	if (!bus) {
		wfd_sink_error("cannot get bus from pipeline.");
		goto CREATE_ERROR;
	}

	/* add bus message callback*/
	gst_bus_add_watch(bus, (GstBusFunc)_mm_wfd_sink_msg_callback, wfd_sink);

	/* set sync handler to get tag synchronously */
	gst_bus_set_sync_handler(bus, _mm_wfd_bus_sync_callback, wfd_sink, NULL);

	g_list_free(element_bucket);
	gst_object_unref(GST_OBJECT(bus));

	/* now we have completed mainbin. take it */
	wfd_sink->pipeline->mainbin = mainbin;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

	/* ERRORS */
CREATE_ERROR:
	wfd_sink_error("ERROR : releasing pipeline");

	if (element_bucket)
		g_list_free(element_bucket);
	element_bucket = NULL;

	/* finished */
	if (bus)
		gst_object_unref(GST_OBJECT(bus));
	bus = NULL;

	/* release element which are not added to bin */
	for (i = 1; i < WFD_SINK_M_NUM; i++) {	/* NOTE : skip pipeline */
		if (mainbin != NULL && mainbin[i].gst) {
			GstObject *parent = NULL;
			parent = gst_element_get_parent(mainbin[i].gst);

			if (!parent) {
				gst_object_unref(GST_OBJECT(mainbin[i].gst));
				mainbin[i].gst = NULL;
			} else {
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release mainbin with it's childs */
	if (mainbin != NULL && mainbin[WFD_SINK_M_PIPE].gst)
		gst_object_unref(GST_OBJECT(mainbin[WFD_SINK_M_PIPE].gst));

	MMWFDSINK_FREEIF(mainbin);

	MMWFDSINK_FREEIF(wfd_sink->pipeline);

	return MM_ERROR_WFD_INTERNAL;
}

int __mm_wfd_sink_link_audio_decodebin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *a_decodebin = NULL;
	MMWFDSinkGstElement *first_element = NULL;
	MMWFDSinkGstElement *last_element = NULL;
	GList *element_bucket = NULL;
	GstPad *sinkpad = NULL;
	GstPad *srcpad = NULL;
	GstPad *ghostpad = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->a_decodebin &&
	                            wfd_sink->pipeline->a_decodebin[WFD_SINK_A_D_BIN].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->audio_decodebin_is_linked) {
		wfd_sink_debug("audio decodebin is already linked... nothing to do");
		return MM_ERROR_NONE;
	}

	/* take audio decodebin */
	a_decodebin = wfd_sink->pipeline->a_decodebin;

	/* check audio queue */
	if (a_decodebin[WFD_SINK_A_D_QUEUE].gst)
		element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_QUEUE]);

	/* check audio hdcp */
	if (a_decodebin[WFD_SINK_A_D_HDCP].gst)
		element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_HDCP]);

	/* check audio codec */
	switch (wfd_sink->stream_info.audio_stream_info.codec) {
		case MM_WFD_SINK_AUDIO_CODEC_LPCM:
			if (a_decodebin[WFD_SINK_A_D_LPCM_CONVERTER].gst)
				element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_LPCM_CONVERTER]);
			if (a_decodebin[WFD_SINK_A_D_LPCM_FILTER].gst) {
				GstCaps *caps = NULL;
				element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_LPCM_FILTER]);
				caps = gst_caps_new_simple("audio/x-raw",
			                           "rate", G_TYPE_INT, wfd_sink->stream_info.audio_stream_info.sample_rate,
			                           "channels", G_TYPE_INT, wfd_sink->stream_info.audio_stream_info.channels,
			                           "format", G_TYPE_STRING, "S16BE", NULL);

				g_object_set(G_OBJECT(a_decodebin[WFD_SINK_A_D_LPCM_CONVERTER].gst), "caps", caps, NULL);
				gst_object_unref(GST_OBJECT(caps));
			}
			break;

		case MM_WFD_SINK_AUDIO_CODEC_AAC:
			if (a_decodebin[WFD_SINK_A_D_AAC_PARSE].gst)
				element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_AAC_PARSE]);
			if (a_decodebin[WFD_SINK_A_D_AAC_DEC].gst)
				element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_AAC_DEC]);
			break;

		case MM_WFD_SINK_AUDIO_CODEC_AC3:
			if (a_decodebin[WFD_SINK_A_D_AC3_PARSE].gst)
				element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_AC3_PARSE]);
			if (a_decodebin[WFD_SINK_A_D_AC3_DEC].gst)
				element_bucket = g_list_append(element_bucket, &a_decodebin[WFD_SINK_A_D_AC3_DEC]);
			break;

		default:
			wfd_sink_error("audio codec is not decied yet. cannot link audio decodebin...");
			return MM_ERROR_WFD_INTERNAL;
			break;
	}

	if (element_bucket == NULL) {
		wfd_sink_error("there are no elements to be linked in the audio decodebin, destroy it");
		if (MM_ERROR_NONE != __mm_wfd_sink_destroy_audio_decodebin(wfd_sink)) {
			wfd_sink_error("failed to destroy audio decodebin");
			goto fail_to_link;
		}
		goto done;
	}

	/* adding elements to audio decodebin */
	if (!__mm_wfd_sink_gst_element_add_bucket_to_bin(GST_BIN_CAST(a_decodebin[WFD_SINK_A_D_BIN].gst), element_bucket, FALSE)) {
		wfd_sink_error("failed to add elements to audio decodebin");
		goto fail_to_link;
	}

	/* linking elements in the bucket by added order. */
	if (__mm_wfd_sink_gst_element_link_bucket(element_bucket) == -1) {
		wfd_sink_error("failed to link elements of the audio decodebin");
		goto fail_to_link;
	}

	/* get first element's sinkpad for creating ghostpad */
	first_element = (MMWFDSinkGstElement *)g_list_first(element_bucket)->data;
	if (!first_element) {
		wfd_sink_error("failed to get first element of the audio decodebin");
		goto fail_to_link;
	}

	sinkpad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!sinkpad) {
		wfd_sink_error("failed to get sink pad from element(%s)",
			GST_STR_NULL(GST_ELEMENT_NAME(first_element->gst)));
		goto fail_to_link;
	}

	ghostpad = gst_ghost_pad_new("sink", sinkpad);
	if (!ghostpad) {
		wfd_sink_error("failed to create ghostpad of audio decodebin");
		goto fail_to_link;
	}

	if (FALSE == gst_element_add_pad(a_decodebin[WFD_SINK_A_D_BIN].gst, ghostpad)) {
		wfd_sink_error("failed to add ghostpad to audio decodebin");
		goto fail_to_link;
	}
	gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;


	/* get last element's src for creating ghostpad */
	last_element = (MMWFDSinkGstElement *)g_list_last(element_bucket)->data;
	if (!last_element) {
		wfd_sink_error("failed to get last element of the audio decodebin");
		goto fail_to_link;
	}

	srcpad = gst_element_get_static_pad(GST_ELEMENT(last_element->gst), "src");
	if (!srcpad) {
		wfd_sink_error("failed to get src pad from element(%s)",
			GST_STR_NULL(GST_ELEMENT_NAME(last_element->gst)));
		goto fail_to_link;
	}

	ghostpad = gst_ghost_pad_new("src", srcpad);
	if (!ghostpad) {
		wfd_sink_error("failed to create ghostpad of audio decodebin");
		goto fail_to_link;
	}

	if (FALSE == gst_element_add_pad(a_decodebin[WFD_SINK_A_D_BIN].gst, ghostpad)) {
		wfd_sink_error("failed to add ghostpad to audio decodebin");
		goto fail_to_link;
	}
	gst_object_unref(GST_OBJECT(srcpad));
	srcpad = NULL;

	g_list_free(element_bucket);

done:
	wfd_sink->audio_decodebin_is_linked = TRUE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

	/* ERRORS*/
fail_to_link:
	if (srcpad)
		gst_object_unref(GST_OBJECT(srcpad));
	srcpad = NULL;

	if (sinkpad)
		gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;

	g_list_free(element_bucket);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_prepare_audiosink(mm_wfd_sink_t *wfd_sink, GstElement *audio_sink)
{
	wfd_sink_debug_fenter();

	/* check audiosink is created */
	wfd_sink_return_val_if_fail(audio_sink, MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	g_object_set(G_OBJECT(audio_sink), "provide-clock", FALSE,  NULL);
	g_object_set(G_OBJECT(audio_sink), "buffer-time", 100000LL, NULL);
	g_object_set(G_OBJECT(audio_sink), "slave-method", 2,  NULL);
	g_object_set(G_OBJECT(audio_sink), "async", wfd_sink->ini.audio_sink_async,  NULL);
	g_object_set(G_OBJECT(audio_sink), "ts-offset", (gint64)wfd_sink->ini.sink_ts_offset, NULL);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int  __mm_wfd_sink_destroy_audio_decodebin(mm_wfd_sink_t *wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement *a_decodebin = NULL;
	GstObject *parent = NULL;
	int i;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline &&
	    wfd_sink->pipeline->a_decodebin &&
	    wfd_sink->pipeline->a_decodebin[WFD_SINK_A_D_BIN].gst) {
		a_decodebin = wfd_sink->pipeline->a_decodebin;
	} else {
		wfd_sink_debug("audio decodebin is not created, nothing to destroy");
		return MM_ERROR_NONE;
	}

	parent = gst_element_get_parent(a_decodebin[WFD_SINK_A_D_BIN].gst);
	if (!parent) {
		wfd_sink_debug("audio decodebin has no parent.. need to relase by itself");

		if (GST_STATE(a_decodebin[WFD_SINK_A_D_BIN].gst) >= GST_STATE_READY) {
			wfd_sink_debug("try to change state of audio decodebin to NULL");
			ret = gst_element_set_state(a_decodebin[WFD_SINK_A_D_BIN].gst, GST_STATE_NULL);
			if (ret != GST_STATE_CHANGE_SUCCESS) {
				wfd_sink_error("failed to change state of audio decodebin to NULL");
				return MM_ERROR_WFD_INTERNAL;
			}
		}

		/* release element which are not added to bin */
		for (i = 1; i < WFD_SINK_A_D_NUM; i++) {	/* NOTE : skip bin */
			if (a_decodebin[i].gst) {
				parent = gst_element_get_parent(a_decodebin[i].gst);
				if (!parent) {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(a_decodebin[i].gst)),
					               ((GObject *) a_decodebin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(a_decodebin[i].gst));
					a_decodebin[i].gst = NULL;
				} else {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(a_decodebin[i].gst)),
					               ((GObject *) a_decodebin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(parent));
				}
			}
		}

		/* release audio decodebin with it's childs */
		if (a_decodebin[WFD_SINK_A_D_BIN].gst)
			gst_object_unref(GST_OBJECT(a_decodebin[WFD_SINK_A_D_BIN].gst));

	} else {
		wfd_sink_debug("audio decodebin has parent(%s), unref it ",
		               GST_STR_NULL(GST_OBJECT_NAME(GST_OBJECT(parent))));

		gst_object_unref(GST_OBJECT(parent));
	}

	wfd_sink->audio_decodebin_is_linked = FALSE;

	MMWFDSINK_FREEIF(wfd_sink->pipeline->a_decodebin);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_create_audio_decodebin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *a_decodebin = NULL;
	gint audio_codec = WFD_AUDIO_UNKNOWN;
	GList *element_bucket = NULL;
	gboolean link = TRUE;
	gint i = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* check audio decodebin could be linked now */
	switch (wfd_sink->stream_info.audio_stream_info.codec) {
		case MM_WFD_SINK_AUDIO_CODEC_AAC:
			audio_codec = WFD_AUDIO_AAC;
			link = TRUE;
			break;
		case MM_WFD_SINK_AUDIO_CODEC_AC3:
			audio_codec = WFD_AUDIO_AC3;
			link = TRUE;
			break;
		case MM_WFD_SINK_AUDIO_CODEC_LPCM:
			audio_codec = WFD_AUDIO_LPCM;
			link = TRUE;
			break;
		case MM_WFD_SINK_AUDIO_CODEC_NONE:
		default:
			wfd_sink_debug("audio decodebin could NOT be linked now, just create");
			audio_codec = wfd_sink->ini.audio_codec;
			link = FALSE;
			break;
	}

	/* alloc handles */
	a_decodebin = (MMWFDSinkGstElement *)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_A_D_NUM);
	if (!a_decodebin) {
		wfd_sink_error("failed to allocate memory for audio decodebin");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	memset(a_decodebin, 0, sizeof(MMWFDSinkGstElement) * WFD_SINK_A_D_NUM);

	/* create audio decodebin */
	a_decodebin[WFD_SINK_A_D_BIN].id = WFD_SINK_A_D_BIN;
	a_decodebin[WFD_SINK_A_D_BIN].gst = gst_bin_new("audio_deocebin");
	if (!a_decodebin[WFD_SINK_A_D_BIN].gst) {
		wfd_sink_error("failed to create audio decodebin");
		goto CREATE_ERROR;
	}

	/* create queue */
	MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_QUEUE, "queue", "audio_queue", FALSE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_QUEUE].gst,  "sink");
	if (a_decodebin[WFD_SINK_A_D_QUEUE].gst)
		__mm_wfd_sink_prepare_queue(wfd_sink, a_decodebin[WFD_SINK_A_D_QUEUE].gst);

	/* create hdcp */
	MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_HDCP, wfd_sink->ini.name_of_audio_hdcp, "audio_hdcp", FALSE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_HDCP].gst,  "sink");

	/* create codec */
	audio_codec = wfd_sink->ini.audio_codec;
	if (audio_codec & WFD_AUDIO_LPCM) {
		/* create LPCM converter */
		MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_LPCM_CONVERTER, wfd_sink->ini.name_of_lpcm_converter, "audio_lpcm_convert", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_LPCM_CONVERTER].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_LPCM_CONVERTER].gst,  "src");

		/* create LPCM filter */
		MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_LPCM_FILTER, wfd_sink->ini.name_of_lpcm_filter, "audio_lpcm_filter", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_LPCM_FILTER].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_LPCM_FILTER].gst,  "src");
	}

	if (audio_codec & WFD_AUDIO_AAC) {
		/* create AAC parse  */
		MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_AAC_PARSE, wfd_sink->ini.name_of_aac_parser, "audio_aac_parser", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AAC_PARSE].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AAC_PARSE].gst,  "src");

		/* create AAC decoder  */
		MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_AAC_DEC, wfd_sink->ini.name_of_aac_decoder, "audio_aac_dec", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AAC_DEC].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AAC_DEC].gst,  "src");
	}

	if (audio_codec & WFD_AUDIO_AC3) {
		/* create AC3 parser  */
		MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_AC3_PARSE, wfd_sink->ini.name_of_ac3_parser, "audio_ac3_parser", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AC3_PARSE].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AC3_PARSE].gst,  "src");

		/* create AC3 decoder  */
		MMWFDSINK_CREATE_ELEMENT(a_decodebin, WFD_SINK_A_D_AC3_DEC, wfd_sink->ini.name_of_ac3_decoder, "audio_ac3_dec", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AC3_DEC].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_decodebin[WFD_SINK_A_D_AC3_DEC].gst,  "src");
	}

	g_list_free(element_bucket);

	/* take it */
	wfd_sink->pipeline->a_decodebin = a_decodebin;

	/* link audio decodebin if audio codec is fixed */
	if (link) {
		if (MM_ERROR_NONE != __mm_wfd_sink_link_audio_decodebin(wfd_sink)) {
			wfd_sink_error("failed to link audio decodebin, destroy audio decodebin");
			__mm_wfd_sink_destroy_audio_decodebin(wfd_sink);
			return MM_ERROR_WFD_INTERNAL;
		}
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

CREATE_ERROR:
	wfd_sink_error("failed to create audio decodebin, release all");

	g_list_free(element_bucket);

	/* release element which are not added to bin */
	for (i = 1; i < WFD_SINK_A_D_NUM; i++) {	/* NOTE : skip bin */
		if (a_decodebin != NULL && a_decodebin[i].gst) {
			GstObject *parent = NULL;
			parent = gst_element_get_parent(a_decodebin[i].gst);

			if (!parent) {
				gst_object_unref(GST_OBJECT(a_decodebin[i].gst));
				a_decodebin[i].gst = NULL;
			} else {
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release audioo decodebin with it's childs */
	if (a_decodebin != NULL && a_decodebin[WFD_SINK_A_D_BIN].gst)
		gst_object_unref(GST_OBJECT(a_decodebin[WFD_SINK_A_D_BIN].gst));

	MMWFDSINK_FREEIF(a_decodebin);

	return MM_ERROR_WFD_INTERNAL;
}

static int  __mm_wfd_sink_destroy_audio_sinkbin(mm_wfd_sink_t *wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement *a_sinkbin = NULL;
	GstObject *parent = NULL;
	int i;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline &&
	    wfd_sink->pipeline->a_sinkbin &&
	    wfd_sink->pipeline->a_sinkbin[WFD_SINK_A_S_BIN].gst) {
		a_sinkbin = wfd_sink->pipeline->a_sinkbin;
	} else {
		wfd_sink_debug("audio sinkbin is not created, nothing to destroy");
		return MM_ERROR_NONE;
	}


	parent = gst_element_get_parent(a_sinkbin[WFD_SINK_A_S_BIN].gst);
	if (!parent) {
		wfd_sink_debug("audio decodebin has no parent.. need to relase by itself");

		if (GST_STATE(a_sinkbin[WFD_SINK_A_S_BIN].gst) >= GST_STATE_READY) {
			wfd_sink_debug("try to change state of audio decodebin to NULL");
			ret = gst_element_set_state(a_sinkbin[WFD_SINK_A_S_BIN].gst, GST_STATE_NULL);
			if (ret != GST_STATE_CHANGE_SUCCESS) {
				wfd_sink_error("failed to change state of audio decodebin to NULL");
				return MM_ERROR_WFD_INTERNAL;
			}
		}

		/* release element which are not added to bin */
		for (i = 1; i < WFD_SINK_A_S_NUM; i++) {	/* NOTE : skip bin */
			if (a_sinkbin[i].gst) {
				parent = gst_element_get_parent(a_sinkbin[i].gst);
				if (!parent) {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(a_sinkbin[i].gst)),
					               ((GObject *) a_sinkbin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(a_sinkbin[i].gst));
					a_sinkbin[i].gst = NULL;
				} else {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(a_sinkbin[i].gst)),
					               ((GObject *) a_sinkbin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(parent));
				}
			}
		}

		/* release audio decodebin with it's childs */
		if (a_sinkbin[WFD_SINK_A_S_BIN].gst)
			gst_object_unref(GST_OBJECT(a_sinkbin[WFD_SINK_A_S_BIN].gst));

	} else {
		wfd_sink_debug("audio sinkbin has parent(%s), unref it ",
		               GST_STR_NULL(GST_OBJECT_NAME(GST_OBJECT(parent))));

		gst_object_unref(GST_OBJECT(parent));
	}

	MMWFDSINK_FREEIF(wfd_sink->pipeline->a_sinkbin);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_create_audio_sinkbin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *a_sinkbin = NULL;
	MMWFDSinkGstElement *first_element = NULL;
	GList *element_bucket = NULL;
	GstPad *ghostpad = NULL;
	GstPad *pad = NULL;
	gint i = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* alloc handles */
	a_sinkbin = (MMWFDSinkGstElement *)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_A_S_NUM);
	if (!a_sinkbin) {
		wfd_sink_error("failed to allocate memory for audio sinkbin");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	memset(a_sinkbin, 0, sizeof(MMWFDSinkGstElement) * WFD_SINK_A_S_NUM);

	/* create audio sinkbin */
	a_sinkbin[WFD_SINK_A_S_BIN].id = WFD_SINK_A_S_BIN;
	a_sinkbin[WFD_SINK_A_S_BIN].gst = gst_bin_new("audio_sinkbin");
	if (!a_sinkbin[WFD_SINK_A_S_BIN].gst) {
		wfd_sink_error("failed to create audio sinkbin");
		goto CREATE_ERROR;
	}

	/* create resampler */
	MMWFDSINK_CREATE_ELEMENT(a_sinkbin, WFD_SINK_A_S_RESAMPLER, wfd_sink->ini.name_of_audio_resampler, "audio_resampler", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_sinkbin[WFD_SINK_A_S_RESAMPLER].gst,  "sink");
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_sinkbin[WFD_SINK_A_S_RESAMPLER].gst,  "src");

	/* create volume */
	MMWFDSINK_CREATE_ELEMENT(a_sinkbin, WFD_SINK_A_S_VOLUME, wfd_sink->ini.name_of_audio_volume, "audio_volume", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_sinkbin[WFD_SINK_A_S_VOLUME].gst,  "sink");
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_sinkbin[WFD_SINK_A_S_VOLUME].gst,  "src");

	/* create sink */
	MMWFDSINK_CREATE_ELEMENT(a_sinkbin, WFD_SINK_A_S_SINK, wfd_sink->ini.name_of_audio_sink, "audio_sink", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, a_sinkbin[WFD_SINK_A_S_SINK].gst,  "sink");
	if (a_sinkbin[WFD_SINK_A_S_SINK].gst) {
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_audiosink(wfd_sink, a_sinkbin[WFD_SINK_A_S_SINK].gst)) {
			wfd_sink_error("failed to set audio sink property....");
			goto CREATE_ERROR;
		}
	}

	/* adding created elements to audio sinkbin */
	if (!__mm_wfd_sink_gst_element_add_bucket_to_bin(GST_BIN_CAST(a_sinkbin[WFD_SINK_A_S_BIN].gst), element_bucket, FALSE)) {
		wfd_sink_error("failed to add elements to audio sinkbin");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	if (__mm_wfd_sink_gst_element_link_bucket(element_bucket) == -1) {
		wfd_sink_error("failed to link elements fo the audio sinkbin");
		goto CREATE_ERROR;
	}

	/* get first element's of the audio sinkbin */
	first_element = (MMWFDSinkGstElement *)g_list_first(element_bucket)->data;
	if (!first_element) {
		wfd_sink_error("failed to get first element of the audio sinkbin");
		goto CREATE_ERROR;
	}

	/* get first element's sinkpad for creating ghostpad */
	pad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!pad) {
		wfd_sink_error("failed to get sink pad from element(%s)",
			GST_STR_NULL(GST_ELEMENT_NAME(first_element->gst)));
		goto CREATE_ERROR;
	}

	ghostpad = gst_ghost_pad_new("sink", pad);
	if (!ghostpad) {
		wfd_sink_error("failed to create ghostpad of audio sinkbin");
		goto CREATE_ERROR;
	}

	if (FALSE == gst_element_add_pad(a_sinkbin[WFD_SINK_A_S_BIN].gst, ghostpad)) {
		wfd_sink_error("failed to add ghostpad to audio sinkbin");
		goto CREATE_ERROR;
	}
	gst_object_unref(GST_OBJECT(pad));

	g_list_free(element_bucket);

	/* take it */
	wfd_sink->pipeline->a_sinkbin = a_sinkbin;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

CREATE_ERROR:
	wfd_sink_error("failed to create audio sinkbin, releasing all");

	if (pad)
		gst_object_unref(GST_OBJECT(pad));
	pad = NULL;

	if (ghostpad)
		gst_object_unref(GST_OBJECT(ghostpad));
	ghostpad = NULL;

	if (element_bucket)
		g_list_free(element_bucket);
	element_bucket = NULL;

	/* release element which are not added to bin */
	for (i = 1; i < WFD_SINK_A_S_NUM; i++) {	/* NOTE : skip bin */
		if (a_sinkbin != NULL && a_sinkbin[i].gst) {
			GstObject *parent = NULL;
			parent = gst_element_get_parent(a_sinkbin[i].gst);

			if (!parent) {
				gst_object_unref(GST_OBJECT(a_sinkbin[i].gst));
				a_sinkbin[i].gst = NULL;
			} else {
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release audio sinkbin with it's childs */
	if (a_sinkbin != NULL && a_sinkbin[WFD_SINK_A_S_BIN].gst)
		gst_object_unref(GST_OBJECT(a_sinkbin[WFD_SINK_A_S_BIN].gst));

	MMWFDSINK_FREEIF(a_sinkbin);

	return MM_ERROR_WFD_INTERNAL;
}

int __mm_wfd_sink_link_video_decodebin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement 	*v_decodebin = NULL;
	MMWFDSinkGstElement *first_element = NULL;
	MMWFDSinkGstElement *last_element = NULL;
	GList *element_bucket = NULL;
	GstPad *sinkpad = NULL;
	GstPad *srcpad = NULL;
	GstPad *ghostpad = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline &&
	                            wfd_sink->pipeline->v_decodebin &&
	                            wfd_sink->pipeline->v_decodebin[WFD_SINK_V_D_BIN].gst,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->video_decodebin_is_linked) {
		wfd_sink_debug("video decodebin is already linked... nothing to do");
		return MM_ERROR_NONE;
	}

	/* take video decodebin */
	v_decodebin = wfd_sink->pipeline->v_decodebin;

	/* check video queue */
	if (v_decodebin[WFD_SINK_V_D_QUEUE].gst)
		element_bucket = g_list_append(element_bucket, &v_decodebin[WFD_SINK_V_D_QUEUE]);

	/* check video hdcp */
	if (v_decodebin[WFD_SINK_V_D_HDCP].gst)
		element_bucket = g_list_append(element_bucket, &v_decodebin[WFD_SINK_V_D_HDCP]);

	/* check video codec */
	switch (wfd_sink->stream_info.video_stream_info.codec) {
		case MM_WFD_SINK_VIDEO_CODEC_H264:
			if (v_decodebin[WFD_SINK_V_D_PARSE].gst)
				element_bucket = g_list_append(element_bucket, &v_decodebin[WFD_SINK_V_D_PARSE]);
			if (v_decodebin[WFD_SINK_V_D_CAPSSETTER].gst) {
				GstCaps *caps = NULL;

				element_bucket = g_list_append(element_bucket, &v_decodebin[WFD_SINK_V_D_CAPSSETTER]);
				caps = gst_caps_new_simple("video/x-h264",
				                           "width", G_TYPE_INT, wfd_sink->stream_info.video_stream_info.width,
				                           "height", G_TYPE_INT, wfd_sink->stream_info.video_stream_info.height,
				                           "framerate", GST_TYPE_FRACTION, wfd_sink->stream_info.video_stream_info.frame_rate, 1, NULL);
				g_object_set(G_OBJECT(v_decodebin[WFD_SINK_V_D_CAPSSETTER].gst), "caps", caps, NULL);
				gst_object_unref(GST_OBJECT(caps));
			}
			if (v_decodebin[WFD_SINK_V_D_DEC].gst)
				element_bucket = g_list_append(element_bucket, &v_decodebin[WFD_SINK_V_D_DEC]);
			break;

		default:
			wfd_sink_error("video codec is not decied yet. cannot link video decpdebin...");
			return MM_ERROR_WFD_INTERNAL;
			break;
	}

	if (element_bucket == NULL) {
		wfd_sink_error("there are no elements to be linked in the video decodebin, destroy it");
		if (MM_ERROR_NONE != __mm_wfd_sink_destroy_video_decodebin(wfd_sink)) {
			wfd_sink_error("failed to destroy video decodebin");
			goto fail_to_link;
		}
		goto done;
	}

	/* adding elements to video decodebin */
	if (!__mm_wfd_sink_gst_element_add_bucket_to_bin(GST_BIN_CAST(v_decodebin[WFD_SINK_V_D_BIN].gst), element_bucket, FALSE)) {
		wfd_sink_error("failed to add elements to video decodebin");
		goto fail_to_link;
	}

	/* linking elements in the bucket by added order. */
	if (__mm_wfd_sink_gst_element_link_bucket(element_bucket) == -1) {
		wfd_sink_error("failed to link elements of the video decodebin");
		goto fail_to_link;
	}

	/* get first element's sinkpad for creating ghostpad */
	first_element = (MMWFDSinkGstElement *)g_list_first(element_bucket)->data;
	if (!first_element) {
		wfd_sink_error("failed to get first element of the video decodebin");
		goto fail_to_link;
	}

	sinkpad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!sinkpad) {
		wfd_sink_error("failed to get sink pad from element(%s)",
			GST_STR_NULL(GST_ELEMENT_NAME(first_element->gst)));
		goto fail_to_link;
	}

	ghostpad = gst_ghost_pad_new("sink", sinkpad);
	if (!ghostpad) {
		wfd_sink_error("failed to create ghostpad of video decodebin");
		goto fail_to_link;
	}

	if (FALSE == gst_element_add_pad(v_decodebin[WFD_SINK_V_D_BIN].gst, ghostpad)) {
		wfd_sink_error("failed to add ghostpad to video decodebin");
		goto fail_to_link;
	}
	gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;


	/* get last element's src for creating ghostpad */
	last_element = (MMWFDSinkGstElement *)g_list_last(element_bucket)->data;
	if (!last_element) {
		wfd_sink_error("failed to get last element of the video decodebin");
		goto fail_to_link;
	}

	srcpad = gst_element_get_static_pad(GST_ELEMENT(last_element->gst), "src");
	if (!srcpad) {
		wfd_sink_error("failed to get src pad from element(%s)",
			GST_STR_NULL(GST_ELEMENT_NAME(last_element->gst)));
		goto fail_to_link;
	}

	ghostpad = gst_ghost_pad_new("src", srcpad);
	if (!ghostpad) {
		wfd_sink_error("failed to create ghostpad of video decodebin");
		goto fail_to_link;
	}

	if (FALSE == gst_element_add_pad(v_decodebin[WFD_SINK_V_D_BIN].gst, ghostpad)) {
		wfd_sink_error("failed to add ghostpad to video decodebin");
		goto fail_to_link;
	}
	gst_object_unref(GST_OBJECT(srcpad));
	srcpad = NULL;

	g_list_free(element_bucket);

done:
	wfd_sink->video_decodebin_is_linked = TRUE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

	/* ERRORS*/
fail_to_link:
	if (srcpad != NULL)
		gst_object_unref(GST_OBJECT(srcpad));
	srcpad = NULL;

	if (sinkpad != NULL)
		gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;

	g_list_free(element_bucket);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_prepare_videodec(mm_wfd_sink_t *wfd_sink, GstElement *video_dec)
{
	wfd_sink_debug_fenter();

	/* check video decoder is created */
	wfd_sink_return_val_if_fail(video_dec, MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail(wfd_sink && wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_prepare_videosink(mm_wfd_sink_t *wfd_sink, GstElement *video_sink)
{
	gboolean visible = TRUE;
	gint surface_type = MM_DISPLAY_SURFACE_X;

	wfd_sink_debug_fenter();

	/* check videosink is created */
	wfd_sink_return_val_if_fail(video_sink, MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail(wfd_sink && wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);

	/* update display surface */
	mm_attrs_get_int_by_name(wfd_sink->attrs, "display_surface_type", &surface_type);
	wfd_sink_debug("check display surface type attribute: %d", surface_type);
	mm_attrs_get_int_by_name(wfd_sink->attrs, "display_visible", &visible);
	wfd_sink_debug("check display visible attribute: %d", visible);

	/* configuring display */
	switch (surface_type) {
		case MM_DISPLAY_SURFACE_EVAS: {
				void *object = NULL;
				gint scaling = 0;

				/* common case if using evas surface */
				mm_attrs_get_data_by_name(wfd_sink->attrs, "display_overlay", &object);
				mm_attrs_get_int_by_name(wfd_sink->attrs, "display_evas_do_scaling", &scaling);
				if (object) {
					wfd_sink_debug("set video param : evas-object %x", object);
					g_object_set(G_OBJECT(video_sink), "evas-object", object, NULL);
				} else {
					wfd_sink_error("no evas object");
					return MM_ERROR_WFD_INTERNAL;
				}
			}
			break;

		case MM_DISPLAY_SURFACE_X: {
				void *object = NULL;
				Evas_Object *obj = NULL;
				const char *object_type = NULL;
				unsigned int g_xwin = 0;

				/* x surface */
				mm_attrs_get_data_by_name(wfd_sink->attrs, "display_overlay", &object);

				if(object != NULL) {
					obj = (Evas_Object *)object;
					object_type = evas_object_type_get(obj);

					wfd_sink_debug("window object type : %s", object_type);
					
					if (!strcmp(object_type, "elm_win"))
						g_xwin = elm_win_xwindow_get(obj);
				}

				wfd_sink_debug("xid = %lu", g_xwin);
				gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink), g_xwin);

			}
			break;

		case MM_DISPLAY_SURFACE_NULL: {
				/* do nothing */
				wfd_sink_error("Not Supported Surface.");
				return MM_ERROR_WFD_INTERNAL;
			}
			break;
		default: {
				wfd_sink_error("Not Supported Surface.(default case)");
				return MM_ERROR_WFD_INTERNAL;
			}
			break;
	}

	g_object_set(G_OBJECT(video_sink), "qos", FALSE, NULL);
	g_object_set(G_OBJECT(video_sink), "async", wfd_sink->ini.video_sink_async, NULL);
	g_object_set(G_OBJECT(video_sink), "max-lateness", (gint64)wfd_sink->ini.video_sink_max_lateness, NULL);
	g_object_set(G_OBJECT(video_sink), "visible", visible, NULL);
	g_object_set(G_OBJECT(video_sink), "ts-offset", (gint64)(wfd_sink->ini.sink_ts_offset), NULL);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_destroy_video_decodebin(mm_wfd_sink_t *wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement *v_decodebin = NULL;
	GstObject *parent = NULL;
	int i;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline &&
	    wfd_sink->pipeline->v_decodebin &&
	    wfd_sink->pipeline->v_decodebin[WFD_SINK_V_D_BIN].gst) {
		v_decodebin = wfd_sink->pipeline->v_decodebin;
	} else {
		wfd_sink_debug("video decodebin is not created, nothing to destroy");
		return MM_ERROR_NONE;
	}


	parent = gst_element_get_parent(v_decodebin[WFD_SINK_V_D_BIN].gst);
	if (!parent) {
		wfd_sink_debug("video decodebin has no parent.. need to relase by itself");

		if (GST_STATE(v_decodebin[WFD_SINK_V_D_BIN].gst) >= GST_STATE_READY) {
			wfd_sink_debug("try to change state of video decodebin to NULL");
			ret = gst_element_set_state(v_decodebin[WFD_SINK_V_D_BIN].gst, GST_STATE_NULL);
			if (ret != GST_STATE_CHANGE_SUCCESS) {
				wfd_sink_error("failed to change state of video decodebin to NULL");
				return MM_ERROR_WFD_INTERNAL;
			}
		}
		/* release element which are not added to bin */
		for (i = 1; i < WFD_SINK_V_D_NUM; i++) {	/* NOTE : skip bin */
			if (v_decodebin[i].gst) {
				parent = gst_element_get_parent(v_decodebin[i].gst);
				if (!parent) {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(v_decodebin[i].gst)),
					               ((GObject *) v_decodebin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(v_decodebin[i].gst));
					v_decodebin[i].gst = NULL;
				} else {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(v_decodebin[i].gst)),
					               ((GObject *) v_decodebin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(parent));
				}
			}
		}
		/* release video decodebin with it's childs */
		if (v_decodebin[WFD_SINK_V_D_BIN].gst) {
			gst_object_unref(GST_OBJECT(v_decodebin[WFD_SINK_V_D_BIN].gst));
			wfd_sink_debug("unref %s(current ref %d)",
				               GST_STR_NULL(GST_ELEMENT_NAME(v_decodebin[WFD_SINK_V_D_BIN].gst)),
				               ((GObject *)v_decodebin[WFD_SINK_V_D_BIN].gst)->ref_count);
		}
	} else {
		wfd_sink_debug("video decodebin has parent(%s), unref it",
		               GST_STR_NULL(GST_OBJECT_NAME(GST_OBJECT(parent))));

		gst_object_unref(GST_OBJECT(parent));
	}

	wfd_sink->video_decodebin_is_linked = FALSE;

	MMWFDSINK_FREEIF(wfd_sink->pipeline->v_decodebin);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_create_video_decodebin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *v_decodebin = NULL;
	guint video_codec = WFD_VIDEO_UNKNOWN;
	GList *element_bucket = NULL;
	gboolean link = TRUE;
	gint i = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline->v_decodebin) {
		wfd_sink_debug("video decodebin is already created... nothing to do");
		return MM_ERROR_NONE;
	}

	/* check audio decodebin could be linked now */
	switch (wfd_sink->stream_info.video_stream_info.codec) {
		case MM_WFD_SINK_VIDEO_CODEC_H264:
			video_codec = WFD_VIDEO_H264;
			link = TRUE;
			break;
		case MM_WFD_SINK_VIDEO_CODEC_NONE:
		default:
			wfd_sink_debug("video decodebin could NOT be linked now, just create");
			video_codec = wfd_sink->ini.video_codec;
			link = FALSE;
			break;
	}

	/* alloc handles */
	v_decodebin = (MMWFDSinkGstElement *)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_V_D_NUM);
	if (!v_decodebin) {
		wfd_sink_error("failed to allocate memory for video decodebin");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	memset(v_decodebin, 0, sizeof(MMWFDSinkGstElement) * WFD_SINK_V_D_NUM);

	/* create video decodebin */
	v_decodebin[WFD_SINK_V_D_BIN].id = WFD_SINK_V_D_BIN;
	v_decodebin[WFD_SINK_V_D_BIN].gst = gst_bin_new("video_decodebin");
	if (!v_decodebin[WFD_SINK_V_D_BIN].gst) {
		wfd_sink_error("failed to create video decodebin");
		goto CREATE_ERROR;
	}

	/* create queue */
	MMWFDSINK_CREATE_ELEMENT(v_decodebin, WFD_SINK_V_D_QUEUE, "queue", "video_queue", FALSE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_QUEUE].gst,  "sink");
	if (v_decodebin[WFD_SINK_V_D_QUEUE].gst)
		__mm_wfd_sink_prepare_queue(wfd_sink, v_decodebin[WFD_SINK_V_D_QUEUE].gst);

	/* create hdcp */
	MMWFDSINK_CREATE_ELEMENT(v_decodebin, WFD_SINK_V_D_HDCP, wfd_sink->ini.name_of_video_hdcp, "video_hdcp", FALSE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_HDCP].gst,  "sink");
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_HDCP].gst,  "src");

	if (video_codec & WFD_VIDEO_H264) {
		/* create parser */
		MMWFDSINK_CREATE_ELEMENT(v_decodebin, WFD_SINK_V_D_PARSE, wfd_sink->ini.name_of_video_parser, "video_parser", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_PARSE].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_PARSE].gst,  "src");

		/* create capssetter */
		MMWFDSINK_CREATE_ELEMENT(v_decodebin, WFD_SINK_V_D_CAPSSETTER, wfd_sink->ini.name_of_video_capssetter, "video_capssetter", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_CAPSSETTER].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_CAPSSETTER].gst,  "src");

		/* create dec */
		MMWFDSINK_CREATE_ELEMENT(v_decodebin, WFD_SINK_V_D_DEC, wfd_sink->ini.name_of_video_decoder, "video_dec", FALSE);
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_DEC].gst,  "sink");
		MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_decodebin[WFD_SINK_V_D_DEC].gst,  "src");
		if (v_decodebin[WFD_SINK_V_D_DEC].gst) {
			if (MM_ERROR_NONE != __mm_wfd_sink_prepare_videodec(wfd_sink, v_decodebin[WFD_SINK_V_D_DEC].gst)) {
				wfd_sink_error("failed to set video decoder property...");
				goto CREATE_ERROR;
			}
		}
	}

	g_list_free(element_bucket);

	/* take it */
	wfd_sink->pipeline->v_decodebin = v_decodebin;

	/* link video decodebin if video codec is fixed */
	if (link) {
		if (MM_ERROR_NONE != __mm_wfd_sink_link_video_decodebin(wfd_sink)) {
			wfd_sink_error("failed to link video decodebin, destroy video decodebin");
			__mm_wfd_sink_destroy_video_decodebin(wfd_sink);
			return MM_ERROR_WFD_INTERNAL;
		}
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

	/* ERRORS */
CREATE_ERROR:
	wfd_sink_error("failed to create video decodebin, releasing all");

	g_list_free(element_bucket);

	/* release element which are not added to bin */
	for (i = 1; i < WFD_SINK_V_D_NUM; i++) {	/* NOTE : skip bin */
		if (v_decodebin != NULL && v_decodebin[i].gst) {
			GstObject *parent = NULL;
			parent = gst_element_get_parent(v_decodebin[i].gst);

			if (!parent) {
				gst_object_unref(GST_OBJECT(v_decodebin[i].gst));
				v_decodebin[i].gst = NULL;
			} else {
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release video decodebin with it's childs */
	if (v_decodebin != NULL && v_decodebin[WFD_SINK_V_D_BIN].gst)
		gst_object_unref(GST_OBJECT(v_decodebin[WFD_SINK_V_D_BIN].gst));

	MMWFDSINK_FREEIF(v_decodebin);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_destroy_video_sinkbin(mm_wfd_sink_t *wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement *v_sinkbin = NULL;
	GstObject *parent = NULL;
	int i;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline &&
	    wfd_sink->pipeline->v_sinkbin &&
	    wfd_sink->pipeline->v_sinkbin[WFD_SINK_V_S_BIN].gst) {
		v_sinkbin = wfd_sink->pipeline->v_sinkbin;
	} else {
		wfd_sink_debug("video sinkbin is not created, nothing to destroy");
		return MM_ERROR_NONE;
	}


	parent = gst_element_get_parent(v_sinkbin[WFD_SINK_V_S_BIN].gst);
	if (!parent) {
		wfd_sink_debug("video sinkbin has no parent.. need to relase by itself");

		if (GST_STATE(v_sinkbin[WFD_SINK_V_S_BIN].gst) >= GST_STATE_READY) {
			wfd_sink_debug("try to change state of video sinkbin to NULL");
			ret = gst_element_set_state(v_sinkbin[WFD_SINK_V_S_BIN].gst, GST_STATE_NULL);
			if (ret != GST_STATE_CHANGE_SUCCESS) {
				wfd_sink_error("failed to change state of video sinkbin to NULL");
				return MM_ERROR_WFD_INTERNAL;
			}
		}
		/* release element which are not added to bin */
		for (i = 1; i < WFD_SINK_V_S_NUM; i++) {	/* NOTE : skip bin */
			if (v_sinkbin[i].gst) {
				parent = gst_element_get_parent(v_sinkbin[i].gst);
				if (!parent) {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(v_sinkbin[i].gst)),
					               ((GObject *) v_sinkbin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(v_sinkbin[i].gst));
					v_sinkbin[i].gst = NULL;
				} else {
					wfd_sink_debug("unref %s(current ref %d)",
					               GST_STR_NULL(GST_ELEMENT_NAME(v_sinkbin[i].gst)),
					               ((GObject *) v_sinkbin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(parent));
				}
			}
		}
		/* release video sinkbin with it's childs */
		if (v_sinkbin[WFD_SINK_V_S_BIN].gst) {
			gst_object_unref(GST_OBJECT(v_sinkbin[WFD_SINK_V_S_BIN].gst));
		}
	} else {
		wfd_sink_debug("video sinkbin has parent(%s), unref it ",
		               GST_STR_NULL(GST_OBJECT_NAME(GST_OBJECT(parent))));

		gst_object_unref(GST_OBJECT(parent));
	}

	MMWFDSINK_FREEIF(wfd_sink->pipeline->v_sinkbin);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_create_video_sinkbin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *first_element = NULL;
	MMWFDSinkGstElement *v_sinkbin = NULL;
	GList *element_bucket = NULL;
	GstPad *pad = NULL;
	GstPad *ghostpad = NULL;
	gint i = 0;
	gint surface_type = MM_DISPLAY_SURFACE_X;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink &&
	                            wfd_sink->pipeline,
	                            MM_ERROR_WFD_NOT_INITIALIZED);

	/* alloc handles */
	v_sinkbin = (MMWFDSinkGstElement *)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_V_S_NUM);
	if (!v_sinkbin) {
		wfd_sink_error("failed to allocate memory for video sinkbin");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	memset(v_sinkbin, 0, sizeof(MMWFDSinkGstElement) * WFD_SINK_V_S_NUM);

	/* create video sinkbin */
	v_sinkbin[WFD_SINK_V_S_BIN].id = WFD_SINK_V_S_BIN;
	v_sinkbin[WFD_SINK_V_S_BIN].gst = gst_bin_new("video_sinkbin");
	if (!v_sinkbin[WFD_SINK_V_S_BIN].gst) {
		wfd_sink_error("failed to create video sinkbin");
		goto CREATE_ERROR;
	}

	/* create convert */
	MMWFDSINK_CREATE_ELEMENT(v_sinkbin, WFD_SINK_V_S_CONVERT, wfd_sink->ini.name_of_video_converter, "video_convert", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_sinkbin[WFD_SINK_V_S_CONVERT].gst,  "sink");
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_sinkbin[WFD_SINK_V_S_CONVERT].gst,  "src");

	/* create filter */
	MMWFDSINK_CREATE_ELEMENT(v_sinkbin, WFD_SINK_V_S_FILTER, wfd_sink->ini.name_of_video_filter, "video_filter", TRUE);
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_sinkbin[WFD_SINK_V_S_FILTER].gst,  "sink");
	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_sinkbin[WFD_SINK_V_S_FILTER].gst,  "src");
	if (v_sinkbin[WFD_SINK_V_S_FILTER].gst) {
		GstCaps *caps = NULL;
		caps = gst_caps_new_simple("video/x-raw",
		                           "format", G_TYPE_STRING, "SN12", NULL);
		g_object_set(G_OBJECT(v_sinkbin[WFD_SINK_V_S_FILTER].gst), "caps", caps, NULL);
		gst_object_unref(GST_OBJECT(caps));
	}
	
	/* create sink */
	mm_attrs_get_int_by_name(wfd_sink->attrs, "display_surface_type", &surface_type);

	if(surface_type == MM_DISPLAY_SURFACE_X) {
		MMWFDSINK_CREATE_ELEMENT(v_sinkbin, WFD_SINK_V_S_SINK, wfd_sink->ini.name_of_video_xv_sink, "video_sink", TRUE);
	} else if(surface_type == MM_DISPLAY_SURFACE_EVAS) {
		MMWFDSINK_CREATE_ELEMENT(v_sinkbin, WFD_SINK_V_S_SINK, wfd_sink->ini.name_of_video_evas_sink, "video_sink", TRUE);
	} else {
		wfd_sink_error("failed to set video sink....");
		goto CREATE_ERROR;
	}

	MMWFDSINK_PAD_PROBE(wfd_sink, NULL, v_sinkbin[WFD_SINK_V_S_SINK].gst,  "sink");
	if (v_sinkbin[WFD_SINK_V_S_SINK].gst) {
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_videosink(wfd_sink, v_sinkbin[WFD_SINK_V_S_SINK].gst)) {
			wfd_sink_error("failed to set video sink property....");
			goto CREATE_ERROR;
		}
	}

	/* adding created elements to video sinkbin */
	if (!__mm_wfd_sink_gst_element_add_bucket_to_bin(GST_BIN_CAST(v_sinkbin[WFD_SINK_V_S_BIN].gst), element_bucket, FALSE)) {
		wfd_sink_error("failed to add elements to video sinkbin");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	if (__mm_wfd_sink_gst_element_link_bucket(element_bucket) == -1) {
		wfd_sink_error("failed to link elements of the video sinkbin");
		goto CREATE_ERROR;
	}

	/* get first element's sinkpad for creating ghostpad */
	first_element = (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, 0);
	if (!first_element) {
		wfd_sink_error("failed to get first element of the video sinkbin");
		goto CREATE_ERROR;
	}

	pad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!pad) {
		wfd_sink_error("failed to get pad from first element(%s) of the video sinkbin",
			GST_STR_NULL(GST_ELEMENT_NAME(first_element->gst)));
		goto CREATE_ERROR;
	}

	ghostpad = gst_ghost_pad_new("sink", pad);
	if (!ghostpad) {
		wfd_sink_error("failed to create ghostpad of the video sinkbin");
		goto CREATE_ERROR;
	}

	if (FALSE == gst_element_add_pad(v_sinkbin[WFD_SINK_V_S_BIN].gst, ghostpad)) {
		wfd_sink_error("failed to add ghostpad to video sinkbin");
		goto CREATE_ERROR;
	}

	gst_object_unref(GST_OBJECT(pad));

	g_list_free(element_bucket);


	/* take it */
	wfd_sink->pipeline->v_sinkbin = v_sinkbin;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

	/* ERRORS */
CREATE_ERROR:
	wfd_sink_error("failed to create video sinkbin, releasing all");

	if (pad)
		gst_object_unref(GST_OBJECT(pad));
	pad = NULL;

	if (ghostpad)
		gst_object_unref(GST_OBJECT(ghostpad));
	ghostpad = NULL;

	g_list_free(element_bucket);

	/* release element which are not added to bin */
	for (i = 1; i < WFD_SINK_V_S_NUM; i++) {	/* NOTE : skip bin */
		if (v_sinkbin != NULL && v_sinkbin[i].gst) {
			GstObject *parent = NULL;
			parent = gst_element_get_parent(v_sinkbin[i].gst);

			if (!parent) {
				gst_object_unref(GST_OBJECT(v_sinkbin[i].gst));
				v_sinkbin[i].gst = NULL;
			} else {
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release video sinkbin with it's childs */
	if (v_sinkbin != NULL && v_sinkbin[WFD_SINK_V_S_BIN].gst)
		gst_object_unref(GST_OBJECT(v_sinkbin[WFD_SINK_V_S_BIN].gst));

	MMWFDSINK_FREEIF(v_sinkbin);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_destroy_pipeline(mm_wfd_sink_t *wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* cleanup gst stuffs */
	if (wfd_sink->pipeline) {
		MMWFDSinkGstElement *mainbin = wfd_sink->pipeline->mainbin;

		if (mainbin) {
			ret = gst_element_set_state(mainbin[WFD_SINK_M_PIPE].gst, GST_STATE_NULL);
			if (ret != GST_STATE_CHANGE_SUCCESS) {
				wfd_sink_error("failed to change state of mainbin to NULL");
				return MM_ERROR_WFD_INTERNAL;
			}

			if (MM_ERROR_NONE != __mm_wfd_sink_destroy_video_decodebin(wfd_sink)) {
				wfd_sink_error("failed to destroy video decodebin");
				return MM_ERROR_WFD_INTERNAL;
			}

			if (MM_ERROR_NONE != __mm_wfd_sink_destroy_audio_decodebin(wfd_sink)) {
				wfd_sink_error("failed to destroy audio decodebin");
				return MM_ERROR_WFD_INTERNAL;
			}

			if (MM_ERROR_NONE != __mm_wfd_sink_destroy_video_sinkbin(wfd_sink)) {
				wfd_sink_error("failed to destroy video sinkbin");
				return MM_ERROR_WFD_INTERNAL;
			}

			if (MM_ERROR_NONE != __mm_wfd_sink_destroy_audio_sinkbin(wfd_sink)) {
				wfd_sink_error("failed to destroy audio sinkbin");
				return MM_ERROR_WFD_INTERNAL;
			}

			gst_object_unref(GST_OBJECT(mainbin[WFD_SINK_M_PIPE].gst));

			MMWFDSINK_FREEIF(mainbin);
		}

		MMWFDSINK_FREEIF(wfd_sink->pipeline);
	}

	wfd_sink->audio_decodebin_is_linked = FALSE;
	wfd_sink->video_decodebin_is_linked = FALSE;
	wfd_sink->need_to_reset_basetime = FALSE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static void
__mm_wfd_sink_dump_pipeline_state(mm_wfd_sink_t *wfd_sink)
{
	GstIterator *iter = NULL;
	gboolean done = FALSE;

	GstElement *item = NULL;
	GstElementFactory *factory = NULL;

	GstState state = GST_STATE_VOID_PENDING;
	GstState pending = GST_STATE_VOID_PENDING;
	GstClockTime time = 200 * GST_MSECOND;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail(wfd_sink &&
	                        wfd_sink->pipeline &&
	                        wfd_sink->pipeline->mainbin &&
	                        wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);

	iter = gst_bin_iterate_recurse(GST_BIN(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst));

	if (iter != NULL) {
		while (!done) {
			switch (gst_iterator_next(iter, (gpointer)&item)) {
				case GST_ITERATOR_OK:
					gst_element_get_state(GST_ELEMENT(item), &state, &pending, time);

					factory = gst_element_get_factory(item) ;
					if (factory) {
						wfd_sink_error("%s:%s : From:%s To:%s refcount : %d",
						               GST_STR_NULL(GST_OBJECT_NAME(factory)),
						               GST_STR_NULL(GST_ELEMENT_NAME(item)),
						               gst_element_state_get_name(state),
						               gst_element_state_get_name(pending),
						               GST_OBJECT_REFCOUNT_VALUE(item));
					}
					gst_object_unref(item);
					break;
				case GST_ITERATOR_RESYNC:
					gst_iterator_resync(iter);
					break;
				case GST_ITERATOR_ERROR:
					done = TRUE;
					break;
				case GST_ITERATOR_DONE:
					done = TRUE;
					break;
				default:
					done = TRUE;
					break;
			}
		}
	}

	item = GST_ELEMENT_CAST(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);

	gst_element_get_state(GST_ELEMENT(item), &state, &pending, time);

	factory = gst_element_get_factory(item) ;
	if (factory) {
		wfd_sink_error("%s:%s : From:%s To:%s refcount : %d",
		               GST_OBJECT_NAME(factory),
		               GST_ELEMENT_NAME(item),
		               gst_element_state_get_name(state),
		               gst_element_state_get_name(pending),
		               GST_OBJECT_REFCOUNT_VALUE(item));
	}

	if (iter)
		gst_iterator_free(iter);

	wfd_sink_debug_fleave();

	return;
}

const gchar *
_mm_wfds_sink_get_state_name(MMWFDSinkStateType state)
{
	switch (state) {
		case MM_WFD_SINK_STATE_NONE:
			return "NONE";
		case MM_WFD_SINK_STATE_NULL:
			return "NULL";
		case MM_WFD_SINK_STATE_PREPARED:
			return "PREPARED";
		case MM_WFD_SINK_STATE_CONNECTED:
			return "CONNECTED";
		case MM_WFD_SINK_STATE_PLAYING:
			return "PLAYING";
		case MM_WFD_SINK_STATE_PAUSED:
			return "PAUSED";
		case MM_WFD_SINK_STATE_DISCONNECTED:
			return "DISCONNECTED";
		default:
			return "INVAID";
	}
}

static void __mm_wfd_sink_prepare_video_resolution(gint resolution, guint *CEA_resolution,
                                                   guint *VESA_resolution, guint *HH_resolution)
{
	if (resolution == MM_WFD_SINK_RESOLUTION_UNKNOWN) return;

	*CEA_resolution = 0;
	*VESA_resolution = 0;
	*HH_resolution = 0;

	if (resolution & MM_WFD_SINK_RESOLUTION_1920x1080_P30)
		*CEA_resolution |= WFD_CEA_1920x1080P30;

	if (resolution & MM_WFD_SINK_RESOLUTION_1280x720_P30)
		*CEA_resolution |= WFD_CEA_1280x720P30;

	if (resolution & MM_WFD_SINK_RESOLUTION_960x540_P30)
		*HH_resolution |= WFD_HH_960x540P30;

	if (resolution & MM_WFD_SINK_RESOLUTION_864x480_P30)
		*HH_resolution |= WFD_HH_864x480P30;

	if (resolution & MM_WFD_SINK_RESOLUTION_720x480_P60)
		*CEA_resolution |= WFD_CEA_720x480P60;

	if (resolution & MM_WFD_SINK_RESOLUTION_640x480_P60)
		*CEA_resolution |= WFD_CEA_640x480P60;

	if (resolution & MM_WFD_SINK_RESOLUTION_640x360_P30)
		*HH_resolution |= WFD_HH_640x360P30;
}

int _mm_wfd_sink_set_resolution(mm_wfd_sink_t *wfd_sink, MMWFDSinkResolution resolution)
{
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_PRINT_STATE(wfd_sink);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd_sink);
	if (cur_state != MM_WFD_SINK_STATE_NULL) {
		wfd_sink_error("This function must be called when MM_WFD_SINK_STATE_NULL");
		return MM_ERROR_WFD_INVALID_STATE;
	}

	wfd_sink->supportive_resolution = resolution;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}
