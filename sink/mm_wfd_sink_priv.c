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

#include "mm_wfd_sink_util.h"
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_manager.h"
#include "mm_wfd_sink_dlog.h"


/* gstreamer */
static int __mm_wfd_sink_init_gstreamer(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_pipeline(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_videobin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_audiobin(mm_wfd_sink_t *wfd_sink);
 static int __mm_wfd_sink_destroy_pipeline(mm_wfd_sink_t* wfd_sink);
static int __mm_wfd_sink_set_pipeline_state(mm_wfd_sink_t* wfd_sink, GstState state, gboolean async);

/* state */
static int __mm_wfd_sink_check_state(mm_wfd_sink_t* wfd_sink, MMWFDSinkCommandType cmd);
static int __mm_wfd_sink_set_state(mm_wfd_sink_t* wfd_sink, MMWFDSinkStateType state);

/* util */
static void __mm_wfd_sink_dump_pipeline_state(mm_wfd_sink_t *wfd_sink);
const gchar * __mm_wfds_sink_get_state_name ( MMWFDSinkStateType state );

int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	mm_wfd_sink_t *new_wfd_sink = NULL;

	/* create handle */
	new_wfd_sink = g_malloc0(sizeof(mm_wfd_sink_t));
	if (!new_wfd_sink)
	{
		wfd_sink_error("failed to allocate memory for wi-fi display sink\n");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* Initialize gstreamer related */
	new_wfd_sink->attrs = 0;

	new_wfd_sink->pipeline = NULL;
	new_wfd_sink->added_av_pad_num = 0;
	new_wfd_sink->audio_bin_is_linked = FALSE;
	new_wfd_sink->video_bin_is_linked = FALSE;
	new_wfd_sink->prev_audio_dec_src_pad = NULL;
	new_wfd_sink->next_audio_dec_sink_pad = NULL;

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
	MMWFDSINK_CURRENT_STATE (new_wfd_sink) = MM_WFD_SINK_STATE_NONE;
	MMWFDSINK_PREVIOUS_STATE (new_wfd_sink) =  MM_WFD_SINK_STATE_NONE;
	MMWFDSINK_PENDING_STATE (new_wfd_sink) =  MM_WFD_SINK_STATE_NONE;

	/* initialize audio/video information */
	new_wfd_sink->stream_info.audio_stream_info.codec = WFD_SINK_AUDIO_CODEC_NONE;
	new_wfd_sink->stream_info.audio_stream_info.channels = 0;
	new_wfd_sink->stream_info.audio_stream_info.sample_rate = 0;
	new_wfd_sink->stream_info.audio_stream_info.bitwidth = 0;
	new_wfd_sink->stream_info.video_stream_info.codec = WFD_SINK_VIDEO_CODEC_NONE;
	new_wfd_sink->stream_info.video_stream_info.width = 0;
	new_wfd_sink->stream_info.video_stream_info.height = 0;
	new_wfd_sink->stream_info.video_stream_info.frame_rate = 0;

	/* Initialize command */
	new_wfd_sink->cmd = MM_WFD_SINK_COMMAND_CREATE;
	new_wfd_sink->waiting_cmd = FALSE;

	/* Initialize resource related */
	new_wfd_sink->resource_list = NULL;

	/* Initialize manager related */
	new_wfd_sink->manager_thread = NULL;
	new_wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_NONE;

	/* construct attributes */
	new_wfd_sink->attrs = _mmwfd_construct_attribute((MMHandleType)new_wfd_sink);
	if (!new_wfd_sink->attrs)
	{
		MMWFDSINK_FREEIF (new_wfd_sink);
		wfd_sink_error("failed to set attribute\n");
		return MM_ERROR_WFD_INTERNAL;
	}

	/* load ini for initialize */
	result = mm_wfd_sink_ini_load(&new_wfd_sink->ini);
	if (result != MM_ERROR_NONE)
	{
		wfd_sink_error("failed to load ini file\n");
		goto fail_to_load_ini;
	}
	new_wfd_sink->need_to_reset_basetime = new_wfd_sink->ini.enable_reset_basetime;

	/* initialize manager */
	result = _mm_wfd_sink_init_manager(new_wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("failed to init manager : %d\n", result);
		goto fail_to_init;
	}

	/* initialize gstreamer */
	result = __mm_wfd_sink_init_gstreamer(new_wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("failed to init gstreamer : %d\n", result);
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
	MMWFDSINK_FREEIF (new_wfd_sink);

	*wfd_sink = NULL;

	return result;
}

int _mm_wfd_sink_prepare (mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_PREPARE);

	/* construct pipeline */
	/* create main pipeline */
	result = __mm_wfd_sink_create_pipeline(wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("failed to create pipeline : %d\n", result);
		goto fail_to_create;
	}

	/* create videobin */
	result = __mm_wfd_sink_create_videobin(wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("failed to create videobin : %d\n", result);
		goto fail_to_create;
	}

	/* create audiobin */
	result = __mm_wfd_sink_create_audiobin(wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("fail to create audiobin : %d\n", result);
		goto fail_to_create;
	}

	/* set pipeline READY state */
	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_READY, TRUE);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("failed to set state : %d\n", result);
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

int _mm_wfd_sink_connect (mm_wfd_sink_t *wfd_sink, const char *uri)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(uri && strlen(uri) > strlen("rtsp://"),
		MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_DEPAY].gst &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_DEMUX].gst,
		MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_CONNECT);

	wfd_sink_debug ("try to connect to %s.....\n", GST_STR_NULL(uri));

	/* set uri to wfdrtspsrc */
	g_object_set (G_OBJECT(wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst), "location", uri, NULL);

	/* set pipeline PAUSED state */
	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_PAUSED, TRUE);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("failed to set state : %d\n", result);
		return result;
	}

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_START);

	WFD_SINK_MANAGER_LOCK(wfd_sink) ;
	wfd_sink_debug("check pipeline is ready to start");
	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_PLAYING, TRUE);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("failed to set state : %d\n", result);
		return result;
	}

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_disconnect(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_DISCONNECT);

	WFD_SINK_MANAGER_LOCK(wfd_sink) ;
	WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_EXIT);
	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	result = __mm_wfd_sink_set_pipeline_state (wfd_sink, GST_STATE_READY, FALSE);
	if (result < MM_ERROR_NONE)
	{
		wfd_sink_error("fail to set state : %d\n", result);
		return result;
	}

	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_DISCONNECTED);

	wfd_sink_debug_fleave();

	return result;
}

int _mm_wfd_sink_unprepare(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_UNPREPARE);

	WFD_SINK_MANAGER_LOCK(wfd_sink) ;
	WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink, WFD_SINK_MANAGER_CMD_EXIT);
	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	/* release pipeline */
	result =  __mm_wfd_sink_destroy_pipeline (wfd_sink);
	if ( result != MM_ERROR_NONE)
	{
		wfd_sink_error("failed to destory pipeline\n");
		return MM_ERROR_WFD_INTERNAL;
	}
	else
	{
		wfd_sink_debug("success to destory pipeline\n");
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
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_DESTROY);

	/* unload ini */
	mm_wfd_sink_ini_unload(&wfd_sink->ini);

	/* release attributes */
	_mmwfd_deconstruct_attribute(wfd_sink->attrs);

	if (MM_ERROR_NONE != _mm_wfd_sink_release_manager(wfd_sink))
	{
		wfd_sink_error("failed to release manager\n");
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

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	wfd_sink->msg_cb = callback;
	wfd_sink->msg_user_data = user_data;

	wfd_sink_debug_fleave();

	return result;
}

