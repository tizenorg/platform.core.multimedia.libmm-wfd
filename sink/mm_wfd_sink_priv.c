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
#include <mm_debug.h>

#include "mm_wfd_sink_util.h"
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_manager.h"


/* gstreamer */
static int __mm_wfd_sink_init_gstreamer(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_pipeline(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_videobin(mm_wfd_sink_t *wfd_sink);
static int __mm_wfd_sink_create_audiobin(mm_wfd_sink_t *wfd_sink);
 static int __mm_wfd_sink_destroy_pipeline(mm_wfd_sink_t* wfd_sink);
static int __mm_wfd_sink_set_pipeline_state(mm_wfd_sink_t* wfd_sink, GstState state, gboolean async);

/* state */
static int __mm_wfd_sink_check_state(mm_wfd_sink_t* wfd_sink, MMWfdSinkCommandType cmd);
static int __mm_wfd_sink_set_state(mm_wfd_sink_t* wfd_sink, MMWfdSinkStateType state);

/* util */
static void __mm_wfd_sink_dump_pipeline_state(mm_wfd_sink_t *wfd_sink);
const gchar * __mm_wfds_sink_get_state_name ( MMWfdSinkStateType state );

int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink)
{
	int result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	mm_wfd_sink_t *new_wfd_sink = NULL;

	/* create handle */
	new_wfd_sink = g_malloc0(sizeof(mm_wfd_sink_t));
	if (!new_wfd_sink)
	{
		debug_error("failed to allocate memory for wi-fi display sink\n");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* Initialize gstreamer related */
	new_wfd_sink->attrs = 0;

	new_wfd_sink->pipeline = NULL;
	new_wfd_sink->is_constructed = FALSE;
	new_wfd_sink->added_av_pad_num = 0;
	new_wfd_sink->audio_bin_is_linked = FALSE;
	new_wfd_sink->video_bin_is_linked = FALSE;
	new_wfd_sink->audio_bin_is_prepared = FALSE;
	new_wfd_sink->video_bin_is_prepared = FALSE;
	new_wfd_sink->prev_audio_dec_src_pad = NULL;
	new_wfd_sink->next_audio_dec_sink_pad = NULL;
	new_wfd_sink->need_to_reset_basetime = FALSE;
	new_wfd_sink->demux_video_pad_probe_id = 0;
	new_wfd_sink->demux_audio_pad_probe_id = 0;
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
	new_wfd_sink->cmd_lock = NULL;

	/* Initialize resource related */
	new_wfd_sink->resource_list = NULL;

	/* Initialize manager related */
	new_wfd_sink->manager_thread = NULL;
	new_wfd_sink->manager_thread_mutex = NULL;
	new_wfd_sink->manager_thread_cond = NULL;
	new_wfd_sink->manager_thread_cmd = WFD_SINK_MANAGER_CMD_NONE;

	/* construct attributes */
	new_wfd_sink->attrs = _mmwfd_construct_attribute((MMHandleType)new_wfd_sink);
	if (!new_wfd_sink->attrs)
	{
		MMWFDSINK_FREEIF (new_wfd_sink);
		debug_error("failed to set attribute\n");
		return MM_ERROR_WFD_INTERNAL;
	}

	/* load ini for initialize */
	result = mm_wfd_sink_ini_load(&new_wfd_sink->ini);
	if (result != MM_ERROR_NONE)
	{
		debug_error("failed to load ini file\n");
		goto fail_to_load_ini;
	}
	new_wfd_sink->need_to_reset_basetime = new_wfd_sink->ini.enable_reset_basetime;

	/* initialize manager */
	result = _mm_wfd_sink_init_manager(new_wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		debug_error("failed to init manager : %d\n", result);
		goto fail_to_init;
	}

	/* initialize gstreamer */
	result = __mm_wfd_sink_init_gstreamer(new_wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		debug_error("failed to init gstreamer : %d\n", result);
		goto fail_to_init;
	}

	/* set state */
	__mm_wfd_sink_set_state(new_wfd_sink,  MM_WFD_SINK_STATE_NULL);

	/* now take handle */
	*wfd_sink = new_wfd_sink;

	debug_fleave();

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

int _mm_wfd_sink_realize (mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_REALIZE);

	/* construct pipeline */
	/* create main pipeline */
	result = __mm_wfd_sink_create_pipeline(wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		debug_error("failed to create pipeline : %d\n", result);
		goto fail_to_create;
	}

	/* create videobin */
	result = __mm_wfd_sink_create_videobin(wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		debug_error("failed to create videobin : %d\n", result);
		goto fail_to_create;
	}

	/* create audiobin */
	result = __mm_wfd_sink_create_audiobin(wfd_sink);
	if (result < MM_ERROR_NONE)
	{
		debug_error("fail to create audiobin : %d\n", result);
		goto fail_to_create;
	}

	/* set pipeline READY state */
	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_READY, TRUE);
	if (result < MM_ERROR_NONE)
	{
		debug_error("failed to set state : %d\n", result);
		goto fail_to_create;
	}

	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_READY);

	debug_fleave();

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

	debug_fenter();

	return_val_if_fail(uri && strlen(uri) > strlen("rtsp://"),
		MM_ERROR_WFD_INVALID_ARGUMENT);
	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_DEPAY].gst &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_DEMUX].gst,
		MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_CONNECT);

	debug_error ("No-error: try to connect to %s.....\n", GST_STR_NULL(uri));

	/* set uri to wfdrtspsrc */
	g_object_set (G_OBJECT(wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst), "location", uri, NULL);

	/* set pipeline PAUSED state */
	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_PAUSED, TRUE);
	if (result < MM_ERROR_NONE)
	{
		debug_error("failed to set state : %d\n", result);
		return result;
	}

	debug_fleave();

	return result;
}

int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	/* MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_START); */

	WFD_SINK_MANAGER_LOCK(wfd_sink) ;
	debug_error("No-error:need to check ready to play");
	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	result = __mm_wfd_sink_set_pipeline_state(wfd_sink, GST_STATE_PLAYING, TRUE);
	if (result < MM_ERROR_NONE)
	{
		debug_error("failed to set state : %d\n", result);
		return result;
	}

	debug_fleave();

	return result;
}

int _mm_wfd_sink_stop(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_STOP);

	result = __mm_wfd_sink_set_pipeline_state (wfd_sink, GST_STATE_READY, FALSE);
	if (result < MM_ERROR_NONE)
	{
		debug_error("fail to set state : %d\n", result);
		return result;
	}

	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_READY);

	debug_fleave();

	return result;
}

int _mm_wfd_sink_unrealize(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_UNREALIZE);

	/* release pipeline */
	result =  __mm_wfd_sink_destroy_pipeline (wfd_sink);
	if ( result != MM_ERROR_NONE)
	{
		debug_error("failed to destory pipeline\n");
		return MM_ERROR_WFD_INTERNAL;
	}

	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_NULL);

	debug_fleave();

	return result;
}

int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	/* check current wi-fi display sink state */
	MMWFDSINK_CHECK_STATE (wfd_sink, MM_WFD_SINK_COMMAND_DESTROY);

	/* unload ini */
	mm_wfd_sink_ini_unload(&wfd_sink->ini);

	/* release attributes */
	_mmwfd_deconstruct_attribute(wfd_sink->attrs);

	if (MM_ERROR_NONE != _mm_wfd_sink_release_manager(wfd_sink))
	{
		debug_error("failed to release manager\n");
		return MM_ERROR_WFD_INTERNAL;
	}


	/* set state */
	__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_NONE);

	debug_fleave();

	return result;
}

int _mm_wfd_set_message_callback(mm_wfd_sink_t *wfd_sink, MMWFDMessageCallback callback, void *user_data)
{
	int result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	wfd_sink->msg_cb = callback;
	wfd_sink->msg_user_data = user_data;

	debug_fleave();

	return result;
}

void __mm_wfd_post_message(mm_wfd_sink_t* wfd_sink, MMWfdSinkStateType msgtype)
{
	if (wfd_sink->msg_cb)
	{
		debug_error("No-error: Message (type : %d) will be posted using user callback\n", msgtype);
		wfd_sink->msg_cb(msgtype, wfd_sink->msg_user_data);
	}
}

static int __mm_wfd_sink_init_gstreamer(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	gint* argc = NULL;
	gchar** argv = NULL;
	static const int max_argc = 50;
	GError *err = NULL;
	gint i = 0;

	debug_fenter();

	/* alloc */
	argc = calloc(1, sizeof(gint) );
	argv = calloc(max_argc, sizeof(gchar*));
	if (!argc || !argv)
	{
		debug_error("Cannot allocate memory for wfdsink\n");

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
			debug_log("set %s\n", wfd_sink->ini.gst_param[i]);
			argv[*argc] = g_strdup(wfd_sink->ini.gst_param[i]);
			(*argc)++;
		}
	}

	debug_log("initializing gstreamer with following parameter\n");
	debug_log("argc : %d\n", *argc);

	for ( i = 0; i < *argc; i++ )
	{
		debug_log("argv[%d] : %s\n", i, argv[i]);
	}

	/* initializing gstreamer */
	if ( ! gst_init_check (argc, &argv, &err))
	{
		debug_error("Could not initialize GStreamer: %s\n", err ? err->message : "unknown error occurred");
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

	debug_fleave();

	return result;
}



int
_mm_wfd_sink_get_resource(mm_wfd_sink_t* wfd_sink)
{
	gint result = MM_ERROR_NONE;

	debug_fenter();

	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
		MM_ERROR_WFD_NOT_INITIALIZED);

	if (!wfd_sink->resource_list)
	{
		debug_log ("there are no list of resource\n");
		goto done;
	}

done:
	WFD_SINK_MANAGER_LOCK(wfd_sink);
	WFD_SINK_MANAGER_SIGNAL_CMD(wfd_sink,
		WFD_SINK_MANAGER_CMD_PREPARE_A_BIN |WFD_SINK_MANAGER_CMD_PREPARE_V_BIN);
	WFD_SINK_MANAGER_UNLOCK(wfd_sink);

	debug_fleave();

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
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
	GstBusSyncReply ret = GST_BUS_PASS;

	switch (GST_MESSAGE_TYPE (message))
	{
		case GST_MESSAGE_TAG:
		break;
		case GST_MESSAGE_DURATION:
		break;
		case GST_MESSAGE_STATE_CHANGED:
		{
			/* we only handle messages from pipeline */
			if (message->src != GST_OBJECT_CAST(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst))
				ret = GST_BUS_DROP;
		}
		break;
		case GST_MESSAGE_ASYNC_DONE:
		{
			if( message->src != GST_OBJECT_CAST(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst) )
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
	gboolean ret = TRUE;
	gchar** splited_message;
	const gchar* message_structure_name;
	const GstStructure* message_structure = gst_message_get_structure(msg);

	return_val_if_fail (wfd_sink, FALSE);
	return_val_if_fail (msg && GST_IS_MESSAGE(msg), FALSE);

	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_ERROR:
		{
			GError *error = NULL;
			gchar* debug = NULL;

			/* get error code */
			gst_message_parse_error( msg, &error, &debug );

			debug_error("error : %s\n", error->message);
			debug_error("debug : %s\n", debug);

			MMWFDSINK_FREEIF( debug );
			g_error_free( error );
		}
		break;

		case GST_MESSAGE_WARNING:
		{
			char* debug = NULL;
			GError* error = NULL;

			gst_message_parse_warning(msg, &error, &debug);

			debug_error("warning : %s\n", error->message);
			debug_error("debug : %s\n", debug);

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

			debug_error("No-error: state changed [%s] : %s ---> %s final : %s\n",
			GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)),
			gst_element_state_get_name( (GstState)oldstate ),
			gst_element_state_get_name( (GstState)newstate ),
			gst_element_state_get_name( (GstState)pending ) );

			if (oldstate == newstate)
			{
				debug_error("pipeline reports state transition to old state\n");
				break;
			}

			switch(newstate)
			{
				case GST_STATE_VOID_PENDING:
				case GST_STATE_NULL:
				case GST_STATE_READY:
				case GST_STATE_PAUSED:
				break;

				case GST_STATE_PLAYING:
					if (wfd_sink->is_constructed) {
						__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_PLAYING);
						_mm_wfd_sink_correct_pipeline_latency (wfd_sink);
					}
					else
						debug_error ("No-error: pipleline is not constructed yet, intermediate state changing\n");
				break;

				default:
				break;
			}
		}
		break;

		case GST_MESSAGE_CLOCK_LOST:
		{
			GstClock *clock = NULL;
			gst_message_parse_clock_lost (msg, &clock);
			debug_error("No-error: GST_MESSAGE_CLOCK_LOST : %s\n", (clock ? GST_OBJECT_NAME (clock) : "NULL"));
		}
		break;

		case GST_MESSAGE_NEW_CLOCK:
		{
			GstClock *clock = NULL;
			gst_message_parse_new_clock (msg, &clock);
			debug_error("No-error: GST_MESSAGE_NEW_CLOCK : %s\n", (clock ? GST_OBJECT_NAME (clock) : "NULL"));
		}
		break;

		case GST_MESSAGE_APPLICATION:
		{
			message_structure_name = gst_structure_get_name(message_structure);
			debug_error ("No-error: message name : %s", message_structure_name);
			splited_message = g_strsplit((gchar*)message_structure_name, "_", 2);
			if(g_strrstr(message_structure_name, "TEARDOWN"))
			{
				debug_error ("Got TEARDOWN.. Closing..\n");
				//_mm_wfd_sink_stop (wfd_sink);
				__mm_wfd_post_message(wfd_sink, MM_WFD_SINK_STATE_TEARDOWN);
			}
			g_strfreev(splited_message);
		}
		break;

		case GST_MESSAGE_ELEMENT:
		{
			const gchar *structure_name = NULL;
			const GstStructure *structure;
			structure = gst_message_get_structure(msg);
			if (structure == NULL)
				break;
			structure_name = gst_structure_get_name(structure);
			if (structure_name)
				debug_error("No-error: GST_MESSAGE_ELEMENT : %s\n", structure_name);
		}
		break;

		case GST_MESSAGE_PROGRESS:
		{
			GstProgressType type = GST_PROGRESS_TYPE_ERROR;
			gchar *category=NULL, *text=NULL;

			debug_error("No-error: GST_MESSAGE_PROGRESS\n");

			gst_message_parse_progress (msg, &type, &category, &text);

			switch (type)
			{
				case GST_PROGRESS_TYPE_START:
					break;
				case GST_PROGRESS_TYPE_COMPLETE:
					if (category && !strcmp (category, "open"))
					{
						debug_error("No-error: %s\n", GST_STR_NULL(text));
						__mm_wfd_sink_set_state(wfd_sink,  MM_WFD_SINK_STATE_PAUSED);
					}
					break;
				case GST_PROGRESS_TYPE_CANCELED:
					break;
				case GST_PROGRESS_TYPE_ERROR:
					if (category && !strcmp (category, "open"))
					{
						debug_error("got error : %s\n", GST_STR_NULL(text));
						//_mm_wfd_sink_stop (wfd_sink);
						__mm_wfd_post_message(wfd_sink, MM_WFD_SINK_STATE_TEARDOWN);
					}
					else if (category && !strcmp (category, "play"))
					{
						debug_error("got error : %s\n", GST_STR_NULL(text));
						//_mm_wfd_sink_stop (wfd_sink);
						__mm_wfd_post_message(wfd_sink, MM_WFD_SINK_STATE_TEARDOWN);
					}
					else
					{
						debug_error("got error : %s\n", GST_STR_NULL(text));
					}
					break;
				default :
					debug_error("progress message has no type\n");
					return ret;
			}

			debug_error("No-error: %s : %s \n", GST_STR_NULL(category), GST_STR_NULL(text));
			MMWFDSINK_FREEIF (category);
			MMWFDSINK_FREEIF (text);
		}
		break;

		case GST_MESSAGE_UNKNOWN: 	debug_log("GST_MESSAGE_UNKNOWN\n"); break;
		case GST_MESSAGE_INFO:  debug_log("GST_MESSAGE_STATE_DIRTY\n"); break;
		case GST_MESSAGE_TAG: /*debug_log("GST_MESSAGE_TAG\n");*/  break;
		case GST_MESSAGE_BUFFERING: debug_log("GST_MESSAGE_BUFFERING\n");break;
		case GST_MESSAGE_EOS:	debug_log("GST_MESSAGE_EOS received\n");		break;
		case GST_MESSAGE_STATE_DIRTY:	debug_log("GST_MESSAGE_STATE_DIRTY\n"); break;
		case GST_MESSAGE_STEP_DONE:  debug_log("GST_MESSAGE_STEP_DONE\n"); break;
		case GST_MESSAGE_CLOCK_PROVIDE:  debug_log("GST_MESSAGE_CLOCK_PROVIDE\n"); break;
		case GST_MESSAGE_STRUCTURE_CHANGE:	debug_log("GST_MESSAGE_STRUCTURE_CHANGE\n"); break;
		case GST_MESSAGE_STREAM_STATUS:	debug_log("GST_MESSAGE_STREAM_STATUS\n"); break;
		case GST_MESSAGE_SEGMENT_START:	debug_log("GST_MESSAGE_SEGMENT_START\n"); break;
		case GST_MESSAGE_SEGMENT_DONE:	debug_log("GST_MESSAGE_SEGMENT_DONE\n"); break;
		case GST_MESSAGE_DURATION: debug_log("GST_MESSAGE_DURATION\n");break;
		case GST_MESSAGE_LATENCY:	debug_log("GST_MESSAGE_LATENCY\n"); break;
		case GST_MESSAGE_ASYNC_START:	debug_log("GST_MESSAGE_ASYNC_START : %s\n", gst_element_get_name(GST_MESSAGE_SRC(msg))); break;
		case GST_MESSAGE_ASYNC_DONE:  debug_log("GST_MESSAGE_ASYNC_DONE");break;
		case GST_MESSAGE_REQUEST_STATE:		debug_log("GST_MESSAGE_REQUEST_STATE\n");  break;
		case GST_MESSAGE_STEP_START:		debug_log("GST_MESSAGE_STEP_START\n");  break;
		case GST_MESSAGE_QOS:					/*debug_log("GST_MESSAGE_QOS\n");*/  break;
		case GST_MESSAGE_ANY:				debug_log("GST_MESSAGE_ANY\n"); break;
	 	default:
			debug_log("unhandled message\n");
		break;
	}

	return ret;
}