static int __mm_wfd_sink_init_gstreamer(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	gint* argc = NULL;
	gchar** argv = NULL;
	static const int max_argc = 50;
	GError *err = NULL;
	gint i = 0;

	wfd_sink_debug_fenter();

	/* alloc */
	argc = calloc(1, sizeof(gint) );
	argv = calloc(max_argc, sizeof(gchar*));
	if (!argc || !argv)
	{
		wfd_sink_error("failed to allocate memory for wfdsink\n");

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
	for (i=0; i<5; i++)
	{
		if (strlen( wfd_sink->ini.gst_param[i] ) > 2)
		{
			wfd_sink_debug("set %s\n", wfd_sink->ini.gst_param[i]);
			argv[*argc] = g_strdup(wfd_sink->ini.gst_param[i]);
			(*argc)++;
		}
	}

	wfd_sink_debug("initializing gstreamer with following parameter\n");
	wfd_sink_debug("argc : %d\n", *argc);

	for ( i = 0; i < *argc; i++ )
	{
		wfd_sink_debug("argv[%d] : %s\n", i, argv[i]);
	}

	/* initializing gstreamer */
	if ( ! gst_init_check (argc, &argv, &err))
	{
		wfd_sink_error("failed to initialize gstreamer: %s\n",
			err ? err->message : "unknown error occurred");
		if (err)
			g_error_free (err);

		result = MM_ERROR_WFD_INTERNAL;
	}

	/* release */
	for ( i = 0; i < *argc; i++ )
	{
		MMWFDSINK_FREEIF( argv[i] );
	}
	MMWFDSINK_FREEIF(argv);
	MMWFDSINK_FREEIF(argc);

	wfd_sink_debug_fleave();

	return result;
}

static void
_mm_wfd_sink_correct_pipeline_latency (mm_wfd_sink_t *wfd_sink)
{
	GstQuery *qlatency;
	GstClockTime min_latency;

	qlatency = gst_query_new_latency();
	gst_element_query (wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst, qlatency);
	gst_query_parse_latency (qlatency, NULL, &min_latency, NULL);

	debug_msg ("Correct manually pipeline latency: current=%"GST_TIME_FORMAT, GST_TIME_ARGS (min_latency));
	g_object_set (wfd_sink->pipeline->videobin[WFD_SINK_V_SINK].gst, "ts-offset", -(gint64)(min_latency*9/10), NULL);
	g_object_set (wfd_sink->pipeline->audiobin[WFD_SINK_A_SINK].gst, "ts-offset", -(gint64)(min_latency*9/10), NULL);
	gst_query_unref(qlatency);
}

static GstBusSyncReply
_mm_wfd_bus_sync_callback (GstBus * bus, GstMessage * message, gpointer data)
{
	GstBusSyncReply ret = GST_BUS_PASS;

	wfd_sink_return_val_if_fail (message &&
		GST_IS_MESSAGE(message) &&
		GST_MESSAGE_SRC(message),
		GST_BUS_DROP);

	switch (GST_MESSAGE_TYPE (message))
	{
		case GST_MESSAGE_TAG:
		break;
		case GST_MESSAGE_DURATION:
		break;
		case GST_MESSAGE_STATE_CHANGED:
		{
			/* we only handle state change messages from pipeline */
			if (!GST_IS_PIPELINE(GST_MESSAGE_SRC(message)))
				ret = GST_BUS_DROP;
		}
		break;
		case GST_MESSAGE_ASYNC_DONE:
		{
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
_mm_wfd_sink_msg_callback (GstBus *bus, GstMessage *msg, gpointer data)
{
	mm_wfd_sink_t* wfd_sink = (mm_wfd_sink_t*) data;
	const GstStructure* message_structure = gst_message_get_structure(msg);
	gboolean ret = TRUE;

	wfd_sink_return_val_if_fail (wfd_sink, FALSE);
	wfd_sink_return_val_if_fail (msg && GST_IS_MESSAGE(msg), FALSE);

	wfd_sink_debug("got %s from %s \n",
		GST_STR_NULL(GST_MESSAGE_TYPE_NAME(msg)),
		GST_STR_NULL(GST_OBJECT_NAME(GST_MESSAGE_SRC(msg))));

	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_ERROR:
		{
			GError *error = NULL;
			gchar* debug = NULL;

			/* get error code */
			gst_message_parse_error( msg, &error, &debug );

			wfd_sink_error("error : %s\n", error->message);
			wfd_sink_error("debug : %s\n", debug);

			MMWFDSINK_FREEIF( debug );
			g_error_free( error );
		}
		break;

		case GST_MESSAGE_WARNING:
		{
			char* debug = NULL;
			GError* error = NULL;

			gst_message_parse_warning(msg, &error, &debug);

			wfd_sink_error("warning : %s\n", error->message);
			wfd_sink_error("debug : %s\n", debug);

			MMWFDSINK_FREEIF (debug);
			g_error_free (error);
		}
		break;

		case GST_MESSAGE_STATE_CHANGED:
		{
			const GValue *voldstate, *vnewstate, *vpending;
			GstState oldstate, newstate, pending;
			const GstStructure *structure;

			/* we only handle messages from pipeline */
			if (msg->src != (GstObject *)wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst )
				break;

			/* get state info from msg */
			structure = gst_message_get_structure(msg);
			voldstate = gst_structure_get_value (structure, "old-state");
			vnewstate = gst_structure_get_value (structure, "new-state");
			vpending = gst_structure_get_value (structure, "pending-state");

			oldstate = (GstState)voldstate->data[0].v_int;
			newstate = (GstState)vnewstate->data[0].v_int;
			pending = (GstState)vpending->data[0].v_int;

			wfd_sink_debug("state changed [%s] : %s ---> %s final : %s\n",
				GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)),
				gst_element_state_get_name( (GstState)oldstate ),
				gst_element_state_get_name( (GstState)newstate ),
				gst_element_state_get_name( (GstState)pending ) );

			if (oldstate == newstate)
			{
				wfd_sink_error("pipeline reports state transition to old state\n");
				break;
			}

			switch(newstate)
			{
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

		case GST_MESSAGE_CLOCK_LOST:
		{
			GstClock *clock = NULL;
			gst_message_parse_clock_lost (msg, &clock);
			wfd_sink_debug("The current clock[%s] as selected by the pipeline became unusable.",
				(clock ? GST_OBJECT_NAME (clock) : "NULL"));
		}
		break;

		case GST_MESSAGE_NEW_CLOCK:
		{
			GstClock *clock = NULL;
			gst_message_parse_new_clock (msg, &clock);
			if (!clock)
				break;

			if (wfd_sink->clock)
			{
				if (wfd_sink->clock != clock)
					wfd_sink_debug("clock is changed! [%s] --> [%s]\n",
						GST_STR_NULL(GST_OBJECT_NAME (wfd_sink->clock)),
						GST_STR_NULL(GST_OBJECT_NAME (clock)));
				else
					wfd_sink_debug("same clock is selected again! [%s] \n",
						GST_STR_NULL(GST_OBJECT_NAME (clock)));
			}
			else
			{
				wfd_sink_debug("new clock [%s] was selected in the pipeline\n",
					(GST_STR_NULL(GST_OBJECT_NAME (clock))));
			}

			wfd_sink->clock = clock;
		}
		break;

		case GST_MESSAGE_APPLICATION:
		{
			const gchar* message_structure_name;

			message_structure_name = gst_structure_get_name(message_structure);
			if (!message_structure_name)
				break;

			wfd_sink_debug ("message name : %s", GST_STR_NULL(message_structure_name));
		}
		break;

		case GST_MESSAGE_ELEMENT:
		{
			const gchar *structure_name = NULL;
			const GstStructure* message_structure = NULL;

			message_structure = gst_message_get_structure(msg);
			structure_name = gst_structure_get_name(message_structure);
			if (structure_name)
			{
				wfd_sink_debug("got element specific message[%s]\n", GST_STR_NULL(structure_name));
				if(g_strrstr(structure_name, "GstUDPSrcTimeout"))
				{
					wfd_sink_error("Got %s, post error message\n", GST_STR_NULL(structure_name));
					MMWFDSINK_POST_MESSAGE(wfd_sink,
						MM_ERROR_WFD_INTERNAL,
						MMWFDSINK_CURRENT_STATE(wfd_sink));
				}
			}
		}
		break;

		case GST_MESSAGE_PROGRESS:
		{
			GstProgressType type = GST_PROGRESS_TYPE_ERROR;
			gchar *category=NULL, *text=NULL;

			gst_message_parse_progress (msg, &type, &category, &text);
			wfd_sink_debug("%s : %s \n", GST_STR_NULL(category), GST_STR_NULL(text));

			switch (type)
			{
				case GST_PROGRESS_TYPE_START:
					break;
				case GST_PROGRESS_TYPE_COMPLETE:
					if (category && !strcmp (category, "open"))
						__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_CONNECTED);
					else if (category && !strcmp (category, "play"))
					{
						__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_PLAYING);
						//_mm_wfd_sink_correct_pipeline_latency (wfd_sink);
					}
					else if (category && !strcmp (category, "pause"))
						__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_PAUSED);
					else if (category && !strcmp (category, "close"))
						__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_DISCONNECTED);


					break;
				case GST_PROGRESS_TYPE_CANCELED:
					break;
				case GST_PROGRESS_TYPE_ERROR:
					if (category && !strcmp (category, "open"))
					{
						wfd_sink_error("got error : %s\n", GST_STR_NULL(text));
						//_mm_wfd_sink_disconnect (wfd_sink);
						MMWFDSINK_POST_MESSAGE(wfd_sink,
							MM_ERROR_WFD_INTERNAL,
							MMWFDSINK_CURRENT_STATE(wfd_sink));
					}
					else if (category && !strcmp (category, "play"))
					{
						wfd_sink_error("got error : %s\n", GST_STR_NULL(text));
						//_mm_wfd_sink_disconnect (wfd_sink);
						MMWFDSINK_POST_MESSAGE(wfd_sink,
							MM_ERROR_WFD_INTERNAL,
							MMWFDSINK_CURRENT_STATE(wfd_sink));
					}
					else if (category && !strcmp (category, "pause"))
					{
						wfd_sink_error("got error : %s\n", GST_STR_NULL(text));
						//_mm_wfd_sink_disconnect (wfd_sink);
						MMWFDSINK_POST_MESSAGE(wfd_sink,
							MM_ERROR_WFD_INTERNAL,
							MMWFDSINK_CURRENT_STATE(wfd_sink));
					}
					else if (category && !strcmp (category, "close"))
					{
						wfd_sink_error("got error : %s\n", GST_STR_NULL(text));
						//_mm_wfd_sink_disconnect (wfd_sink);
						MMWFDSINK_POST_MESSAGE(wfd_sink,
							MM_ERROR_WFD_INTERNAL,
							MMWFDSINK_CURRENT_STATE(wfd_sink));
					}
					else
					{
						wfd_sink_error("got error : %s\n", GST_STR_NULL(text));
					}
					break;
				default :
					wfd_sink_error("progress message has no type\n");
					return ret;
			}

			MMWFDSINK_FREEIF (category);
			MMWFDSINK_FREEIF (text);
		}
		break;
		case GST_MESSAGE_ASYNC_START:
			wfd_sink_debug("GST_MESSAGE_ASYNC_START : %s\n", gst_element_get_name(GST_MESSAGE_SRC(msg)));
			break;
		case GST_MESSAGE_ASYNC_DONE:
			wfd_sink_debug("GST_MESSAGE_ASYNC_DONE : %s\n", gst_element_get_name(GST_MESSAGE_SRC(msg)));
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
			wfd_sink_debug("unhandled message\n");
		break;
	}

	return ret;
}

static int
__mm_wfd_sink_gst_element_add_bucket_to_bin (GstBin* bin, GList* element_bucket, gboolean need_prepare)
{
	GList* bucket = element_bucket;
	MMWFDSinkGstElement* element = NULL;
	int successful_add_count = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (element_bucket, 0);
	wfd_sink_return_val_if_fail (bin, 0);

	for (; bucket; bucket = bucket->next)
	{
		element = (MMWFDSinkGstElement*)bucket->data;

		if (element && element->gst)
		{
			if (need_prepare)
				gst_element_set_state (GST_ELEMENT(element->gst), GST_STATE_READY);

			if (!gst_bin_add (bin, GST_ELEMENT(element->gst)))
			{
				wfd_sink_error("failed to add element [%s] to bin [%s]\n",
					GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(element->gst))),
					GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT_CAST(bin) )));
				return 0;
			}

			wfd_sink_debug("add element [%s] to bin [%s]\n",
				GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(element->gst))),
				GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT_CAST(bin) )));

			successful_add_count ++;
		}
	}

	wfd_sink_debug_fleave();

	return successful_add_count;
}

static int
__mm_wfd_sink_gst_element_link_bucket (GList* element_bucket)
{
	GList* bucket = element_bucket;
	MMWFDSinkGstElement* element = NULL;
	MMWFDSinkGstElement* prv_element = NULL;
	gint successful_link_count = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (element_bucket, -1);

	prv_element = (MMWFDSinkGstElement*)bucket->data;
	bucket = bucket->next;

	for ( ; bucket; bucket = bucket->next )
	{
		element = (MMWFDSinkGstElement*)bucket->data;

		if (element && element->gst)
		{
			if ( gst_element_link (GST_ELEMENT(prv_element->gst), GST_ELEMENT(element->gst)) )
			{
				wfd_sink_debug("linking [%s] to [%s] success\n",
					GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(prv_element->gst))),
					GST_STR_NULL(GST_ELEMENT_NAME(GST_ELEMENT(element->gst))));
				successful_link_count ++;
			}
			else
			{
				wfd_sink_error("linking [%s] to [%s] failed\n",
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
__mm_wfd_sink_check_state(mm_wfd_sink_t* wfd_sink, MMWFDSinkCommandType cmd)
{
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_PRINT_STATE(wfd_sink);

	cur_state = MMWFDSINK_CURRENT_STATE(wfd_sink);

	switch (cmd)
	{
		case MM_WFD_SINK_COMMAND_CREATE:
		{
			if (cur_state != MM_WFD_SINK_STATE_NONE)
				goto invalid_state;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NULL;
		}
		break;

		case MM_WFD_SINK_COMMAND_PREPARE:
		{
			if (cur_state == MM_WFD_SINK_STATE_PREPARED)
				goto no_operation;
			else if (cur_state != MM_WFD_SINK_STATE_NULL)
				goto invalid_state;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PREPARED;
		}
		break;

		case MM_WFD_SINK_COMMAND_CONNECT:
		{
			if (cur_state == MM_WFD_SINK_STATE_CONNECTED)
				goto no_operation;
			else if (cur_state != MM_WFD_SINK_STATE_PREPARED)
				goto invalid_state;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_CONNECTED;
		}
		break;

		case MM_WFD_SINK_COMMAND_START:
		{
			if (cur_state == MM_WFD_SINK_STATE_PLAYING)
				goto no_operation;
			else if (cur_state != MM_WFD_SINK_STATE_CONNECTED)
				goto invalid_state;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PLAYING;
		}
		break;

		case MM_WFD_SINK_COMMAND_DISCONNECT:
		{
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

		case MM_WFD_SINK_COMMAND_UNPREPARE:
		{
			if (cur_state == MM_WFD_SINK_STATE_NONE ||
				cur_state == MM_WFD_SINK_STATE_NULL)
				goto no_operation;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NULL;
		}
		break;

		case MM_WFD_SINK_COMMAND_DESTROY:
		{
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
	wfd_sink_debug ("already %s state, nothing to do.\n", MMWFDSINK_STATE_GET_NAME(cur_state));
	return MM_ERROR_WFD_NO_OP;

/* ERRORS */
invalid_state:
	wfd_sink_error ("current state is invalid.\n", MMWFDSINK_STATE_GET_NAME(cur_state));
	return MM_ERROR_WFD_INVALID_STATE;
}

static int __mm_wfd_sink_set_state(mm_wfd_sink_t* wfd_sink, MMWFDSinkStateType state)
{
	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	if (MMWFDSINK_CURRENT_STATE(wfd_sink) == state )
	{
		wfd_sink_error("already state(%s)\n", MMWFDSINK_STATE_GET_NAME(state));
		MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NONE;
		return MM_ERROR_NONE;
	}

	/* update wi-fi display state */
	MMWFDSINK_PREVIOUS_STATE(wfd_sink) = MMWFDSINK_CURRENT_STATE(wfd_sink);
	MMWFDSINK_CURRENT_STATE(wfd_sink) = state;

	if ( MMWFDSINK_CURRENT_STATE(wfd_sink) == MMWFDSINK_PENDING_STATE(wfd_sink) )
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
__mm_wfd_sink_set_pipeline_state(mm_wfd_sink_t* wfd_sink, GstState state, gboolean async)
{
	GstStateChangeReturn result = GST_STATE_CHANGE_FAILURE;
	GstState cur_state = GST_STATE_VOID_PENDING;
	GstState pending_state = GST_STATE_VOID_PENDING;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
		MM_ERROR_WFD_NOT_INITIALIZED);

	wfd_sink_return_val_if_fail (state > GST_STATE_VOID_PENDING,
		MM_ERROR_WFD_INVALID_ARGUMENT);

	wfd_sink_debug ("try to set %s state \n", gst_element_state_get_name(state));

	result = gst_element_set_state (wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst, state);
	if (result == GST_STATE_CHANGE_FAILURE)
	{
		wfd_sink_error ("fail to set %s state....\n", gst_element_state_get_name(state));
		return MM_ERROR_WFD_INTERNAL;
	}

	if (!async)
	{
		wfd_sink_debug ("wait for changing state is completed \n");

		result = gst_element_get_state (wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
				&cur_state, &pending_state, wfd_sink->ini.state_change_timeout * GST_SECOND);
		if (result == GST_STATE_CHANGE_FAILURE)
		{
			wfd_sink_error ("fail to get state within %d seconds....\n", wfd_sink->ini.state_change_timeout);

			__mm_wfd_sink_dump_pipeline_state(wfd_sink);

			return MM_ERROR_WFD_INTERNAL;
		}
		else if (result == GST_STATE_CHANGE_NO_PREROLL)
		{
			wfd_sink_debug ("successfully changed state but is not able to provide data yet\n");
		}

		wfd_sink_debug("cur state is %s, pending state is %s\n",
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

	wfd_sink_return_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);
	wfd_sink_return_if_fail (wfd_sink->need_to_reset_basetime);


	if (wfd_sink->clock)
		base_time = gst_clock_get_time (wfd_sink->clock);

	if (GST_CLOCK_TIME_IS_VALID(base_time))
	{

		wfd_sink_debug ("set pipeline base_time as now [%"GST_TIME_FORMAT"]\n",GST_TIME_ARGS(base_time));

		for (i=0; i<WFD_SINK_M_NUM; i++)
		{
			if (wfd_sink->pipeline->mainbin[i].gst)
				gst_element_set_base_time(GST_ELEMENT_CAST (wfd_sink->pipeline->mainbin[i].gst), base_time);
		}

		if (wfd_sink->pipeline->videobin)
		{
			for (i=0; i<WFD_SINK_V_NUM; i++)
			{
				if (wfd_sink->pipeline->videobin[i].gst)
					gst_element_set_base_time(GST_ELEMENT_CAST (wfd_sink->pipeline->videobin[i].gst), base_time);
			}
		}

		if (wfd_sink->pipeline->audiobin)
		{
			for (i=0; i<WFD_SINK_A_NUM; i++)
			{
				if (wfd_sink->pipeline->audiobin[i].gst)
					gst_element_set_base_time(GST_ELEMENT_CAST (wfd_sink->pipeline->audiobin[i].gst), base_time);
			}
		}
		wfd_sink->need_to_reset_basetime = FALSE;
	}

	wfd_sink_debug_fleave();

	return;
}

int
__mm_wfd_sink_prepare_videobin (mm_wfd_sink_t *wfd_sink)
{
	GstElement* videobin = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline,
		MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline->videobin == NULL)
	{
		if (MM_ERROR_NONE !=__mm_wfd_sink_create_videobin (wfd_sink))
		{
			wfd_sink_error ("failed to create videobin....\n");
			goto ERROR;
		}
	}
	else
	{
		wfd_sink_debug ("videobin is already created.\n");
	}

	videobin = wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst;

	if (GST_STATE(videobin) <= GST_STATE_NULL)
	{
		if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (videobin, GST_STATE_READY))
		{
			wfd_sink_error("failed to set state(READY) to %s\n", GST_STR_NULL(GST_ELEMENT_NAME(videobin)));
			goto ERROR;
		}
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
__mm_wfd_sink_prepare_audiobin (mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement* audiobin = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline,
		MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline->audiobin == NULL)
	{
		if (MM_ERROR_NONE !=__mm_wfd_sink_create_audiobin (wfd_sink))
		{
			wfd_sink_error ("failed to create audiobin....\n");
			goto ERROR;
		}
	}

	if (!wfd_sink->audio_bin_is_linked)
	{
		if (MM_ERROR_NONE !=__mm_wfd_sink_link_audiobin(wfd_sink))
		{
			wfd_sink_error ("failed to link audio decoder.....\n");
			goto ERROR;
		}
	}

	audiobin = wfd_sink->pipeline->audiobin;

	if (GST_STATE(audiobin) <= GST_STATE_NULL)
	{
		if (GST_STATE_CHANGE_FAILURE ==gst_element_set_state (audiobin[WFD_SINK_A_BIN].gst, GST_STATE_READY))
		{
			wfd_sink_error("failed to set state(READY) to %s\n",
				GST_STR_NULL(GST_ELEMENT_NAME(audiobin)));
			goto ERROR;
		}
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

#define COMPENSATION_CRETERIA_VALUE 1000000 // 1 msec
#define COMPENSATION_CHECK_PERIOD 30*GST_SECOND  // 30 sec

static GstPadProbeReturn
_mm_wfd_sink_check_running_time(GstPad * pad, GstPadProbeInfo * info, gpointer u_data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)u_data;
	GstClockTime current_time = GST_CLOCK_TIME_NONE;
	GstClockTime start_time = GST_CLOCK_TIME_NONE;
	GstClockTime running_time = GST_CLOCK_TIME_NONE;
	GstClockTime base_time = GST_CLOCK_TIME_NONE;
	GstClockTime render_time = GST_CLOCK_TIME_NONE;
	GstClockTimeDiff diff = GST_CLOCK_TIME_NONE;
	GstBuffer * buffer = NULL;
	gint64 ts_offset = 0LL;
	GstPadProbeReturn ret = GST_PAD_PROBE_OK;

	wfd_sink_return_val_if_fail (info, FALSE);
	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
		GST_PAD_PROBE_DROP);

	if (!wfd_sink->clock)
	{
		wfd_sink_warning ("pipeline did not select clock, yet\n");
		return GST_PAD_PROBE_OK;
	}

	if (wfd_sink->need_to_reset_basetime)
		_mm_wfd_sink_reset_basetime(wfd_sink);

	/* calculate current runninig time */
	current_time = gst_clock_get_time(wfd_sink->clock);
	if (g_strrstr(GST_OBJECT_NAME(pad), "video"))
		base_time = gst_element_get_base_time(wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst);
	else if (g_strrstr(GST_OBJECT_NAME(pad), "audio"))
		base_time = gst_element_get_base_time(wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst);
	start_time = gst_element_get_start_time(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);
	if (GST_CLOCK_TIME_IS_VALID(current_time) &&
		GST_CLOCK_TIME_IS_VALID(start_time) &&
		GST_CLOCK_TIME_IS_VALID(base_time))
	{
		running_time = current_time - (start_time + base_time);
	}
	else
	{
		wfd_sink_debug ("current time %"GST_TIME_FORMAT", start time %"GST_TIME_FORMAT
			"  base time %"GST_TIME_FORMAT"\n", GST_TIME_ARGS(current_time),
			GST_TIME_ARGS(start_time), GST_TIME_ARGS(base_time));
		return GST_PAD_PROBE_OK;
	}

	/* calculate this buffer rendering time */
	buffer = gst_pad_probe_info_get_buffer (info);
	if (!GST_BUFFER_TIMESTAMP_IS_VALID(buffer))
	{
		wfd_sink_error ("buffer timestamp is invalid.\n");
		return GST_PAD_PROBE_OK;
	}

	if (g_strrstr(GST_OBJECT_NAME(pad), "audio"))
	{
		if (wfd_sink->pipeline && wfd_sink->pipeline->audiobin && wfd_sink->pipeline->audiobin[WFD_SINK_A_SINK].gst)
			g_object_get(G_OBJECT(wfd_sink->pipeline->audiobin[WFD_SINK_A_SINK].gst), "ts-offset", &ts_offset, NULL);
	}
	else if (g_strrstr(GST_OBJECT_NAME(pad), "video"))
	{
		if (wfd_sink->pipeline && wfd_sink->pipeline->videobin && wfd_sink->pipeline->videobin[WFD_SINK_V_SINK].gst)
			g_object_get(G_OBJECT(wfd_sink->pipeline->videobin[WFD_SINK_V_SINK].gst), "ts-offset", &ts_offset, NULL);
	}

	render_time = GST_BUFFER_TIMESTAMP(buffer);
	render_time += ts_offset;

	/* chekc this buffer could be rendered or not */
	if (GST_CLOCK_TIME_IS_VALID(running_time) && GST_CLOCK_TIME_IS_VALID(render_time))
	{
		diff=GST_CLOCK_DIFF(running_time, render_time);
		if (diff < 0)
		{
			/* this buffer could be NOT rendered */
			wfd_sink_debug ("%s : diff time : -%" GST_TIME_FORMAT "\n",
				GST_STR_NULL((GST_OBJECT_NAME(pad))),
				GST_TIME_ARGS(GST_CLOCK_DIFF(render_time, running_time)));
		}
		else
		{
			/* this buffer could be rendered */
			//wfd_sink_debug ("%s :diff time : %" GST_TIME_FORMAT "\n",
			//	GST_STR_NULL((GST_OBJECT_NAME(pad))),
			//	GST_TIME_ARGS(diff));
		}
	}

	/* update buffer count and gap */
	if (g_strrstr(GST_OBJECT_NAME(pad), "video"))
	{
		wfd_sink->video_buffer_count ++;
		wfd_sink->video_accumulated_gap += diff;
	}
	else if (g_strrstr(GST_OBJECT_NAME(pad), "audio"))
	{
		wfd_sink->audio_buffer_count ++;
		wfd_sink->audio_accumulated_gap += diff;
	}
	else
	{
		wfd_sink_warning("invalid buffer type.. \n");
		return GST_PAD_PROBE_DROP;
	}

	if (GST_CLOCK_TIME_IS_VALID(wfd_sink->last_buffer_timestamp))
	{
		/* fisrt 60sec, just calculate the gap between source device and sink device */
		if (GST_BUFFER_TIMESTAMP(buffer) < 60*GST_SECOND)
			return GST_PAD_PROBE_OK;

		/* every 10sec, calculate the gap between source device and sink device */
		if (GST_CLOCK_DIFF(wfd_sink->last_buffer_timestamp, GST_BUFFER_TIMESTAMP(buffer))
			> COMPENSATION_CHECK_PERIOD)
		{
			gint64 audio_avgrage_gap = 0LL;
			gint64 video_avgrage_gap = 0LL;
			gint64 audio_avgrage_gap_diff = 0LL;
			gint64 video_avgrage_gap_diff = 0LL;
			gboolean video_minus_compensation = FALSE;
			gboolean audio_minus_compensation = FALSE;
			gint64 avgrage_gap_diff = 0LL;
			gboolean minus_compensation = FALSE;

			/* check video */
			if (wfd_sink->video_buffer_count > 0)
			{
				video_avgrage_gap = wfd_sink->video_accumulated_gap /wfd_sink->video_buffer_count;

				if (wfd_sink->video_average_gap !=0)
				{
					if (video_avgrage_gap > wfd_sink->video_average_gap)
					{
						video_avgrage_gap_diff = video_avgrage_gap - wfd_sink->video_average_gap;
						video_minus_compensation = TRUE;
					}
					else
					{
						video_avgrage_gap_diff = wfd_sink->video_average_gap - video_avgrage_gap;
						video_minus_compensation = FALSE;
					}
				}
				else
				{
					wfd_sink_debug ("first update video average gap (%lld) \n", video_avgrage_gap);
					wfd_sink->video_average_gap = video_avgrage_gap;
				}
			}
			else
			{
				wfd_sink_debug ("there is no video buffer flow during %"GST_TIME_FORMAT
					" ~ %" GST_TIME_FORMAT"\n",
					GST_TIME_ARGS(wfd_sink->last_buffer_timestamp),
					GST_TIME_ARGS( GST_BUFFER_TIMESTAMP(buffer)));
			}

			/* check audio */
			if (wfd_sink->audio_buffer_count > 0)
			{
				audio_avgrage_gap = wfd_sink->audio_accumulated_gap /wfd_sink->audio_buffer_count;

				if (wfd_sink->audio_average_gap !=0)
				{
					if (audio_avgrage_gap > wfd_sink->audio_average_gap)
					{
						audio_avgrage_gap_diff = audio_avgrage_gap - wfd_sink->audio_average_gap;
						audio_minus_compensation = TRUE;
					}
					else
					{
						audio_avgrage_gap_diff = wfd_sink->audio_average_gap - audio_avgrage_gap;
						audio_minus_compensation = FALSE;
					}
				}
				else
				{
					wfd_sink_debug ("first update audio average gap (%lld) \n", audio_avgrage_gap);
					wfd_sink->audio_average_gap = audio_avgrage_gap;
				}
			}
			else
			{
				wfd_sink_debug ("there is no audio buffer flow during %"GST_TIME_FORMAT
					" ~ %" GST_TIME_FORMAT"\n",
					GST_TIME_ARGS(wfd_sink->last_buffer_timestamp),
					GST_TIME_ARGS( GST_BUFFER_TIMESTAMP(buffer)));
			}

			/* selecet average_gap_diff between video and audio */
			/*  which makes no buffer drop in the sink elements */
			if (video_avgrage_gap_diff && audio_avgrage_gap_diff)
			{
				if (!video_minus_compensation && !audio_minus_compensation)
				{
					minus_compensation = FALSE;
					if (video_avgrage_gap_diff > audio_avgrage_gap_diff)
						avgrage_gap_diff = video_avgrage_gap_diff;
					else
						avgrage_gap_diff = audio_avgrage_gap_diff;
				}
				else if (video_minus_compensation && audio_minus_compensation)
				{
					minus_compensation = TRUE;
					if (video_avgrage_gap_diff > audio_avgrage_gap_diff)
						avgrage_gap_diff = audio_avgrage_gap_diff;
					else
						avgrage_gap_diff = video_avgrage_gap_diff;
				}
				else
				{
					minus_compensation = FALSE;
					if (!video_minus_compensation)
						avgrage_gap_diff = video_avgrage_gap_diff;
					else
						avgrage_gap_diff = audio_avgrage_gap_diff;
				}
			}
			else if (video_avgrage_gap_diff)
			{
				minus_compensation = video_minus_compensation;
				avgrage_gap_diff = video_avgrage_gap_diff;
			}
			else if (audio_avgrage_gap_diff)
			{
				minus_compensation = audio_minus_compensation;
				avgrage_gap_diff = audio_avgrage_gap_diff;
			}

			wfd_sink_debug ("average diff gap difference beween audio:%s%lld and video:%s%lld \n",
				audio_minus_compensation ? "-" : "", audio_avgrage_gap_diff,
				video_minus_compensation ? "-" : "", video_avgrage_gap_diff);


			/* if calculated gap diff is larger than 1ms. need to compensate buffer timestamp */
			if (avgrage_gap_diff >= COMPENSATION_CRETERIA_VALUE)
			{
				if (minus_compensation)
					ts_offset -= avgrage_gap_diff;
				else
					ts_offset += avgrage_gap_diff;

				wfd_sink_debug ("do timestamp compensation : %s%lld (ts-offset : %"
					GST_TIME_FORMAT") at (%" GST_TIME_FORMAT")\n",
					minus_compensation? "-" : "", avgrage_gap_diff,
					GST_TIME_ARGS(ts_offset), GST_TIME_ARGS(running_time));

				if (wfd_sink->pipeline && wfd_sink->pipeline->audiobin && wfd_sink->pipeline->audiobin[WFD_SINK_A_SINK].gst)
					g_object_set(G_OBJECT(wfd_sink->pipeline->audiobin[WFD_SINK_A_SINK].gst), "ts-offset", (gint64)ts_offset, NULL);
				if (wfd_sink->pipeline && wfd_sink->pipeline->videobin && wfd_sink->pipeline->videobin[WFD_SINK_V_SINK].gst)
					g_object_set(G_OBJECT(wfd_sink->pipeline->videobin[WFD_SINK_V_SINK].gst), "ts-offset", (gint64)ts_offset, NULL);
			}
			else
			{
				wfd_sink_debug ("don't need to do timestamp compensation : %s%lld (ts-offset : %"GST_TIME_FORMAT ")\n",
					minus_compensation? "-" : "", avgrage_gap_diff, GST_TIME_ARGS(ts_offset));
			}

			/* reset values*/
			wfd_sink->video_buffer_count = 0;
			wfd_sink->video_accumulated_gap = 0LL;
			wfd_sink->audio_buffer_count = 0;
			wfd_sink->audio_accumulated_gap = 0LL;
			wfd_sink->last_buffer_timestamp = GST_BUFFER_TIMESTAMP(buffer);
		}
	}
	else
	{
		wfd_sink_debug ("first update last buffer timestamp :%" GST_TIME_FORMAT" \n",
			 GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buffer)));
		wfd_sink->last_buffer_timestamp = GST_BUFFER_TIMESTAMP(buffer);
	}

	return GST_PAD_PROBE_OK;
}


static void
__mm_wfd_sink_demux_pad_added (GstElement* ele, GstPad* pad, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
	gchar* name = gst_pad_get_name (pad);
	GstElement* sinkbin = NULL;
	GstPad* sinkpad = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail (wfd_sink && wfd_sink->pipeline);

	if (name[0] == 'v')
	{
		wfd_sink_debug("=========== >>>>>>>>>> Received VIDEO pad...\n");

		MMWFDSINK_PAD_PROBE( wfd_sink, pad, NULL,  NULL );

		gst_pad_add_probe(pad,
			GST_PAD_PROBE_TYPE_BUFFER,
			_mm_wfd_sink_check_running_time,
			(gpointer)wfd_sink,
			NULL);

		if (GST_STATE(wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst) <= GST_STATE_NULL)
		{
			wfd_sink_debug ("need to prepare videobin" );
			if (MM_ERROR_NONE !=__mm_wfd_sink_prepare_videobin (wfd_sink))
			{
				wfd_sink_error ("failed to prepare videobin....\n");
				goto ERROR;
			}
		}

		sinkbin = wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst;

		wfd_sink->added_av_pad_num++;
	}
	else if (name[0] == 'a')
	{
		wfd_sink_debug("=========== >>>>>>>>>> Received AUDIO pad...\n");

		MMWFDSINK_PAD_PROBE( wfd_sink, pad, NULL,  NULL );

		gst_pad_add_probe(pad,
			GST_PAD_PROBE_TYPE_BUFFER,
			_mm_wfd_sink_check_running_time,
			(gpointer)wfd_sink,
			NULL);

		if (GST_STATE(wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst) <= GST_STATE_NULL)
		{
			wfd_sink_debug ("need to prepare audiobin" );
			if (MM_ERROR_NONE !=__mm_wfd_sink_prepare_audiobin (wfd_sink))
			{
				wfd_sink_error ("failed to prepare audiobin....\n");
				goto ERROR;
			}
		}

		sinkbin = wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst;

		wfd_sink->added_av_pad_num++;
	}
	else
	{
		wfd_sink_error("not handling.....\n\n\n");
	}


	if (sinkbin)
	{
		wfd_sink_debug("add %s to pipeline.\n",
			GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));

		/* add */
		if(!gst_bin_add (GST_BIN(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst), sinkbin))
		{
			wfd_sink_error("failed to add sinkbin to pipeline\n");
			goto ERROR;
		}

		wfd_sink_debug("link %s .\n", GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));

		/* link */
		sinkpad = gst_element_get_static_pad (GST_ELEMENT_CAST(sinkbin), "sink");
		if (!sinkpad)
		{
			wfd_sink_error("failed to get pad from sinkbin\n");
			goto ERROR;
		}

		if (GST_PAD_LINK_OK != gst_pad_link_full (pad, sinkpad, GST_PAD_LINK_CHECK_NOTHING))
		{
			wfd_sink_error("failed to link sinkbin\n");
			goto ERROR;
		}

		wfd_sink_debug("sync state %s with pipeline .\n",
			GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));

		/* run */
		if (!gst_element_sync_state_with_parent (GST_ELEMENT_CAST(sinkbin)))
		{
			wfd_sink_error ("failed to sync sinkbin state with parent \n");
			goto ERROR;
		}

		gst_object_unref(GST_OBJECT(sinkpad));
		sinkpad = NULL;
	}


	if (wfd_sink->added_av_pad_num == 2)
	{
		wfd_sink_debug("whole pipeline is constructed. \n");

		/* generate dot file of the constructed pipeline of wifi display sink */
		MMWFDSINK_GENERATE_DOT_IF_ENABLED( wfd_sink, "constructed-pipeline" );
	}

	MMWFDSINK_FREEIF(name);

	wfd_sink_debug_fleave();

	return;

/* ERRORS */
ERROR:
	MMWFDSINK_FREEIF(name);

	if (sinkpad)
		gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;

	/* need to notify to app */
	MMWFDSINK_POST_MESSAGE (wfd_sink,
		MM_ERROR_WFD_INTERNAL,
		MMWFDSINK_CURRENT_STATE(wfd_sink));

	return;
}

static void
__mm_wfd_sink_change_av_format (GstElement* wfdrtspsrc, gpointer* need_to_flush, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail (wfd_sink);
	wfd_sink_return_if_fail (need_to_flush);

	if (MMWFDSINK_CURRENT_STATE(wfd_sink)==MM_WFD_SINK_STATE_PLAYING)
	{
		wfd_sink_debug("need to flush pipeline");
		*need_to_flush = (gpointer) TRUE;
	}
	else
	{
		wfd_sink_debug("don't need to flush pipeline");
		*need_to_flush = (gpointer) FALSE;
	}


	wfd_sink_debug_fleave();
}


static void
__mm_wfd_sink_update_stream_info (GstElement* wfdrtspsrc, GstStructure* str, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
	WFDSinkManagerCMDType cmd = WFD_SINK_MANAGER_CMD_NONE;
	MMWFDSinkStreamInfo * stream_info = NULL;
	gint is_valid_audio_format = FALSE;
	gint is_valid_video_format = FALSE;
	gchar * audio_format;
	gchar * video_format;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail (str && GST_IS_STRUCTURE(str));
	wfd_sink_return_if_fail (wfd_sink);

	stream_info = &wfd_sink->stream_info;

	if (gst_structure_has_field (str, "audio_format"))
	{
		is_valid_audio_format = TRUE;
		audio_format = g_strdup(gst_structure_get_string(str, "audio_format"));
		if (g_strrstr(audio_format,"AAC"))
			stream_info->audio_stream_info.codec = WFD_SINK_AUDIO_CODEC_AAC;
		else if (g_strrstr(audio_format,"AC3"))
			stream_info->audio_stream_info.codec = WFD_SINK_AUDIO_CODEC_AC3;
		else if (g_strrstr(audio_format,"LPCM"))
			stream_info->audio_stream_info.codec = WFD_SINK_AUDIO_CODEC_LPCM;
		else
		{
			wfd_sink_error ("invalid audio format(%s)...\n", audio_format);
			is_valid_audio_format = FALSE;
		}

		if (is_valid_audio_format == TRUE)
		{
			if (gst_structure_has_field (str, "audio_rate"))
				gst_structure_get_int (str, "audio_rate", &stream_info->audio_stream_info.sample_rate);
			if (gst_structure_has_field (str, "audio_channels"))
				gst_structure_get_int (str, "audio_channels", &stream_info->audio_stream_info.channels);
			if (gst_structure_has_field (str, "audio_bitwidth"))
				gst_structure_get_int (str, "audio_bitwidth", &stream_info->audio_stream_info.bitwidth);

			cmd = cmd | WFD_SINK_MANAGER_CMD_LINK_A_BIN;

			wfd_sink_debug ("audio_format : %s \n \t rate :	%d \n \t channels :  %d \n \t bitwidth :  %d \n \t	\n",
				audio_format,
				stream_info->audio_stream_info.sample_rate,
				stream_info->audio_stream_info.channels,
				stream_info->audio_stream_info.bitwidth);
		}
	}

	if (gst_structure_has_field (str, "video_format"))
	{
		is_valid_video_format = TRUE;
		video_format = g_strdup(gst_structure_get_string(str, "video_format"));
		if (!g_strrstr(video_format,"H264"))
		{
			wfd_sink_error ("invalid video format(%s)...\n", video_format);
			is_valid_video_format = FALSE;
		}

		if(is_valid_video_format == TRUE)
		{
			stream_info->video_stream_info.codec = WFD_SINK_VIDEO_CODEC_H264;

			if (gst_structure_has_field (str, "video_width"))
				gst_structure_get_int (str, "video_width", &stream_info->video_stream_info.width);
			if (gst_structure_has_field (str, "video_height"))
				gst_structure_get_int (str, "video_height", &stream_info->video_stream_info.height);
			if (gst_structure_has_field (str, "video_framerate"))
				gst_structure_get_int (str, "video_framerate", &stream_info->video_stream_info.frame_rate);

			cmd = cmd | WFD_SINK_MANAGER_CMD_LINK_V_BIN;

			wfd_sink_debug("video_format : %s \n \t width :  %d \n \t height :  %d \n \t frame_rate :  %d \n \t  \n",
				video_format,
				stream_info->video_stream_info.width,
				stream_info->video_stream_info.height,
				stream_info->video_stream_info.frame_rate);
		}
	}

	if (cmd != WFD_SINK_MANAGER_CMD_NONE)
	{
		WFD_SINK_MANAGER_LOCK(wfd_sink);
		WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink, cmd);
		WFD_SINK_MANAGER_UNLOCK(wfd_sink);
	}

	wfd_sink_debug_fleave();
}