static gboolean
__mm_wfd_sink_gst_sync_state_bucket (GList* element_bucket)
{
	GList* bucket = element_bucket;
	MMWFDSinkGstElement* element = NULL;

	debug_fenter();

	return_val_if_fail (element_bucket, FALSE);

	for (; bucket; bucket = bucket->next)
	{
		element = (MMWFDSinkGstElement*)bucket->data;

		if (element && element->gst)
		{
			if (!gst_element_sync_state_with_parent (GST_ELEMENT(element->gst)))
			{
				debug_error ("fail to sync %s state with parent\n", GST_ELEMENT_NAME(element->gst));
				return FALSE;
			}
		}
	}

	debug_fleave();

	return TRUE;
}


static int
__mm_wfd_sink_gst_element_add_bucket_to_bin (GstBin* bin, GList* element_bucket, gboolean need_prepare)
{
	GList* bucket = element_bucket;
	MMWFDSinkGstElement* element = NULL;
	int successful_add_count = 0;

	debug_fenter();

	return_val_if_fail (element_bucket, 0);
	return_val_if_fail (bin, 0);

	for (; bucket; bucket = bucket->next)
	{
		element = (MMWFDSinkGstElement*)bucket->data;

		if (element && element->gst)
		{
			if (need_prepare)
				gst_element_set_state (GST_ELEMENT(element->gst), GST_STATE_READY);

			if (!gst_bin_add (bin, GST_ELEMENT(element->gst)))
			{
				debug_error("No-error: Adding element [%s]to bin [%s] failed\n",
					GST_ELEMENT_NAME(GST_ELEMENT(element->gst)),
					GST_ELEMENT_NAME(GST_ELEMENT(bin) ) );
				return 0;
			}
			successful_add_count ++;
		}
	}

	debug_fleave();

	return successful_add_count;
}

static int
__mm_wfd_sink_gst_element_link_bucket (GList* element_bucket)
{
	GList* bucket = element_bucket;
	MMWFDSinkGstElement* element = NULL;
	MMWFDSinkGstElement* prv_element = NULL;
	gint successful_link_count = 0;

	debug_fenter();

	return_val_if_fail (element_bucket, -1);

	prv_element = (MMWFDSinkGstElement*)bucket->data;
	bucket = bucket->next;

	for ( ; bucket; bucket = bucket->next )
	{
		element = (MMWFDSinkGstElement*)bucket->data;

		if (element && element->gst)
		{
			if ( gst_element_link (GST_ELEMENT(prv_element->gst), GST_ELEMENT(element->gst)) )
			{
				debug_error("No-error: linking [%s] to [%s] success\n",
					GST_ELEMENT_NAME(GST_ELEMENT(prv_element->gst)),
					GST_ELEMENT_NAME(GST_ELEMENT(element->gst)) );
				successful_link_count ++;
			}
			else
			{
				debug_error("linking [%s] to [%s] failed\n",
					GST_ELEMENT_NAME(GST_ELEMENT(prv_element->gst)),
					GST_ELEMENT_NAME(GST_ELEMENT(element->gst)) );
				return -1;
			}
		}

		prv_element = element;
	}

	debug_fleave();

	return successful_link_count;
}

static int
__mm_wfd_sink_check_state(mm_wfd_sink_t* wfd_sink, MMWfdSinkCommandType cmd)
{
	MMWfdSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

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

		case MM_WFD_SINK_COMMAND_REALIZE:
		{
			if (cur_state == MM_WFD_SINK_STATE_READY)
				goto no_operation;
			else if (cur_state != MM_WFD_SINK_STATE_NULL)
				goto invalid_state;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_READY;
		}
		break;

		case MM_WFD_SINK_COMMAND_CONNECT:
		{
			if (cur_state == MM_WFD_SINK_STATE_PAUSED)
				goto no_operation;
			else if (cur_state != MM_WFD_SINK_STATE_READY)
				goto invalid_state;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PAUSED;
		}
		break;

		case MM_WFD_SINK_COMMAND_START:
		{
			if (cur_state == MM_WFD_SINK_STATE_PLAYING)
				goto no_operation;
			else if (cur_state != MM_WFD_SINK_STATE_PAUSED)
				goto invalid_state;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_PLAYING;
		}
		break;

		case MM_WFD_SINK_COMMAND_STOP:
		{
			if (cur_state == MM_WFD_SINK_STATE_READY || cur_state == MM_WFD_SINK_STATE_NULL || cur_state == MM_WFD_SINK_STATE_NONE)
				goto no_operation;

			MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_READY;
		}
		break;

		case MM_WFD_SINK_COMMAND_UNREALIZE:
		{
			if (cur_state == MM_WFD_SINK_STATE_NULL || cur_state == MM_WFD_SINK_STATE_NONE)
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

	debug_fleave();

	return MM_ERROR_NONE;

no_operation:
	debug_error ("No-error: already %s state, nothing to do.\n", MMWFDSINK_STATE_GET_NAME(cur_state));
	return MM_ERROR_WFD_NO_OP;

/* ERRORS */
invalid_state:
	debug_error ("current state is invalid.\n", MMWFDSINK_STATE_GET_NAME(cur_state));
	return MM_ERROR_WFD_INVALID_STATE;
}

static int __mm_wfd_sink_set_state(mm_wfd_sink_t* wfd_sink, MMWfdSinkStateType state)
{
	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	if (MMWFDSINK_CURRENT_STATE(wfd_sink) == state )
	{
		debug_error("already state(%s)\n", MMWFDSINK_STATE_GET_NAME(state));
		MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NONE;
		return MM_ERROR_NONE;
	}

	/* update wi-fi display state */
	MMWFDSINK_PREVIOUS_STATE(wfd_sink) = MMWFDSINK_CURRENT_STATE(wfd_sink);
	MMWFDSINK_CURRENT_STATE(wfd_sink) = state;

	if ( MMWFDSINK_CURRENT_STATE(wfd_sink) == MMWFDSINK_PENDING_STATE(wfd_sink) )
		MMWFDSINK_PENDING_STATE(wfd_sink) = MM_WFD_SINK_STATE_NONE;

	/* poset state message to application */
	__mm_wfd_post_message(wfd_sink, state);

	/* print state */
	MMWFDSINK_PRINT_STATE(wfd_sink);

	debug_fleave();

	return MM_ERROR_NONE;
}

static int
__mm_wfd_sink_set_pipeline_state(mm_wfd_sink_t* wfd_sink, GstState state, gboolean async)
{
	GstStateChangeReturn result = GST_STATE_CHANGE_FAILURE;
	GstState cur_state = GST_STATE_VOID_PENDING;
	GstState pending_state = GST_STATE_VOID_PENDING;

	debug_fenter();

	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin &&
		wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
		MM_ERROR_WFD_NOT_INITIALIZED);

	return_val_if_fail (state > GST_STATE_VOID_PENDING,
		MM_ERROR_WFD_INVALID_ARGUMENT);

	debug_error ("No-error: try to set %s state \n", gst_element_state_get_name(state));

	result = gst_element_set_state (wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst, state);
	if (result == GST_STATE_CHANGE_FAILURE)
	{
		debug_error ("fail to set %s state....\n", gst_element_state_get_name(state));
		return MM_ERROR_WFD_INTERNAL;
	}

	if (!async)
	{
		debug_error ("No-error: wait for changing state is completed \n");

		result = gst_element_get_state (wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst,
				&cur_state, &pending_state, wfd_sink->ini.state_change_timeout * GST_SECOND);
		if (result == GST_STATE_CHANGE_FAILURE)
		{
			debug_error ("fail to get state within %d seconds....\n", wfd_sink->ini.state_change_timeout);

			__mm_wfd_sink_dump_pipeline_state(wfd_sink);

			return MM_ERROR_WFD_INTERNAL;
		}
		else if (result == GST_STATE_CHANGE_NO_PREROLL)
		{
			debug_error ("No-error: successfully changed state but is not able to provide data yet\n");
		}

		debug_error ("No-error: cur state is %s, pending state is %s\n",
			gst_element_state_get_name(cur_state),
			gst_element_state_get_name(pending_state));
	}


	debug_fleave();

	return MM_ERROR_NONE;
}