static int __mm_wfd_sink_prepare_wfdrtspsrc(mm_wfd_sink_t *wfd_sink, GstElement *wfdrtspsrc)
{
	GstStructure *audio_param = NULL;
	GstStructure *video_param = NULL;
	GstStructure *hdcp_param = NULL;
	void *hdcp_handle = NULL;
	gint hdcp_version = 0;
	gint hdcp_port = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail (wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail (wfdrtspsrc, MM_ERROR_WFD_NOT_INITIALIZED);

	g_object_set (G_OBJECT(wfdrtspsrc), "debug", wfd_sink->ini.set_debug_property, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "latency", wfd_sink->ini.jitter_buffer_latency, NULL);
#if 0
	g_object_set (G_OBJECT(wfdrtspsrc), "do-request", wfd_sink->ini.enable_retransmission, NULL);
#endif
	g_object_set (G_OBJECT(wfdrtspsrc), "udp-buffer-size", 2097152, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "enable-pad-probe", wfd_sink->ini.enable_wfdrtspsrc_pad_probe, NULL);

	audio_param = gst_structure_new ("audio_param",
		"audio_codec", G_TYPE_UINT, wfd_sink->ini.audio_codec,
		"audio_latency", G_TYPE_UINT, wfd_sink->ini.audio_latency,
		"audio_channels", G_TYPE_UINT, wfd_sink->ini.audio_channel,
		"audio_sampling_frequency", G_TYPE_UINT, wfd_sink->ini.audio_sampling_frequency,
		NULL);

	video_param = gst_structure_new ("video_param",
		"video_codec", G_TYPE_UINT, wfd_sink->ini.video_codec,
		"video_native_resolution", G_TYPE_UINT, wfd_sink->ini.video_native_resolution,
		"video_cea_support", G_TYPE_UINT, wfd_sink->ini.video_cea_support,
		"video_vesa_support", G_TYPE_UINT, wfd_sink->ini.video_vesa_support,
		"video_hh_support", G_TYPE_UINT, wfd_sink->ini.video_hh_support,
		"video_profile", G_TYPE_UINT, wfd_sink->ini.video_profile,
		"video_level", G_TYPE_UINT, wfd_sink->ini.video_level,
		"video_latency", G_TYPE_UINT, wfd_sink->ini.video_latency,
		"video_vertical_resolution", G_TYPE_INT, wfd_sink->ini.video_vertical_resolution,
		"video_horizontal_resolution", G_TYPE_INT, wfd_sink->ini.video_horizontal_resolution,
		"video_minimum_slicing", G_TYPE_INT, wfd_sink->ini.video_minimum_slicing,
		"video_slice_enc_param", G_TYPE_INT, wfd_sink->ini.video_slice_enc_param,
		"video_framerate_control_support", G_TYPE_INT, wfd_sink->ini.video_framerate_control_support,
		NULL);

	mm_attrs_get_data_by_name(wfd_sink->attrs, "hdcp_handle", &hdcp_handle);
	mm_attrs_get_int_by_name(wfd_sink->attrs, "hdcp_version", &hdcp_version);
	mm_attrs_get_int_by_name(wfd_sink->attrs, "hdcp_port", &hdcp_port);
	wfd_sink_debug("set hdcp version %d with %d port\n", hdcp_version, hdcp_port);

	hdcp_param = gst_structure_new ("hdcp_param",
			"hdcp_version", G_TYPE_INT, hdcp_version,
			"hdcp_port_no", G_TYPE_INT, hdcp_port,
			NULL);

	g_object_set (G_OBJECT(wfdrtspsrc), "audio-param", audio_param, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "video-param", video_param, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "hdcp-param", hdcp_param, NULL);

	g_signal_connect (wfdrtspsrc, "update-media-info",
		G_CALLBACK(__mm_wfd_sink_update_stream_info), wfd_sink);

	g_signal_connect (wfdrtspsrc, "change-av-format",
		G_CALLBACK(__mm_wfd_sink_change_av_format), wfd_sink);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_prepare_demux(mm_wfd_sink_t *wfd_sink, GstElement *demux)
{
	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail (demux, MM_ERROR_WFD_NOT_INITIALIZED);

	g_signal_connect (demux, "pad-added",
		G_CALLBACK(__mm_wfd_sink_demux_pad_added),	wfd_sink);

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_create_pipeline(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *mainbin = NULL;
	GList* element_bucket = NULL;
	GstBus	*bus = NULL;
	int i;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail (wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);

	/* Create pipeline */
	wfd_sink->pipeline = (MMWFDSinkGstPipelineInfo*) g_malloc0 (sizeof(MMWFDSinkGstPipelineInfo));
	if (wfd_sink->pipeline == NULL)
		goto CREATE_ERROR;

	memset (wfd_sink->pipeline, 0, sizeof(MMWFDSinkGstPipelineInfo));

	/* create mainbin */
	mainbin = (MMWFDSinkGstElement*) g_malloc0 (sizeof(MMWFDSinkGstElement) * WFD_SINK_M_NUM);
	if (mainbin == NULL)
		goto CREATE_ERROR;

	memset (mainbin, 0, sizeof(MMWFDSinkGstElement) * WFD_SINK_M_NUM);

	/* create pipeline */
	mainbin[WFD_SINK_M_PIPE].id = WFD_SINK_M_PIPE;
	mainbin[WFD_SINK_M_PIPE].gst = gst_pipeline_new ("wfdsink");
	if (!mainbin[WFD_SINK_M_PIPE].gst)
	{
		wfd_sink_error ("failed to create pipeline\n");
		goto CREATE_ERROR;
	}

	/* create wfdrtspsrc */
	MMWFDSINK_CREATE_ELEMENT(mainbin, WFD_SINK_M_SRC, "wfdrtspsrc", "wfdsink_source", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_SRC].gst,  "src" );
	if (mainbin[WFD_SINK_M_SRC].gst)
	{
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_wfdrtspsrc (wfd_sink, mainbin[WFD_SINK_M_SRC].gst))
		{
			wfd_sink_error ("failed to prepare wfdrtspsrc...\n");
			goto CREATE_ERROR;
		}
	}

	/* create rtpmp2tdepay */
	MMWFDSINK_CREATE_ELEMENT (mainbin, WFD_SINK_M_DEPAY, "rtpmp2tdepay", "wfdsink_depay", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_DEPAY].gst, "src" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_DEPAY].gst, "sink" );

	MMWFDSINK_TS_DATA_DUMP( wfd_sink, mainbin[WFD_SINK_M_DEPAY].gst, "src" );

	/* create tsdemuxer*/
	MMWFDSINK_CREATE_ELEMENT (mainbin, WFD_SINK_M_DEMUX, wfd_sink->ini.name_of_tsdemux, "wfdsink_demux", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_DEMUX].gst, "sink" );
	if (mainbin[WFD_SINK_M_DEMUX].gst)
	{
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_demux (wfd_sink, mainbin[WFD_SINK_M_DEMUX].gst))
		{
			wfd_sink_error ("failed to prepare demux...\n");
			goto CREATE_ERROR;
		}
	}

	/* adding created elements to pipeline */
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin (GST_BIN_CAST(mainbin[WFD_SINK_M_PIPE].gst), element_bucket, FALSE))
	{
		wfd_sink_error ("failed to add elements\n");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		wfd_sink_error("failed to link elements\n");
		goto CREATE_ERROR;
	}

	/* connect bus callback */
	bus = gst_pipeline_get_bus (GST_PIPELINE(mainbin[WFD_SINK_M_PIPE].gst));
	if (!bus)
	{
		wfd_sink_error ("cannot get bus from pipeline.\n");
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
	wfd_sink_error("ERROR : releasing pipeline\n");

	if (element_bucket)
		g_list_free (element_bucket);
	element_bucket = NULL;

	/* finished */
	if (bus)
		gst_object_unref(GST_OBJECT(bus));
	bus = NULL;

	/* release element which are not added to bin */
	for (i = 1; i < WFD_SINK_M_NUM; i++) 	/* NOTE : skip pipeline */
	{
		if (mainbin != NULL && mainbin[i].gst)
		{
			GstObject* parent = NULL;
			parent = gst_element_get_parent (mainbin[i].gst);

			if (!parent)
			{
				gst_object_unref(GST_OBJECT(mainbin[i].gst));
				mainbin[i].gst = NULL;
			}
			else
			{
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release audiobin with it's childs */
	if (mainbin != NULL && mainbin[WFD_SINK_M_PIPE].gst )
	{
		gst_object_unref (GST_OBJECT(mainbin[WFD_SINK_M_PIPE].gst));
	}

	MMWFDSINK_FREEIF (mainbin);

	MMWFDSINK_FREEIF (wfd_sink->pipeline);

	return MM_ERROR_WFD_INTERNAL;
}

int __mm_wfd_sink_link_audiobin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement 	*audiobin = NULL;
	MMWFDSinkGstElement *first_element = NULL;
	MMWFDSinkGstElement *last_element = NULL;
	gint audio_codec = WFD_SINK_AUDIO_CODEC_NONE;
	GList* element_bucket = NULL;
	GstPad *sinkpad = NULL;
	GstPad *srcpad = NULL;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->audiobin &&
		wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst,
		MM_ERROR_WFD_NOT_INITIALIZED );
	wfd_sink_return_val_if_fail (wfd_sink->prev_audio_dec_src_pad &&
		wfd_sink->next_audio_dec_sink_pad,
		MM_ERROR_WFD_INTERNAL);

	if (wfd_sink->audio_bin_is_linked)
	{
		wfd_sink_debug ("audiobin is already linked... nothing to do\n");
		return MM_ERROR_NONE;
	}

	/* take audiobin */
	audiobin = wfd_sink->pipeline->audiobin;

	/* check audio codec */
	audio_codec = wfd_sink->stream_info.audio_stream_info.codec;
	switch (audio_codec)
	{
		case WFD_SINK_AUDIO_CODEC_LPCM:
			if (audiobin[WFD_SINK_A_LPCM_CONVERTER].gst)
				element_bucket = g_list_append(element_bucket, &audiobin[WFD_SINK_A_LPCM_CONVERTER]);
			if (audiobin[WFD_SINK_A_LPCM_FILTER].gst)
				element_bucket = g_list_append(element_bucket, &audiobin[WFD_SINK_A_LPCM_FILTER]);
			break;

		case WFD_SINK_AUDIO_CODEC_AAC:
			if (audiobin[WFD_SINK_A_AAC_PARSE].gst)
				element_bucket = g_list_append(element_bucket, &audiobin[WFD_SINK_A_AAC_PARSE]);
			if (audiobin[WFD_SINK_A_AAC_DEC].gst)
				element_bucket = g_list_append(element_bucket, &audiobin[WFD_SINK_A_AAC_DEC]);
			break;

		case WFD_SINK_AUDIO_CODEC_AC3:
			if (audiobin[WFD_SINK_A_AC3_PARSE].gst)
				element_bucket = g_list_append(element_bucket, &audiobin[WFD_SINK_A_AC3_PARSE]);
			if (audiobin[WFD_SINK_A_AC3_DEC].gst)
				element_bucket = g_list_append(element_bucket, &audiobin[WFD_SINK_A_AC3_DEC]);
			break;

		default:
			wfd_sink_error("audio type is not decied yet. cannot link audio decoder...\n");
			return MM_ERROR_WFD_INTERNAL;
			break;
	}

	if (!element_bucket)
	{
		wfd_sink_debug ("there is no additional elements to be linked... just link audiobin.\n");
		if (GST_PAD_LINK_OK != gst_pad_link_full(wfd_sink->prev_audio_dec_src_pad, wfd_sink->next_audio_dec_sink_pad, GST_PAD_LINK_CHECK_NOTHING))
		{
			wfd_sink_error("failed to link audiobin....\n");
			goto fail_to_link;
		}
		goto done;
	}

	/* adding elements to audiobin */
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), element_bucket, FALSE))
	{
		wfd_sink_error ("failed to add elements to audiobin\n");
		goto fail_to_link;
	}

	/* linking elements in the bucket by added order. */
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		wfd_sink_error("failed to link elements\n");
		goto fail_to_link;
	}

	/* get src pad */
	first_element = (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, 0);
	if (!first_element)
	{
		wfd_sink_error("failed to get first element to be linked....\n");
		goto fail_to_link;
	}

	sinkpad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!sinkpad)
	{
		wfd_sink_error("failed to get sink pad from element(%s)\n", GST_ELEMENT_NAME(first_element->gst));
		goto fail_to_link;
	}

	if (GST_PAD_LINK_OK != gst_pad_link_full(wfd_sink->prev_audio_dec_src_pad, sinkpad, GST_PAD_LINK_CHECK_NOTHING))
	{
		wfd_sink_error("failed to link audiobin....\n");
		goto fail_to_link;
	}

	gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;


	/* get sink pad */
	last_element = (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, g_list_length(element_bucket)-1);
	if (!last_element)
	{
		wfd_sink_error("failed to get last element to be linked....\n");
		goto fail_to_link;
	}

	srcpad = gst_element_get_static_pad(GST_ELEMENT(last_element->gst), "src");
	if (!srcpad)
	{
		wfd_sink_error("failed to get src pad from element(%s)\n", GST_ELEMENT_NAME(last_element->gst));
		goto fail_to_link;
	}

	if (GST_PAD_LINK_OK != gst_pad_link_full(srcpad, wfd_sink->next_audio_dec_sink_pad, GST_PAD_LINK_CHECK_NOTHING))
	{
		wfd_sink_error("failed to link audiobin....\n");
		goto fail_to_link;
	}

	gst_object_unref(GST_OBJECT(srcpad));
	srcpad = NULL;

	g_list_free (element_bucket);