static GstPadProbeReturn
_mm_wfd_sink_reset_basetime(GstPad * pad, GstPadProbeInfo * info, gpointer u_data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)u_data;

	debug_fenter();

	if (wfd_sink->need_to_reset_basetime)
	{
		GstClock * clock = NULL;
		GstClockTime base_time = GST_CLOCK_TIME_NONE;

		clock = gst_element_get_clock (GST_ELEMENT_CAST (wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst));
		if (clock)
		{
			base_time = gst_clock_get_time (clock);
			gst_object_unref (clock);
		}

		if (base_time != GST_CLOCK_TIME_NONE)
		{
			debug_error ("No-error: set pipeline basetime as now \n");
			gst_element_set_base_time(GST_ELEMENT_CAST (wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst), base_time);

			wfd_sink->need_to_reset_basetime = FALSE;
		}
	}
	else
	{
		if (wfd_sink->demux_audio_pad_probe_id)
		{
			debug_log ("remove pad probe(%d) for demux audio pad \n", wfd_sink->demux_audio_pad_probe_id);
			gst_pad_remove_probe (pad, wfd_sink->demux_audio_pad_probe_id);
		}
		if (wfd_sink->demux_video_pad_probe_id)
		{
			debug_log ("remove pad probe(%d) for demux video pad \n", wfd_sink->demux_video_pad_probe_id);
			gst_pad_remove_probe (pad, wfd_sink->demux_video_pad_probe_id);
		}
	}

	debug_fleave();

	return TRUE;
}


int
__mm_wfd_sink_prepare_videobin (mm_wfd_sink_t *wfd_sink)
{
	GstElement* videobin = NULL;

	debug_fenter();

	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline,
		MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline->videobin == NULL)
	{
		if (MM_ERROR_NONE !=__mm_wfd_sink_create_videobin (wfd_sink))
		{
			debug_error ("failed to create videobin....\n");
			goto ERROR;
		}
	}
	else
	{
		debug_error ("No-error: videobin is already created.\n");
	}

	videobin = wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst;

	if (!wfd_sink->video_bin_is_prepared)
	{
		if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (videobin, GST_STATE_READY))
		{
			debug_error("failed to set state(READY) to %s\n", GST_STR_NULL(GST_ELEMENT_NAME(videobin)));
			goto ERROR;
		}
		wfd_sink->video_bin_is_prepared = TRUE;
	}

	debug_fleave();

	return MM_ERROR_NONE;

/* ERRORS */
ERROR:
	/* need to notify to app */
	__mm_wfd_post_message(wfd_sink, MM_WFD_SINK_STATE_TEARDOWN);

	return MM_ERROR_WFD_INTERNAL;
}

int
__mm_wfd_sink_prepare_audiobin (mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement* audiobin = NULL;

	debug_fenter();

	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline,
		MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline->audiobin == NULL)
	{
		if (MM_ERROR_NONE !=__mm_wfd_sink_create_audiobin (wfd_sink))
		{
			debug_error ("failed to create audiobin....\n");
			goto ERROR;
		}
	}

	if (!wfd_sink->audio_bin_is_linked)
	{
		if (MM_ERROR_NONE !=__mm_wfd_sink_link_audiobin(wfd_sink))
		{
			debug_error ("failed to link audio decoder.....\n");
			goto ERROR;
		}
	}

	audiobin = wfd_sink->pipeline->audiobin;

	if (!wfd_sink->audio_bin_is_prepared)
	{
		if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (audiobin[WFD_SINK_A_BIN].gst, GST_STATE_READY))
		{
			debug_error("failed to set state(READY) to %s\n", GST_STR_NULL(GST_ELEMENT_NAME(audiobin)));
			goto ERROR;
		}

		wfd_sink->audio_bin_is_prepared = TRUE;
	}

	debug_fleave();

	return MM_ERROR_NONE;

/* ERRORS */
ERROR:
	/* need to notify to app */
	__mm_wfd_post_message(wfd_sink, MM_WFD_SINK_STATE_TEARDOWN);

	return MM_ERROR_WFD_INTERNAL;
}

static void
__mm_wfd_sink_demux_pad_added (GstElement* ele, GstPad* pad, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
	gchar* name = gst_pad_get_name (pad);
	GstElement* sinkbin = NULL;
	GstPad* sinkpad = NULL;

	debug_fenter();

	return_if_fail (wfd_sink && wfd_sink->pipeline);

	if (name[0] == 'v')
	{
		debug_error("No-error:  =========== >>>>>>>>>> Received VIDEO pad...:%s\n", name);

		MMWFDSINK_PAD_PROBE( wfd_sink, pad, NULL,  NULL );

		if (wfd_sink->need_to_reset_basetime)
		{
			debug_error("No-error: need to reset basetime... set pad probe for video pad of demux\n");
			wfd_sink->demux_video_pad_probe_id = gst_pad_add_probe(pad,
					GST_PAD_PROBE_TYPE_BUFFER, _mm_wfd_sink_reset_basetime, wfd_sink, NULL);
		}

		if (!wfd_sink->video_bin_is_prepared)
		{
			debug_error ("need to prepare videobin" );
			if (MM_ERROR_NONE !=__mm_wfd_sink_prepare_videobin (wfd_sink))
			{
				debug_error ("failed to prepare videobin....\n");
				goto ERROR;
			}
		}

		sinkbin = wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst;

		wfd_sink->added_av_pad_num++;
	}
	else if (name[0] == 'a')
	{
		debug_error("No-error:  =========== >>>>>>>>>> Received AUDIO pad...:%s\n", name);

		MMWFDSINK_PAD_PROBE( wfd_sink, pad, NULL,  NULL );

		if (wfd_sink->need_to_reset_basetime)
		{
			debug_error("No-error: need to reset basetime... set pad probe for audio pad of demux\n");
			wfd_sink->demux_audio_pad_probe_id = gst_pad_add_probe(pad,
					GST_PAD_PROBE_TYPE_BUFFER, _mm_wfd_sink_reset_basetime, wfd_sink, NULL);
		}

		if (!wfd_sink->audio_bin_is_prepared)
		{
			debug_error ("need to prepare audiobin" );
			if (MM_ERROR_NONE !=__mm_wfd_sink_prepare_audiobin (wfd_sink))
			{
				debug_error ("failed to prepare audiobin....\n");
				goto ERROR;
			}
		}

		sinkbin = wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst;

		wfd_sink->added_av_pad_num++;
	}
	else
	{
		debug_error("not handling.....\n\n\n");
	}


	if (sinkbin)
	{
		debug_error("No-error:  add %s to pipeline.\n", GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));

		/* add */
		if(!gst_bin_add (GST_BIN(wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst), sinkbin))
		{
			debug_error("failed to add sinkbin to pipeline\n");
			goto ERROR;
		}

		debug_error("No-error:  link %s .\n", GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));

		/* link */
		sinkpad = gst_element_get_static_pad (GST_ELEMENT_CAST(sinkbin), "sink");
		if (!sinkpad)
		{
			debug_error("failed to get pad from sinkbin\n");
			goto ERROR;
		}

		if (GST_PAD_LINK_OK != gst_pad_link_full (pad, sinkpad, GST_PAD_LINK_CHECK_NOTHING))
		{
			debug_error("failed to link sinkbin\n");
			goto ERROR;
		}

		debug_error("No-error:  sync state %s with pipeline .\n", GST_STR_NULL(GST_ELEMENT_NAME(sinkbin)));

		/* run */
		if (!gst_element_sync_state_with_parent (GST_ELEMENT_CAST(sinkbin)))
		{
			debug_error ("failed to sync sinkbin state with parent \n");
			goto ERROR;
		}

		gst_object_unref(GST_OBJECT(sinkpad));
		sinkpad = NULL;
	}


	if (wfd_sink->added_av_pad_num == 2)
	{
		debug_error ("No-error: whole pipeline is constructed. \n");

		/* generate dot file of the constructed pipeline of wifi display sink */
		MMWFD_GENERATE_DOT_IF_ENABLED( wfd_sink, "constructed-pipeline" );

		wfd_sink->is_constructed = TRUE;
	}

	MMWFDSINK_FREEIF(name);

	debug_fleave();

	return;

/* ERRORS */
ERROR:
	MMWFDSINK_FREEIF(name);

	if (sinkpad)
		gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;

	/* need to notify to app */
	__mm_wfd_post_message(wfd_sink, MM_WFD_SINK_STATE_TEARDOWN);

	return;
}

static void
__mm_wfd_sink_change_av_format (GstElement* wfdrtspsrc, gpointer* need_to_flush, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;

	debug_fenter();

	return_if_fail (wfd_sink);
	return_if_fail (need_to_flush);

	if (MMWFDSINK_CURRENT_STATE(wfd_sink)==MM_WFD_SINK_STATE_PLAYING)
	{
		debug_error("No-error: need to flush pipeline");
		*need_to_flush = (gpointer) TRUE;
	}
	else
	{
		debug_error("No-error: don't need to flush pipeline");
		*need_to_flush = (gpointer) FALSE;
	}


	debug_fleave();
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

	debug_fenter();

	return_if_fail (str && GST_IS_STRUCTURE(str));
	return_if_fail (wfd_sink);

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
			debug_error ("invalid audio format(%s)...\n", audio_format);
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

			debug_error ("No-error: audio_format : %s \n \t rate :	%d \n \t channels :  %d \n \t bitwidth :  %d \n \t	\n",
				audio_format, stream_info->audio_stream_info.sample_rate,
				stream_info->audio_stream_info.channels, stream_info->audio_stream_info.bitwidth);
		}
	}

	if (gst_structure_has_field (str, "video_format"))
	{
		is_valid_video_format = TRUE;
		video_format = g_strdup(gst_structure_get_string(str, "video_format"));
		if (!g_strrstr(video_format,"H264"))
		{
			debug_error ("invalid video format(%s)...\n", video_format);
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

			cmd = cmd | WFD_SINK_MANAGER_CMD_LINK_V_BIN ;

			debug_error ("No-error: video_format : %s \n \t width :  %d \n \t height :  %d \n \t frame_rate :  %d \n \t  \n",
				video_format, stream_info->video_stream_info.width,
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

	debug_fleave();
}

static int __mm_wfd_sink_prepare_wfdrtspsrc(mm_wfd_sink_t *wfd_sink, GstElement *wfdrtspsrc)
{
	GstStructure *audio_param = NULL;
	GstStructure *video_param = NULL;
	GstStructure *hdcp_param = NULL;
	void *hdcp_handle = NULL;
	gint hdcp_version = 0;
	gint hdcp_port = 0;

	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	return_val_if_fail (wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);
	return_val_if_fail (wfdrtspsrc, MM_ERROR_WFD_NOT_INITIALIZED);

	g_object_set (G_OBJECT(wfdrtspsrc), "debug", wfd_sink->ini.set_debug_property, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "latency", wfd_sink->ini.jitter_buffer_latency, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "do-request", wfd_sink->ini.enable_retransmission, NULL);
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
	debug_log("set hdcp version %d with %d port\n", hdcp_version, hdcp_port);

	hdcp_param = gst_structure_new ("hdcp_param",
			"hdcp_version", G_TYPE_INT, hdcp_version,
			"hdcp_port_no", G_TYPE_INT, hdcp_port,
			NULL);

	g_object_set (G_OBJECT(wfdrtspsrc), "audio-param", audio_param, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "video-param", video_param, NULL);
	g_object_set (G_OBJECT(wfdrtspsrc), "hdcp-param", hdcp_param, NULL);

	g_signal_connect (wfdrtspsrc, "receive-M4-request",
		G_CALLBACK(__mm_wfd_sink_update_stream_info), wfd_sink);

	g_signal_connect (wfdrtspsrc, "change-av-format",
		G_CALLBACK(__mm_wfd_sink_change_av_format), wfd_sink);

	debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_prepare_demux(mm_wfd_sink_t *wfd_sink, GstElement *demux)
{
	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	return_val_if_fail (demux, MM_ERROR_WFD_NOT_INITIALIZED);

	g_signal_connect (demux, "pad-added", G_CALLBACK(__mm_wfd_sink_demux_pad_added), wfd_sink);

	debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_create_pipeline(mm_wfd_sink_t *wfd_sink)
{
	MMWFDSinkGstElement *mainbin = NULL;
	GList* element_bucket = NULL;
	GstBus	*bus = NULL;
	int i;

	debug_fenter();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	return_val_if_fail (wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED);

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
		debug_error ("failed to create pipeline\n");
		goto CREATE_ERROR;
	}

	/* create wfdrtspsrc */
	MMWFDSINK_CREATE_ELEMENT(mainbin, WFD_SINK_M_SRC, "wfdrtspsrc", "wfdsink_source", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_SRC].gst,  "src" );
	if (MM_ERROR_NONE != __mm_wfd_sink_prepare_wfdrtspsrc (wfd_sink, mainbin[WFD_SINK_M_SRC].gst))
	{
		debug_error ("failed to prepare wfdrtspsrc...\n");
		goto CREATE_ERROR;
	}

	/* create rtpmp2tdepay */
	MMWFDSINK_CREATE_ELEMENT (mainbin, WFD_SINK_M_DEPAY, "rtpmp2tdepay", "wfdsink_depay", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_DEPAY].gst, "src" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_DEPAY].gst, "sink" );

	MMWFDSINK_TS_DATA_DUMP( wfd_sink, mainbin[WFD_SINK_M_DEPAY].gst, "src" );

	/* create tsdemuxer*/
	MMWFDSINK_CREATE_ELEMENT (mainbin, WFD_SINK_M_DEMUX, wfd_sink->ini.name_of_tsdemux, "wfdsink_demux", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, mainbin[WFD_SINK_M_DEMUX].gst, "sink" );
	if (MM_ERROR_NONE != __mm_wfd_sink_prepare_demux (wfd_sink, mainbin[WFD_SINK_M_DEMUX].gst))
	{
		debug_error ("failed to prepare demux...\n");
		goto CREATE_ERROR;
	}

	/* adding created elements to pipeline */
	debug_error("No-error: try to add created elements to pipeline\n");
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin (GST_BIN_CAST(mainbin[WFD_SINK_M_PIPE].gst), element_bucket, FALSE))
	{
		debug_error ("failed to add elements\n");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	debug_error("No-error: try to link elements in the bucket by added order.\n");
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		debug_error("failed to link elements\n");
		goto CREATE_ERROR;
	}

	/* connect bus callback */
	bus = gst_pipeline_get_bus (GST_PIPELINE(mainbin[WFD_SINK_M_PIPE].gst));
	if (!bus)
	{
		debug_error ("cannot get bus from pipeline.\n");
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

	debug_fleave();

	return MM_ERROR_NONE;

/* ERRORS */
CREATE_ERROR:
	debug_error("ERROR : releasing pipeline\n");

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

	debug_fenter();

	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->audiobin &&
		wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst,
		MM_ERROR_WFD_NOT_INITIALIZED );
	return_val_if_fail (wfd_sink->prev_audio_dec_src_pad &&
		wfd_sink->next_audio_dec_sink_pad,
		MM_ERROR_WFD_INTERNAL);

	if (wfd_sink->audio_bin_is_linked)
	{
		debug_error ("No-error : audiobin is already linked... nothing to do\n");
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
			debug_error("audio type is not decied yet. cannot link audio decoder...\n");
			return MM_ERROR_WFD_INTERNAL;
			break;
	}

	if (!element_bucket)
	{
		debug_error ("No-error: there is no additional elements to be linked... just link audiobin.\n");
		if (GST_PAD_LINK_OK != gst_pad_link_full(wfd_sink->prev_audio_dec_src_pad, wfd_sink->next_audio_dec_sink_pad, GST_PAD_LINK_CHECK_NOTHING))
		{
			debug_error("failed to link audiobin....\n");
			goto fail_to_link;
		}
		goto done;
	}

	/* adding elements to audiobin */
	debug_error("No-error: try to add elements to audiobin\n");
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), element_bucket, FALSE))
	{
		debug_error ("failed to add elements to audiobin\n");
		goto fail_to_link;
	}

	/* linking elements in the bucket by added order. */
	debug_error("No-error: try to link elements in the bucket by added order.\n");
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		debug_error("failed to link elements\n");
		goto fail_to_link;
	}

	/* get src pad */
	first_element = (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, 0);
	if (!first_element)
	{
		debug_error("failed to get first element to be linked....\n");
		goto fail_to_link;
	}

	sinkpad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!sinkpad)
	{
		debug_error("failed to get sink pad from element(%s)\n", GST_ELEMENT_NAME(first_element->gst));
		goto fail_to_link;
	}

	if (GST_PAD_LINK_OK != gst_pad_link_full(wfd_sink->prev_audio_dec_src_pad, sinkpad, GST_PAD_LINK_CHECK_NOTHING))
	{
		debug_error("failed to link audiobin....\n");
		goto fail_to_link;
	}

	gst_object_unref(GST_OBJECT(sinkpad));
	sinkpad = NULL;


	/* get sink pad */
	last_element = (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, g_list_length(element_bucket)-1);
	if (!last_element)
	{
		debug_error("failed to get last element to be linked....\n");
		goto fail_to_link;
	}

	srcpad = gst_element_get_static_pad(GST_ELEMENT(last_element->gst), "src");
	if (!srcpad)
	{
		debug_error("failed to get src pad from element(%s)\n", GST_ELEMENT_NAME(last_element->gst));
		goto fail_to_link;
	}

	if (GST_PAD_LINK_OK != gst_pad_link_full(srcpad, wfd_sink->next_audio_dec_sink_pad, GST_PAD_LINK_CHECK_NOTHING))
	{
		debug_error("failed to link audiobin....\n");
		goto fail_to_link;
	}

	gst_object_unref(GST_OBJECT(srcpad));
	srcpad = NULL;

	g_list_free (element_bucket);