done:
	wfd_sink->audio_bin_is_linked = TRUE;

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

	g_list_free (element_bucket);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_prepare_audiosink(mm_wfd_sink_t *wfd_sink, GstElement *audio_sink)
{
	wfd_sink_debug_fenter();

	/* check audiosink is created */
	wfd_sink_return_val_if_fail (audio_sink, MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	g_object_set (G_OBJECT(audio_sink), "provide-clock", FALSE,  NULL);
	g_object_set (G_OBJECT(audio_sink), "buffer-time", 100000LL, NULL);
	g_object_set (G_OBJECT(audio_sink), "query-position-support", FALSE,  NULL);
	g_object_set (G_OBJECT(audio_sink), "slave-method", 2,  NULL);
	g_object_set (G_OBJECT(audio_sink), "async", wfd_sink->ini.audio_sink_async,  NULL);
	g_object_set (G_OBJECT(audio_sink), "ts-offset", (gint64)wfd_sink->ini.sink_ts_offset, NULL );

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static void
__mm_wfd_sink_queue_overrun (GstElement* element, gpointer u_data)
{
        debug_fenter();

        return_if_fail(element);

        wfd_sink_warning ("%s is overrun\n",
			GST_STR_NULL(GST_ELEMENT_NAME(element)));

        debug_fleave();

        return;
}

static int  __mm_wfd_sink_destroy_audiobin(mm_wfd_sink_t* wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement* audiobin = NULL;
	GstObject* parent = NULL;
	int i;

	wfd_sink_debug_fenter ();

	wfd_sink_return_val_if_fail (wfd_sink,
		MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline &&
		wfd_sink->pipeline->audiobin &&
		wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst)
	{
		audiobin = wfd_sink->pipeline->audiobin;
	}
	else
	{
		wfd_sink_debug("audiobin is not created, nothing to destroy\n");
		return MM_ERROR_NONE;
	}


	parent = gst_element_get_parent (audiobin[WFD_SINK_A_BIN].gst);
	if (!parent)
	{
		wfd_sink_debug("audiobin has no parent.. need to relase by itself\n");

		if (GST_STATE(audiobin[WFD_SINK_A_BIN].gst)>=GST_STATE_READY)
		{
			wfd_sink_debug("try to change state of audiobin to NULL\n");
			ret = gst_element_set_state (audiobin[WFD_SINK_A_BIN].gst, GST_STATE_NULL);
			if ( ret != GST_STATE_CHANGE_SUCCESS )
			{
				wfd_sink_error("failed to change state of audiobin to NULL\n");
				return MM_ERROR_WFD_INTERNAL;
			}
		}

		/* release element which are not added to bin */
		for (i = 1; i < WFD_SINK_A_NUM; i++) 	/* NOTE : skip bin */
		{
			if (audiobin[i].gst)
			{
				parent = gst_element_get_parent (audiobin[i].gst);
				if (!parent)
				{
					wfd_sink_debug("unref %s(current ref %d)\n",
						GST_STR_NULL(GST_ELEMENT_NAME(audiobin[i].gst)),
						((GObject *) audiobin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(audiobin[i].gst));
					audiobin[i].gst = NULL;
				}
				else
				{
					wfd_sink_debug("unref %s(current ref %d)\n",
						GST_STR_NULL(GST_ELEMENT_NAME(audiobin[i].gst)),
						((GObject *) audiobin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(parent));
				}
			}
		}

		/* release audiobin with it's childs */
		if (audiobin[WFD_SINK_A_BIN].gst )
		{
			gst_object_unref (GST_OBJECT(audiobin[WFD_SINK_A_BIN].gst));
		}
	}
	else
	{
		wfd_sink_debug("audiobin has parent (%s), unref it \n",
			GST_STR_NULL(GST_OBJECT_NAME(GST_OBJECT(parent))));

		gst_object_unref (GST_OBJECT(parent));
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_create_audiobin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement* audiobin = NULL;
	gint audio_codec = WFD_SINK_AUDIO_CODEC_NONE;
	gboolean link_audio_dec = TRUE;
	GList* element_bucket = NULL;
	GstPad *pad = NULL;
	GstPad *ghostpad = NULL;
	GstCaps *caps = NULL;
	gint i;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin,
		MM_ERROR_WFD_NOT_INITIALIZED );

	/* alloc handles */
	audiobin = (MMWFDSinkGstElement*)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_A_NUM);
	if (!audiobin)
	{
		wfd_sink_error ("failed to allocate memory for audiobin\n");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* create audiobin */
	audiobin[WFD_SINK_A_BIN].id = WFD_SINK_A_BIN;
	audiobin[WFD_SINK_A_BIN].gst = gst_bin_new("audiobin");
	if ( !audiobin[WFD_SINK_A_BIN].gst )
	{
		wfd_sink_error ("failed to create audiobin\n");
		goto CREATE_ERROR;
	}

	/* check audio decoder could be linked or not */
	switch (wfd_sink->stream_info.audio_stream_info.codec)
	{
		case WFD_SINK_AUDIO_CODEC_AAC:
			audio_codec = WFD_AUDIO_AAC;
			break;
		case WFD_SINK_AUDIO_CODEC_AC3:
			audio_codec = WFD_AUDIO_AC3;
			break;
		case WFD_SINK_AUDIO_CODEC_LPCM:
			audio_codec = WFD_AUDIO_LPCM;
			break;
		case WFD_SINK_AUDIO_CODEC_NONE:
		default:
			wfd_sink_debug ("audio decoder could NOT be linked now, just prepare.\n");
			audio_codec = wfd_sink->ini.audio_codec;
			link_audio_dec = FALSE;
			break;
	}

	/* set need to link audio decoder flag*/
	wfd_sink->audio_bin_is_linked = link_audio_dec;

	/* queue - drm - parse - dec/capsfilter -  audioconvert- volume - sink */
	/* create queue */
	MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_QUEUE, "queue", "audio_queue", link_audio_dec);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_QUEUE].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_QUEUE].gst,  "src" );
	if (audiobin[WFD_SINK_A_QUEUE].gst)
	{
		g_object_set(G_OBJECT (audiobin[WFD_SINK_A_QUEUE].gst), "max-size-bytes", 0, NULL);
		g_object_set(G_OBJECT (audiobin[WFD_SINK_A_QUEUE].gst), "max-size-buffers", 0, NULL);
		g_object_set(G_OBJECT (audiobin[WFD_SINK_A_QUEUE].gst), "max-size-time", (guint64)3000000000ULL, NULL);
		g_signal_connect (audiobin[WFD_SINK_A_QUEUE].gst, "overrun",
		        G_CALLBACK(__mm_wfd_sink_queue_overrun), wfd_sink);
	}
	if (!link_audio_dec)
	{
		if (!gst_bin_add (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), audiobin[WFD_SINK_A_QUEUE].gst))
		{
			wfd_sink_error("failed to add %s to audiobin\n",
				GST_STR_NULL(GST_ELEMENT_NAME(audiobin[WFD_SINK_A_QUEUE].gst)));
			goto CREATE_ERROR;
		}

		if (audiobin[WFD_SINK_A_HDCP].gst)
		{
			if (!gst_bin_add (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), audiobin[WFD_SINK_A_HDCP].gst))
			{
				wfd_sink_error("failed to add %s to audiobin\n",
					GST_STR_NULL(GST_ELEMENT_NAME(audiobin[WFD_SINK_A_HDCP].gst)));
				goto CREATE_ERROR;
			}

			if (!gst_element_link (audiobin[WFD_SINK_A_QUEUE].gst, audiobin[WFD_SINK_A_HDCP].gst) )
			{
				wfd_sink_error("failed to link [%s] to [%s] success\n",
					GST_STR_NULL(GST_ELEMENT_NAME(audiobin[WFD_SINK_A_QUEUE].gst)),
					GST_STR_NULL(GST_ELEMENT_NAME(audiobin[WFD_SINK_A_HDCP].gst)));
				goto CREATE_ERROR;
			}

			wfd_sink->prev_audio_dec_src_pad = gst_element_get_static_pad(audiobin[WFD_SINK_A_HDCP].gst, "src");
		}
		else
		{
			wfd_sink->prev_audio_dec_src_pad = gst_element_get_static_pad(audiobin[WFD_SINK_A_QUEUE].gst, "src");
		}

		if (!wfd_sink->prev_audio_dec_src_pad)
		{
			wfd_sink_error ("failed to get src pad from previous element of audio decoder\n");
			goto CREATE_ERROR;
		}

		wfd_sink_debug ("take src pad from previous element of audio decoder for linking\n");
	}

	if (audio_codec & WFD_AUDIO_LPCM)
	{
		/* create LPCM converter */
		MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_LPCM_CONVERTER, wfd_sink->ini.name_of_lpcm_converter, "audio_lpcm_convert", link_audio_dec);
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_LPCM_CONVERTER].gst,  "sink" );
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_LPCM_CONVERTER].gst,  "src" );

		/* create LPCM filter */
		MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_LPCM_FILTER, wfd_sink->ini.name_of_lpcm_filter, "audio_lpcm_filter", link_audio_dec);
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_LPCM_FILTER].gst,  "sink" );
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_LPCM_FILTER].gst,  "src" );
		if (audiobin[WFD_SINK_A_LPCM_FILTER].gst)
		{
			caps = gst_caps_new_simple("audio/x-raw",
					"rate", G_TYPE_INT, 48000,
					"channels", G_TYPE_INT, 2,
					"format", G_TYPE_STRING, "S16LE", NULL);

			g_object_set (G_OBJECT(audiobin[WFD_SINK_A_LPCM_FILTER].gst), "caps", caps, NULL);
			gst_object_unref (GST_OBJECT(caps));
		}
	}

	if (audio_codec & WFD_AUDIO_AAC )
	{
		/* create AAC parse  */
		MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_AAC_PARSE, wfd_sink->ini.name_of_aac_parser, "audio_aac_parser", link_audio_dec);
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AAC_PARSE].gst,  "sink" );
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AAC_PARSE].gst,  "src" );

		/* create AAC decoder  */
		MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_AAC_DEC, wfd_sink->ini.name_of_aac_decoder, "audio_aac_dec", link_audio_dec);
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AAC_DEC].gst,  "sink" );
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AAC_DEC].gst,  "src" );
	}

	if (audio_codec & WFD_AUDIO_AC3)
	{
		/* create AC3 parser  */
		MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_AC3_PARSE, wfd_sink->ini.name_of_ac3_parser, "audio_ac3_parser", link_audio_dec);
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AC3_PARSE].gst,  "sink" );
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AC3_PARSE].gst,  "src" );

		/* create AC3 decoder  */
		MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_AC3_DEC, wfd_sink->ini.name_of_ac3_decoder, "audio_ac3_dec", link_audio_dec);
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AC3_DEC].gst,  "sink" );
		MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_AC3_DEC].gst,  "src" );
	}

	/* create resampler */
	MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_RESAMPLER, wfd_sink->ini.name_of_audio_resampler, "audio_resampler", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_RESAMPLER].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_RESAMPLER].gst,  "src" );

	/* create volume */
	MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_VOLUME, wfd_sink->ini.name_of_audio_volume, "audio_volume", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_VOLUME].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_VOLUME].gst,  "src" );

  	//TODO gstreamer-1.0 alsasink does not want process not S16LE format.
	MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_CAPSFILTER, "capsfilter", "audio_capsfilter", TRUE);
	if (audiobin[WFD_SINK_A_CAPSFILTER].gst)
	{
		caps = gst_caps_from_string("audio/x-raw, format=(string)S16LE");
		g_object_set (G_OBJECT(audiobin[WFD_SINK_A_CAPSFILTER].gst), "caps", caps, NULL);
		gst_caps_unref (caps);
	}

	/* create sink */
	MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_SINK, wfd_sink->ini.name_of_audio_sink, "audio_sink", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_SINK].gst,  "sink" );
	if (audiobin[WFD_SINK_A_SINK].gst)
	{
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_audiosink(wfd_sink, audiobin[WFD_SINK_A_SINK].gst))
		{
			wfd_sink_error ("failed to set audio sink property....\n");
			goto CREATE_ERROR;
		}
	}

	if (!link_audio_dec)
	{
		MMWFDSinkGstElement* first_element = NULL;

		first_element = (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, 0);
		if (!first_element)
		{
			wfd_sink_error("failed to get first element\n");
			goto CREATE_ERROR;
		}

		wfd_sink->next_audio_dec_sink_pad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
		if (!wfd_sink->next_audio_dec_sink_pad)
		{
			wfd_sink_error("failed to get sink pad from next element of audio decoder\n");
			goto CREATE_ERROR;
		}

		wfd_sink_debug ("take sink pad from next element of audio decoder for linking\n");
	}

	/* adding created elements to audiobin */
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), element_bucket, FALSE))
	{
		wfd_sink_error ("failed to add elements\n");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		wfd_sink_error("failed to link elements\n");
		goto CREATE_ERROR;
	}

	/* get queue's sinkpad for creating ghostpad */
	pad = gst_element_get_static_pad(audiobin[WFD_SINK_A_QUEUE].gst, "sink");
	if (!pad)
	{
		wfd_sink_error("failed to get pad from queue of audiobin\n");
		goto CREATE_ERROR;
	}

	ghostpad = gst_ghost_pad_new ("sink", pad);
	if (!ghostpad)
	{
		wfd_sink_error("failed to create ghostpad\n");
		goto CREATE_ERROR;
	}

	if (FALSE == gst_element_add_pad (audiobin[WFD_SINK_A_BIN].gst, ghostpad) )
	{
		wfd_sink_error("failed to add ghostpad to audiobin\n");
		goto CREATE_ERROR;
	}

	gst_object_unref(GST_OBJECT(pad));

	g_list_free(element_bucket);

	/* take it */
	wfd_sink->pipeline->audiobin = audiobin;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

CREATE_ERROR:
	wfd_sink_error("failed to create audiobin, releasing all\n");

	if (wfd_sink->next_audio_dec_sink_pad)
		gst_object_unref (GST_OBJECT(wfd_sink->next_audio_dec_sink_pad));
	wfd_sink->next_audio_dec_sink_pad = NULL;

	if (wfd_sink->prev_audio_dec_src_pad)
		gst_object_unref (GST_OBJECT(wfd_sink->prev_audio_dec_src_pad));
	wfd_sink->prev_audio_dec_src_pad = NULL;

	if (pad)
		gst_object_unref (GST_OBJECT(pad));
	pad = NULL;

	if (ghostpad)
		gst_object_unref (GST_OBJECT(ghostpad));
	ghostpad = NULL;

	if (element_bucket)
		g_list_free (element_bucket);
	element_bucket = NULL;

	/* release element which are not added to bin */
	__mm_wfd_sink_destroy_audiobin(wfd_sink);

	MMWFDSINK_FREEIF (audiobin);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_prepare_videodec(mm_wfd_sink_t *wfd_sink, GstElement *video_dec)
{
	wfd_sink_debug_fenter();

	/* check video decoder is created */
	wfd_sink_return_val_if_fail ( video_dec, MM_ERROR_WFD_INVALID_ARGUMENT );
	wfd_sink_return_val_if_fail ( wfd_sink && wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED );

	g_object_set (G_OBJECT(video_dec), "error-concealment", TRUE, NULL );

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_prepare_videosink(mm_wfd_sink_t *wfd_sink, GstElement *video_sink)
{
	gboolean visible = TRUE;
	gint surface_type = MM_DISPLAY_SURFACE_X;
	gulong xid = 0;

	wfd_sink_debug_fenter();

	/* check videosink is created */
	wfd_sink_return_val_if_fail ( video_sink, MM_ERROR_WFD_INVALID_ARGUMENT );
	wfd_sink_return_val_if_fail ( wfd_sink && wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED );

	/* update display surface */
//	mm_attrs_get_int_by_name(wfd_sink->attrs, "display_surface_type", &surface_type);
	wfd_sink_debug("check display surface type attribute: %d", surface_type);
	mm_attrs_get_int_by_name(wfd_sink->attrs, "display_visible", &visible);
	wfd_sink_debug("check display visible attribute: %d", visible);

	/* configuring display */
	switch ( surface_type )
	{
		case MM_DISPLAY_SURFACE_EVAS:
		{
			void *object = NULL;
			gint scaling = 0;

			/* common case if using evas surface */
			mm_attrs_get_data_by_name(wfd_sink->attrs, "display_overlay", &object);
			mm_attrs_get_int_by_name(wfd_sink->attrs, "display_evas_do_scaling", &scaling);
			if (object)
			{
				wfd_sink_debug("set video param : evas-object %x", object);
				g_object_set(G_OBJECT(video_sink), "evas-object", object, NULL);
			}
			else
			{
				wfd_sink_error("no evas object");
				return MM_ERROR_WFD_INTERNAL;
			}
		}
		break;

		case MM_DISPLAY_SURFACE_X:
		{
			int *object = NULL;

			/* x surface */
			mm_attrs_get_data_by_name(wfd_sink->attrs, "display_overlay", (void**)&object);
			if (object)
			{
				xid = *object;
				wfd_sink_debug("xid = %lu", xid);
				gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink), xid);
			}
			else
			{
				wfd_sink_warning("Handle is NULL. Set xid as 0.. but, it's not recommended.");
				gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink), 0);
			}
		}
		break;

		case MM_DISPLAY_SURFACE_NULL:
		{
			/* do nothing */
			wfd_sink_error("Not Supported Surface.");
			return MM_ERROR_WFD_INTERNAL;
		}
		break;
	}

	g_object_set (G_OBJECT(video_sink), "qos", FALSE, NULL );
	g_object_set (G_OBJECT(video_sink), "async", wfd_sink->ini.video_sink_async, NULL);
	g_object_set (G_OBJECT(video_sink), "max-lateness", (gint64)wfd_sink->ini.video_sink_max_lateness, NULL );
	g_object_set (G_OBJECT(video_sink), "visible", visible, NULL);
	g_object_set (G_OBJECT(video_sink), "ts-offset", (gint64)(wfd_sink->ini.sink_ts_offset), NULL );

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_destroy_videobin(mm_wfd_sink_t* wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement* videobin = NULL;
	GstObject* parent = NULL;
	int i;

	wfd_sink_debug_fenter ();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED );

	if (wfd_sink->pipeline &&
		wfd_sink->pipeline->videobin &&
		wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst)
	{
		videobin = wfd_sink->pipeline->videobin;
	}
	else
	{
		wfd_sink_debug("videobin is not created, nothing to destroy\n");
		return MM_ERROR_NONE;
	}


	parent = gst_element_get_parent (videobin[WFD_SINK_V_BIN].gst);
	if (!parent)
	{
		wfd_sink_debug("videobin has no parent.. need to relase by itself\n");

		if (GST_STATE(videobin[WFD_SINK_V_BIN].gst) >= GST_STATE_READY)
		{
			wfd_sink_debug("try to change state of videobin to NULL\n");
			ret = gst_element_set_state (videobin[WFD_SINK_V_BIN].gst, GST_STATE_NULL);
			if ( ret != GST_STATE_CHANGE_SUCCESS )
			{
				wfd_sink_error("failed to change state of videobin to NULL\n");
				return MM_ERROR_WFD_INTERNAL;
			}
		}
		/* release element which are not added to bin */
		for (i = 1; i < WFD_SINK_V_NUM; i++) 	/* NOTE : skip bin */
		{
			if (videobin[i].gst)
			{
				parent = gst_element_get_parent (videobin[i].gst);
				if (!parent)
				{
					wfd_sink_debug("unref %s(current ref %d)\n",
						GST_STR_NULL(GST_ELEMENT_NAME(videobin[i].gst)),
						((GObject *) videobin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(videobin[i].gst));
					videobin[i].gst = NULL;
				}
				else
				{
					wfd_sink_debug("unref %s(current ref %d)\n",
						GST_STR_NULL(GST_ELEMENT_NAME(videobin[i].gst)),
						((GObject *) videobin[i].gst)->ref_count);
					gst_object_unref(GST_OBJECT(parent));
				}
			}
		}
		/* release audiobin with it's childs */
		if (videobin[WFD_SINK_V_BIN].gst )
		{
			gst_object_unref (GST_OBJECT(videobin[WFD_SINK_V_BIN].gst));
		}
	}
	else
	{
		wfd_sink_debug("videobin has parent (%s), unref it \n",
			GST_STR_NULL(GST_OBJECT_NAME(GST_OBJECT(parent))));

		gst_object_unref (GST_OBJECT(parent));
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}


static int __mm_wfd_sink_create_videobin(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement* first_element = NULL;
	MMWFDSinkGstElement* videobin = NULL;
	GList* element_bucket = NULL;
	GstPad *pad = NULL;
	GstPad *ghostpad = NULL;
	gint i = 0;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin,
		MM_ERROR_WFD_NOT_INITIALIZED );

	/* alloc handles */
	videobin = (MMWFDSinkGstElement*)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_V_NUM);
	if (!videobin )
	{
		wfd_sink_error ("failed to allocate memory for videobin\n");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* create videobin */
	videobin[WFD_SINK_V_BIN].id = WFD_SINK_V_BIN;
	videobin[WFD_SINK_V_BIN].gst = gst_bin_new("videobin");
	if ( !videobin[WFD_SINK_V_BIN].gst )
	{
		wfd_sink_error ("failed to create videobin\n");
		goto CREATE_ERROR;
	}

	/* queue - drm - parse - dec - sink */
	/* create queue */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_QUEUE, "queue", "video_queue", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_QUEUE].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_QUEUE].gst,  "src" );
	if (videobin[WFD_SINK_V_QUEUE].gst)
	{
		g_object_set(G_OBJECT (videobin[WFD_SINK_V_QUEUE].gst), "max-size-bytes", 0, NULL);
		g_object_set(G_OBJECT (videobin[WFD_SINK_V_QUEUE].gst), "max-size-buffers", 0, NULL);
		g_object_set(G_OBJECT (videobin[WFD_SINK_V_QUEUE].gst), "max-size-time", (guint64)3000000000ULL, NULL);
		g_signal_connect (videobin[WFD_SINK_V_QUEUE].gst, "overrun",
			G_CALLBACK(__mm_wfd_sink_queue_overrun), wfd_sink);
	}

	/* create parser */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_PARSE, wfd_sink->ini.name_of_video_parser, "video_parser", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_PARSE].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_PARSE].gst,  "src" );
	if (videobin[WFD_SINK_V_PARSE].gst)
		g_object_set (G_OBJECT(videobin[WFD_SINK_V_PARSE].gst), "wfd-mode", TRUE, NULL );

	/* create dec */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_DEC, wfd_sink->ini.name_of_video_decoder, "video_dec", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_DEC].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_DEC].gst,  "src" );
	if (videobin[WFD_SINK_V_DEC].gst)
	{
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_videodec(wfd_sink, videobin[WFD_SINK_V_DEC].gst))
		{
			wfd_sink_error ("failed to set video sink property....\n");
			goto CREATE_ERROR;
		}
	}

	/* create sink */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_SINK, wfd_sink->ini.name_of_video_sink, "video_sink", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_SINK].gst,  "sink" );
	if (videobin[WFD_SINK_V_SINK].gst)
	{
		if (MM_ERROR_NONE != __mm_wfd_sink_prepare_videosink(wfd_sink, videobin[WFD_SINK_V_SINK].gst))
		{
			wfd_sink_error ("failed to set video sink property....\n");
			goto CREATE_ERROR;
		}
	}

	/* adding created elements to videobin */
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin(GST_BIN_CAST(videobin[WFD_SINK_V_BIN].gst), element_bucket, FALSE))
	{
		wfd_sink_error ("failed to add elements\n");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		wfd_sink_error("failed to link elements\n");
		goto CREATE_ERROR;
	}

	/* get first element's sinkpad for creating ghostpad */
	first_element =  (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, 0);
	if (!first_element)
	{
		wfd_sink_error("failed to get first element of videobin\n");
		goto CREATE_ERROR;
	}

	pad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!pad)
	{
		wfd_sink_error("failed to get pad from first element(%s) of videobin\n", GST_ELEMENT_NAME(first_element->gst));
		goto CREATE_ERROR;
	}

	ghostpad = gst_ghost_pad_new ("sink", pad);
	if (!ghostpad)
	{
		wfd_sink_error("failed to create ghostpad\n");
		goto CREATE_ERROR;
	}

	if (FALSE == gst_element_add_pad (videobin[WFD_SINK_V_BIN].gst, ghostpad) )
	{
		wfd_sink_error("failed to add ghostpad to videobin\n");
		goto CREATE_ERROR;
	}

	gst_object_unref(GST_OBJECT(pad));

	g_list_free(element_bucket);

	wfd_sink->video_bin_is_linked = TRUE;

	/* take it */
	wfd_sink->pipeline->videobin = videobin;

	if (wfd_sink->ini.video_sink_async)
	{
		GstBus *bus = NULL;
		bus = gst_element_get_bus(videobin[WFD_SINK_V_BIN].gst );
		if (bus)
			gst_bus_set_sync_handler(bus, _mm_wfd_bus_sync_callback, wfd_sink, NULL);
		gst_object_unref(bus);
	}

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;