done:
	wfd_sink->audio_bin_is_linked = TRUE;

	debug_fleave();

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

	debug_fenter();

	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin,
		MM_ERROR_WFD_NOT_INITIALIZED );

	/* alloc handles */
	audiobin = (MMWFDSinkGstElement*)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_A_NUM);
	if (!audiobin)
	{
		debug_error ("failed to allocate memory for audiobin\n");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* create audiobin */
	audiobin[WFD_SINK_A_BIN].id = WFD_SINK_A_BIN;
	audiobin[WFD_SINK_A_BIN].gst = gst_bin_new("audiobin");
	if ( !audiobin[WFD_SINK_A_BIN].gst )
	{
		debug_error ("failed to create audiobin\n");
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
			debug_error ("No-error: audio decoder could NOT be linked now, just prepare.\n");
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
	g_object_set (G_OBJECT(wfd_sink->pipeline->mainbin[WFD_SINK_M_SRC].gst), "audio-queue-handle", audiobin[WFD_SINK_A_QUEUE].gst, NULL);

	if (!link_audio_dec)
	{
		if (!gst_bin_add (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), audiobin[WFD_SINK_A_QUEUE].gst))
		{
			debug_error("failed to add %s to audiobin\n", GST_ELEMENT_NAME(audiobin[WFD_SINK_A_QUEUE].gst));
			goto CREATE_ERROR;
		}

		if (audiobin[WFD_SINK_A_HDCP].gst)
		{
			if (!gst_bin_add (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), audiobin[WFD_SINK_A_HDCP].gst))
			{
				debug_error("failed to add %s to audiobin\n", GST_ELEMENT_NAME(audiobin[WFD_SINK_A_HDCP].gst));
				goto CREATE_ERROR;
			}

			if (!gst_element_link (audiobin[WFD_SINK_A_QUEUE].gst, audiobin[WFD_SINK_A_HDCP].gst) )
			{
				debug_error("failed to link [%s] to [%s] success\n",
					GST_ELEMENT_NAME(audiobin[WFD_SINK_A_QUEUE].gst),
					GST_ELEMENT_NAME(audiobin[WFD_SINK_A_HDCP].gst) );
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
			debug_error ("failed to get src pad from previous element of audio decoder\n");
			goto CREATE_ERROR;
		}

		debug_error ("No-error: take src pad from previous element of audio decoder for linking\n");
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
		caps = gst_caps_new_simple("audio/x-raw-int",
				"rate", G_TYPE_INT, 48000,
				"channels", G_TYPE_INT, 2,
				"depth", G_TYPE_INT, 16,
				"width", G_TYPE_INT, 16,
				"signed", G_TYPE_BOOLEAN, TRUE,
				"endianness", G_TYPE_INT, 1234, NULL);
		g_object_set (G_OBJECT(audiobin[WFD_SINK_A_LPCM_FILTER].gst), "caps", caps, NULL);
		gst_object_unref (GST_OBJECT(caps));
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
	caps = gst_caps_from_string("audio/x-raw, format=(string)S16LE");
	g_object_set (G_OBJECT(audiobin[WFD_SINK_A_CAPSFILTER].gst), "caps", caps, NULL);
	gst_caps_unref (caps);

	/* create sink */
	MMWFDSINK_CREATE_ELEMENT (audiobin, WFD_SINK_A_SINK, wfd_sink->ini.name_of_audio_sink, "audio_sink", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, audiobin[WFD_SINK_A_SINK].gst,  "sink" );

	g_object_set (G_OBJECT(audiobin[WFD_SINK_A_SINK].gst), "provide-clock", FALSE,  NULL);
	g_object_set (G_OBJECT(audiobin[WFD_SINK_A_SINK].gst), "buffer-time", 100000LL, NULL);
	//g_object_set (G_OBJECT(audiobin[WFD_SINK_A_SINK].gst), "async", FALSE,  NULL);
	g_object_set (G_OBJECT(audiobin[WFD_SINK_A_SINK].gst), "query-position-support", FALSE,  NULL);

	if (g_str_equal(wfd_sink->ini.name_of_audio_sink, "alsasink"))
	{
		g_object_set (G_OBJECT (audiobin[WFD_SINK_A_SINK].gst), "device", "hw:0,0", NULL);

		debug_error("No-error: set device hw0:0 for alsasink");
	}

	if (!link_audio_dec)
	{
		MMWFDSinkGstElement* first_element = NULL;

		first_element = (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, 0);
		if (!first_element)
		{
			debug_error("failed to get first element\n");
			goto CREATE_ERROR;
		}

		wfd_sink->next_audio_dec_sink_pad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
		if (!wfd_sink->next_audio_dec_sink_pad)
		{
			debug_error("failed to get sink pad from next element of audio decoder\n");
			goto CREATE_ERROR;
		}

		debug_error ("No-error: take sink pad from next element of audio decoder for linking\n");
	}

	/* adding created elements to audiobin */
	debug_error("No-error: adding created elements to audiobin\n");
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin (GST_BIN_CAST(audiobin[WFD_SINK_A_BIN].gst), element_bucket, FALSE))
	{
		debug_error ("failed to add elements\n");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	debug_error("No-error: Linking elements in the bucket by added order.\n");
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		debug_error("failed to link elements\n");
		goto CREATE_ERROR;
	}

	/* get queue's sinkpad for creating ghostpad */
	pad = gst_element_get_static_pad(audiobin[WFD_SINK_A_QUEUE].gst, "sink");
	if (!pad)
	{
		debug_error("failed to get pad from queue of audiobin\n");
		goto CREATE_ERROR;
	}

	ghostpad = gst_ghost_pad_new ("sink", pad);
	if (!ghostpad)
	{
		debug_error("failed to create ghostpad\n");
		goto CREATE_ERROR;
	}

	if (FALSE == gst_element_add_pad (audiobin[WFD_SINK_A_BIN].gst, ghostpad) )
	{
		debug_error("failed to add ghostpad to audiobin\n");
		goto CREATE_ERROR;
	}

	gst_object_unref(GST_OBJECT(pad));

	g_list_free(element_bucket);

	/* take it */
	wfd_sink->pipeline->audiobin = audiobin;

	debug_fleave();

	return MM_ERROR_NONE;

CREATE_ERROR:
	debug_error("ERROR : releasing audiobin\n");

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
	for (i = 1; i < WFD_SINK_A_NUM; i++) 	/* NOTE : skip bin */
	{
		if (audiobin[i].gst)
		{
			GstObject* parent = NULL;
			parent = gst_element_get_parent (audiobin[i].gst);

			if (!parent)
			{
				gst_object_unref(GST_OBJECT(audiobin[i].gst));
				audiobin[i].gst = NULL;
			}
			else
			{
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release audiobin with it's childs */
	if (audiobin[WFD_SINK_A_BIN].gst )
	{
		gst_object_unref (GST_OBJECT(audiobin[WFD_SINK_A_BIN].gst));
	}

	MMWFDSINK_FREEIF (audiobin);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_prepare_videodec(mm_wfd_sink_t *wfd_sink, GstElement *video_dec)
{
	debug_fenter();

	/* check video decoder is created */
	return_val_if_fail ( video_dec, MM_ERROR_WFD_INVALID_ARGUMENT );
	return_val_if_fail ( wfd_sink && wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED );

	g_object_set (G_OBJECT(video_dec), "decoding-type", 1, NULL );
	g_object_set (G_OBJECT(video_dec), "error-concealment", TRUE, NULL );

	debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_prepare_videosink(mm_wfd_sink_t *wfd_sink, GstElement *video_sink)
{
	gboolean visible = TRUE;
	gint surface_type = MM_DISPLAY_SURFACE_X;
	gulong xid = 0;

	debug_fenter();

	/* check videosink is created */
	return_val_if_fail ( video_sink, MM_ERROR_WFD_INVALID_ARGUMENT );
	return_val_if_fail ( wfd_sink && wfd_sink->attrs, MM_ERROR_WFD_NOT_INITIALIZED );

	/* update display surface */
//	mm_attrs_get_int_by_name(wfd_sink->attrs, "display_surface_type", &surface_type);
	debug_error("No-error: check display surface type attribute: %d", surface_type);
	mm_attrs_get_int_by_name(wfd_sink->attrs, "display_visible", &visible);
	debug_error("No-error: check display visible attribute: %d", visible);

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
				debug_error("No-error: set video param : evas-object %x", object);
				g_object_set(G_OBJECT(video_sink), "evas-object", object, NULL);
			}
			else
			{
				debug_error("no evas object");
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
				debug_error("No-error: xid = %lu )", xid);
				gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink), xid);
			}
			else
			{
				debug_error("Handle is NULL. Set xid as 0.. but, it's not recommended.");
				gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(video_sink), 0);
			}
		}
		break;

		case MM_DISPLAY_SURFACE_NULL:
		{
			/* do nothing */
			debug_error("Not Supported Surface.");
			return MM_ERROR_WFD_INTERNAL;
		}
		break;
	}

	g_object_set (G_OBJECT(video_sink), "qos", FALSE, NULL );
	//g_object_set (G_OBJECT(video_sink), "async", FALSE, NULL);
	g_object_set (G_OBJECT(video_sink), "max-lateness", (gint64)wfd_sink->ini.video_sink_max_lateness, NULL );
	g_object_set(G_OBJECT(video_sink), "visible", visible, NULL);
	debug_error("No-error: set video param : visible %d", visible);

	debug_fleave();

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

	debug_fenter();

	return_val_if_fail (wfd_sink &&
		wfd_sink->pipeline &&
		wfd_sink->pipeline->mainbin,
		MM_ERROR_WFD_NOT_INITIALIZED );

	/* alloc handles */
	videobin = (MMWFDSinkGstElement*)g_malloc0(sizeof(MMWFDSinkGstElement) * WFD_SINK_V_NUM);
	if (!videobin )
	{
		debug_error ("failed to allocate memory for videobin\n");
		return MM_ERROR_WFD_NO_FREE_SPACE;
	}

	/* create videobin */
	videobin[WFD_SINK_V_BIN].id = WFD_SINK_V_BIN;
	videobin[WFD_SINK_V_BIN].gst = gst_bin_new("videobin");
	if ( !videobin[WFD_SINK_V_BIN].gst )
	{
		debug_error ("failed to create videobin\n");
		goto CREATE_ERROR;
	}

	/* queue - drm - parse - dec - sink */
	/* create queue */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_QUEUE, "queue", "video_queue", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_QUEUE].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_QUEUE].gst,  "src" );

	/* create parser */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_PARSE, wfd_sink->ini.name_of_video_parser, "video_parser", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_PARSE].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_PARSE].gst,  "src" );
	g_object_set (G_OBJECT(videobin[WFD_SINK_V_PARSE].gst), "wfd-mode", TRUE, NULL );

	/* create dec */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_DEC, wfd_sink->ini.name_of_video_decoder, "video_dec", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_DEC].gst,  "sink" );
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_DEC].gst,  "src" );
	if (MM_ERROR_NONE != __mm_wfd_sink_prepare_videodec(wfd_sink, videobin[WFD_SINK_V_DEC].gst))
	{
		debug_error ("failed to set video sink property....\n");
		goto CREATE_ERROR;
	}

	/* create sink */
	MMWFDSINK_CREATE_ELEMENT (videobin, WFD_SINK_V_SINK, wfd_sink->ini.name_of_video_sink, "video_sink", TRUE);
	MMWFDSINK_PAD_PROBE( wfd_sink, NULL, videobin[WFD_SINK_V_SINK].gst,  "sink" );
	if (MM_ERROR_NONE != __mm_wfd_sink_prepare_videosink(wfd_sink, videobin[WFD_SINK_V_SINK].gst))
	{
		debug_error ("failed to set video sink property....\n");
		goto CREATE_ERROR;
	}

	/* adding created elements to videobin */
	debug_error("No-error: adding created elements to videobin\n");
	if( !__mm_wfd_sink_gst_element_add_bucket_to_bin (GST_BIN_CAST(videobin[WFD_SINK_V_BIN].gst), element_bucket, FALSE))
	{
		debug_error ("failed to add elements\n");
		goto CREATE_ERROR;
	}

	/* linking elements in the bucket by added order. */
	debug_error("No-error: Linking elements in the bucket by added order.\n");
	if ( __mm_wfd_sink_gst_element_link_bucket (element_bucket) == -1 )
	{
		debug_error("failed to link elements\n");
		goto CREATE_ERROR;
	}

	/* get first element's sinkpad for creating ghostpad */
	first_element =  (MMWFDSinkGstElement *)g_list_nth_data(element_bucket, 0);
	if (!first_element)
	{
		debug_error("failed to get first element of videobin\n");
		goto CREATE_ERROR;
	}

	pad = gst_element_get_static_pad(GST_ELEMENT(first_element->gst), "sink");
	if (!pad)
	{
		debug_error("failed to get pad from first element(%s) of videobin\n", GST_ELEMENT_NAME(first_element->gst));
		goto CREATE_ERROR;
	}

	ghostpad = gst_ghost_pad_new ("sink", pad);
	if (!ghostpad)
	{
		debug_error("failed to create ghostpad\n");
		goto CREATE_ERROR;
	}

	if (FALSE == gst_element_add_pad (videobin[WFD_SINK_V_BIN].gst, ghostpad) )
	{
		debug_error("failed to add ghostpad to videobin\n");
		goto CREATE_ERROR;
	}

	gst_object_unref(GST_OBJECT(pad));

	g_list_free(element_bucket);

	wfd_sink->video_bin_is_linked = TRUE;

	/* take it */
	wfd_sink->pipeline->videobin = videobin;

	debug_fleave();

	return MM_ERROR_NONE;