/* ERRORS */
CREATE_ERROR:
	wfd_sink_error("failed to create videobin, releasing all\n");

	if (pad)
		gst_object_unref (GST_OBJECT(pad));
	pad = NULL;

	if (ghostpad)
		gst_object_unref (GST_OBJECT(ghostpad));
	ghostpad = NULL;

	g_list_free (element_bucket);

	__mm_wfd_sink_destroy_videobin(wfd_sink);

	MMWFDSINK_FREEIF (videobin);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_destroy_pipeline(mm_wfd_sink_t* wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

	wfd_sink_debug_fenter ();

	wfd_sink_return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED );

	if (wfd_sink->prev_audio_dec_src_pad)
		gst_object_unref(GST_OBJECT(wfd_sink->prev_audio_dec_src_pad));
	wfd_sink->prev_audio_dec_src_pad = NULL;

	if (wfd_sink->next_audio_dec_sink_pad)
		gst_object_unref(GST_OBJECT(wfd_sink->next_audio_dec_sink_pad));
	wfd_sink->next_audio_dec_sink_pad = NULL;

	/* cleanup gst stuffs */
	if ( wfd_sink->pipeline )
	{
		MMWFDSinkGstElement* mainbin = wfd_sink->pipeline->mainbin;

		if ( mainbin )
		{
			MMWFDSinkGstElement* audiobin = wfd_sink->pipeline->audiobin;
			MMWFDSinkGstElement* videobin = wfd_sink->pipeline->videobin;

			if (MM_ERROR_NONE != __mm_wfd_sink_destroy_videobin(wfd_sink))
			{
				wfd_sink_error("failed to destroy videobin\n");
				return MM_ERROR_WFD_INTERNAL;
			}

			if (MM_ERROR_NONE != __mm_wfd_sink_destroy_audiobin(wfd_sink))
			{
				wfd_sink_error("failed to destroy audiobin\n");
				return MM_ERROR_WFD_INTERNAL;
			}

			ret = gst_element_set_state (mainbin[WFD_SINK_M_PIPE].gst, GST_STATE_NULL);
			if ( ret != GST_STATE_CHANGE_SUCCESS )
			{
				wfd_sink_error("failed to change state of mainbin to NULL\n");
				return MM_ERROR_WFD_INTERNAL;
			}

			gst_object_unref(GST_OBJECT(mainbin[WFD_SINK_M_PIPE].gst));

			MMWFDSINK_FREEIF( audiobin );
			MMWFDSINK_FREEIF( videobin );
			MMWFDSINK_FREEIF( mainbin );
		}

		MMWFDSINK_FREEIF( wfd_sink->pipeline );
	}

	wfd_sink->added_av_pad_num = 0;
	wfd_sink->audio_bin_is_linked = FALSE;
	wfd_sink->video_bin_is_linked = FALSE;
	wfd_sink->need_to_reset_basetime = FALSE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}