/* ERRORS */
CREATE_ERROR:
	debug_error("ERROR : releasing videobin\n");

	if (pad)
		gst_object_unref (GST_OBJECT(pad));
	pad = NULL;

	if (ghostpad)
		gst_object_unref (GST_OBJECT(ghostpad));
	ghostpad = NULL;

	g_list_free (element_bucket);

	/* release element which are not added to bin */
	for (i = 1; i < WFD_SINK_V_NUM; i++) 	/* NOTE : skip bin */
	{
		if (videobin[i].gst)
		{
			GstObject* parent = NULL;
			parent = gst_element_get_parent (videobin[i].gst);

			if (!parent)
			{
				gst_object_unref(GST_OBJECT(videobin[i].gst));
				videobin[i].gst = NULL;
			}
			else
			{
				gst_object_unref(GST_OBJECT(parent));
			}
		}
	}

	/* release videobin with it's childs */
	if (videobin[WFD_SINK_V_BIN].gst )
	{
		gst_object_unref (GST_OBJECT(videobin[WFD_SINK_V_BIN].gst));
	}

	MMWFDSINK_FREEIF (videobin);

	return MM_ERROR_WFD_INTERNAL;
}

static int __mm_wfd_sink_destroy_videobin(mm_wfd_sink_t* wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement* videobin = NULL;
	GstObject* parent = NULL;
	int i;

	debug_fenter ();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED );

	if (wfd_sink->pipeline && wfd_sink->pipeline->videobin && wfd_sink->pipeline->videobin[WFD_SINK_V_BIN].gst)
	{
		videobin = wfd_sink->pipeline->videobin;
	}
	else
	{
		debug_error("No-error : videobin is not created, nothing to destroy\n");
		return MM_ERROR_NONE;
	}


	parent = gst_element_get_parent (videobin[WFD_SINK_V_BIN].gst);
	if (!parent)
	{
		debug_error("No-error : videobin has no parent.. need to relaset by itself\n");

		if (wfd_sink->video_bin_is_prepared)
		{
			debug_error("No-error : try to change state of videobin to NULL\n");
			ret = gst_element_set_state (videobin[WFD_SINK_V_BIN].gst, GST_STATE_NULL);
			if ( ret != GST_STATE_CHANGE_SUCCESS )
			{
				debug_error("failed to change state of videobin to NULL\n");
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
					gst_object_unref(GST_OBJECT(videobin[i].gst));
					videobin[i].gst = NULL;
				}
				else
				{
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
		debug_error("No-error : videobin has parent\n");

		gst_object_unref (GST_OBJECT(parent));
	}

	debug_fleave();

	return MM_ERROR_NONE;
}

static int  __mm_wfd_sink_destroy_audiobin(mm_wfd_sink_t* wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	MMWFDSinkGstElement* audiobin = NULL;
	GstObject* parent = NULL;
	int i;

	debug_fenter ();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	if (wfd_sink->pipeline && wfd_sink->pipeline->audiobin && wfd_sink->pipeline->audiobin[WFD_SINK_A_BIN].gst)
	{
		audiobin = wfd_sink->pipeline->audiobin;
	}
	else
	{
		debug_error("No-error : audiobin is not created, nothing to destroy\n");
		return MM_ERROR_NONE;
	}


	parent = gst_element_get_parent (audiobin[WFD_SINK_A_BIN].gst);
	if (!parent)
	{
		debug_error("No-error : audiobin has no parent.. need to relaset by itself\n");

		if (wfd_sink->audio_bin_is_prepared)
		{
			debug_error("No-error : try to change state of audiobin to NULL\n");
			ret = gst_element_set_state (audiobin[WFD_SINK_A_BIN].gst, GST_STATE_NULL);
			if ( ret != GST_STATE_CHANGE_SUCCESS )
			{
				debug_error("failed to change state of audiobin to NULL\n");
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
					gst_object_unref(GST_OBJECT(audiobin[i].gst));
					audiobin[i].gst = NULL;
				}
				else
				{
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
		debug_error("No-error : audiobin has parent\n");

		gst_object_unref (GST_OBJECT(parent));
	}

	debug_fleave();

	return MM_ERROR_NONE;
}

static int __mm_wfd_sink_destroy_pipeline(mm_wfd_sink_t* wfd_sink)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

	debug_fenter ();

	return_val_if_fail (wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED );

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
				debug_error("failed to destroy videobin\n");
				return MM_ERROR_WFD_INTERNAL;
			}

			if (MM_ERROR_NONE != __mm_wfd_sink_destroy_audiobin(wfd_sink))
			{
				debug_error("failed to destroy audiobin\n");
				return MM_ERROR_WFD_INTERNAL;
			}

			ret = gst_element_set_state (mainbin[WFD_SINK_M_PIPE].gst, GST_STATE_NULL);
			if ( ret != GST_STATE_CHANGE_SUCCESS )
			{
				debug_error("failed to change state of mainbin to NULL\n");
				return MM_ERROR_WFD_INTERNAL;
			}

			gst_object_unref(GST_OBJECT(mainbin[WFD_SINK_M_PIPE].gst));

			MMWFDSINK_FREEIF( audiobin );
			MMWFDSINK_FREEIF( videobin );
			MMWFDSINK_FREEIF( mainbin );
		}

		MMWFDSINK_FREEIF( wfd_sink->pipeline );
	}

	wfd_sink->is_constructed = FALSE;
	wfd_sink->added_av_pad_num = 0;
	wfd_sink->need_to_reset_basetime = FALSE;
	wfd_sink->demux_video_pad_probe_id = 0;
	wfd_sink->demux_audio_pad_probe_id = 0;
	wfd_sink->audio_bin_is_linked = FALSE;
	wfd_sink->video_bin_is_linked = FALSE;
	wfd_sink->audio_bin_is_prepared = FALSE;
	wfd_sink->video_bin_is_prepared = FALSE;

	debug_fleave();

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

	debug_fenter();

	return_if_fail ( wfd_sink &&
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
						 debug_error("%s:%s : From:%s To:%s refcount : %d\n", GST_OBJECT_NAME(factory) , GST_ELEMENT_NAME(item) ,
							gst_element_state_get_name(state), gst_element_state_get_name(pending) , GST_OBJECT_REFCOUNT_VALUE(item));
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
		debug_error("%s:%s : From:%s To:%srefcount : %d\n",
			GST_OBJECT_NAME(factory),
			GST_ELEMENT_NAME(item),
			gst_element_state_get_name(state),
			gst_element_state_get_name(pending),
			GST_OBJECT_REFCOUNT_VALUE(item) );
	}

	if ( iter )
		gst_iterator_free (iter);

	debug_fleave();

	return;
}

const gchar *
__mm_wfds_sink_get_state_name ( MMWfdSinkStateType state )
{
	switch ( state )
	{
		case MM_WFD_SINK_STATE_NULL:
			return "NULL";
		case MM_WFD_SINK_STATE_READY:
			return "READY";
		case MM_WFD_SINK_STATE_PAUSED:
			return "PAUSED";
		case MM_WFD_SINK_STATE_PLAYING:
			return "PLAYING";
		case MM_WFD_SINK_STATE_NONE:
			return "NONE";
		case MM_WFD_SINK_STATE_TEARDOWN:
			return "TEARDOWN";
		default:
			return "INVAID";
	}
}