static void
__mm_wfd_sink_dump_pipeline_state(mm_wfd_sink_t *wfd_sink)
{
	GstIterator*iter = NULL;
	gboolean done = FALSE;

	GstElement *item = NULL;
	GstElementFactory *factory = NULL;

	GstState state = GST_STATE_VOID_PENDING;
	GstState pending = GST_STATE_VOID_PENDING;
	GstClockTime time = 200*GST_MSECOND;

	wfd_sink_debug_fenter();

	wfd_sink_return_if_fail ( wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);

	iter = gst_bin_iterate_recurse(GST_BIN(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst));

	if ( iter != NULL )
	{
		while (!done)
		{
			switch ( gst_iterator_next (iter, (gpointer)&item) )
			{
				case GST_ITERATOR_OK:
					gst_element_get_state(GST_ELEMENT (item),&state, &pending, time);

					factory = gst_element_get_factory (item) ;
					if (factory)
					{
						 wfd_sink_error("%s:%s : From:%s To:%s refcount : %d\n",
						 	GST_STR_NULL(GST_OBJECT_NAME(factory)),
						 	GST_STR_NULL(GST_ELEMENT_NAME(item)),
							gst_element_state_get_name(state),
							gst_element_state_get_name(pending),
							GST_OBJECT_REFCOUNT_VALUE(item));
					}
					 gst_object_unref (item);
					 break;
				 case GST_ITERATOR_RESYNC:
					 gst_iterator_resync (iter);
					 break;
				 case GST_ITERATOR_ERROR:
					 done = TRUE;
					 break;
				 case GST_ITERATOR_DONE:
					 done = TRUE;
					 break;
			 }
		}
	}

	item = GST_ELEMENT_CAST(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst);

	gst_element_get_state(GST_ELEMENT (item),&state, &pending, time);

	factory = gst_element_get_factory (item) ;
	if (factory)
	{
		wfd_sink_error("%s:%s : From:%s To:%s refcount : %d\n",
			GST_OBJECT_NAME(factory),
			GST_ELEMENT_NAME(item),
			gst_element_state_get_name(state),
			gst_element_state_get_name(pending),
			GST_OBJECT_REFCOUNT_VALUE(item) );
	}

	if ( iter )
		gst_iterator_free (iter);

	wfd_sink_debug_fleave();

	return;
}

const gchar *
__mm_wfds_sink_get_state_name ( MMWFDSinkStateType state )
{
	switch ( state )
	{
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
