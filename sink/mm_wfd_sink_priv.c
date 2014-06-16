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
#include <stdlib.h>
#include <gst/gst.h>
#include <mm_debug.h>
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_util.h"

int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink, const char *uri)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	return_val_if_fail(uri, MM_ERROR_WFD_NOT_INITIALIZED);

	mm_wfd_sink_t *new_wfd_sink = NULL;

	new_wfd_sink = g_malloc0(sizeof(mm_wfd_sink_t));
	if (!new_wfd_sink) {
		debug_critical("Cannot allocate memory for wfdsink\n");
		goto ERROR;
	}

	/* Initialize all states */
	new_wfd_sink->state = MM_WFD_SINK_STATE_NULL;
	new_wfd_sink->prev_state = MM_WFD_SINK_STATE_NULL;
	new_wfd_sink->pending_state = MM_WFD_SINK_STATE_NULL;
	new_wfd_sink->target_state = MM_WFD_SINK_STATE_NULL;

	result = _mm_wfd_sink_gstreamer_init(new_wfd_sink);
	if (result < MM_ERROR_NONE) {
		debug_critical("_mm_wfd_sink_gstreamer_init failed : %d", result);
	}

	result = _mm_wfd_sink_create_gst_pipeline(new_wfd_sink, uri);
	if (result < MM_ERROR_NONE) {
		debug_critical("_mm_wfd_sink_gstreamer_init failed : %d", result);
	}

	*wfd_sink = new_wfd_sink;
	return result;
ERROR:
	SAFE_FREE(new_wfd_sink);
	return result;
}

int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	gst_element_set_state(wfd_sink->sink_bin, GST_STATE_PLAYING);
	return result;
}

int _mm_wfd_sink_stop(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	return result;
}

int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	return result;
}

int _mm_wfd_sink_gstreamer_init(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	gint* argc = NULL;
	gchar** argv = NULL;
	static const int max_argc = 50;
	GError *err = NULL;
	int i = 0;

	/* alloc */
	argc = calloc(1, sizeof(int) );
	argv = calloc(max_argc, sizeof(gchar*));
	if (!argc || !argv) {
		debug_critical("Cannot allocate memory for wfdsink\n");
		goto ERROR;
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

	debug_log("initializing gstreamer with following parameter\n");
	debug_log("argc : %d\n", *argc);

	for ( i = 0; i < *argc; i++ ) {
		debug_log("argv[%d] : %s\n", i, argv[i]);
	}

	/* initializing gstreamer */
	if ( ! gst_init_check (argc, &argv, &err)) {
		debug_error("Could not initialize GStreamer: %s\n", err ? err->message : "unknown error occurred");
		if (err) {
			g_error_free (err);
		}

		goto ERROR;
	}

	/* release */
	for ( i = 0; i < *argc; i++ ) {
		SAFE_FREE( argv[i] );
	}

	SAFE_FREE(argv);
	SAFE_FREE(argc);

	return result;
ERROR:
	/* release */
	for ( i = 0; i < *argc; i++ ) {
		SAFE_FREE( argv[i] );
	}
	SAFE_FREE(argv);
	SAFE_FREE(argc);

	return FALSE;
}

static void
wfd_pad_added (GstElement* ele, GstPad* pad, gpointer data)
{
  mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
  gchar* name = gst_pad_get_name (pad);

  GstCaps *caps = NULL;
  GstPad* spad = NULL;

  debug_log(" =========== >>>>>>>>>> Received wfd pad...\n");

  if (!wfd_sink) {
    debug_critical("wfd sink is NULL...\n");
    return;
  }

  if (wfd_sink->depay == NULL) {
    debug_critical("depay element is NULL...\n");
    return;
  }

    caps = gst_pad_query_caps( pad, NULL );
    if (!caps) {
      debug_critical("caps is NULL...\n");
    } else {
        gst_caps_unref(caps);
    }

  spad = gst_element_get_static_pad(wfd_sink->depay, "sink");
  if (!spad) {
    debug_critical("gst_element_get_pad failed ...\n");
    return;
  } else {
    debug_critical("Success...\n");
  }

  if (gst_pad_link(pad, spad)  != GST_PAD_LINK_OK) {
    debug_critical("Failed to link wfdrtspsrc pad & depay sink pad...\n");
    return;
  } else {
    debug_critical("Success to link wfdrtspsrc pad & depay sink pad...\n");
  }
  gst_object_unref(spad);

  GstPad *srcpad = NULL;
  srcpad = gst_element_get_static_pad(wfd_sink->depay, "src");
  if (!srcpad) {
    debug_critical("gst_element_get_pad failed ...\n");
    return;
  } else {
    debug_critical("Success...\n");
  }

  spad = gst_element_get_static_pad(wfd_sink->demux, "sink");
  if (!spad) {
    debug_critical("gst_element_get_pad failed ...\n");
    return;
  } else {
    debug_critical("Success...\n");
  }
  if (gst_pad_link(srcpad, spad)  != GST_PAD_LINK_OK) {
    debug_critical("Failed to link depay src pad & demux sink pad...\n");
    return;
  } else {
    debug_critical("Success to link depay src pad & demux sink pad...\n");
  }
  gst_object_unref(spad);
  gst_object_unref(srcpad);

  gst_element_set_state(wfd_sink->sink_bin, GST_STATE_PLAYING);
  g_free (name);

  debug_log("================ dump pipeline");
  _mm_wfd_sink_dump_pipeline_state(wfd_sink);
}

static void
pad_added (GstElement* ele, GstPad* pad, gpointer data)
{
  mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
  gchar* name = gst_pad_get_name (pad);

  if (name[0] == 'v') {
    GstPad* spad = NULL;

    debug_log(" =========== >>>>>>>>>> Received VIDEO pad...\n");
	_mm_wfd_sink_create_gst_video_pipeline(wfd_sink);

    spad = gst_element_get_static_pad(GST_ELEMENT(wfd_sink->v_queue), "sink");
    if (gst_pad_link(pad, spad)  != GST_PAD_LINK_OK) {
      debug_critical("Failed to link video demuxer pad & video parser sink pad...\n");
      return;
    }
    gst_object_unref(spad);


  } else if (name[0] == 'a') {
    GstPad* spad = NULL;

    debug_log(" =========== >>>>>>>>>> Received AUDIO pad...\n");

	_mm_wfd_sink_create_gst_audio_pipeline(wfd_sink);

    spad = gst_element_get_static_pad(GST_ELEMENT(wfd_sink->a_queue), "sink");
    if (gst_pad_link(pad, spad)  != GST_PAD_LINK_OK) {
      debug_critical ("Failed to link audio demuxer pad & audio parse pad...\n");
      return;
    }
    gst_object_unref(spad);

	GstElement* parent = NULL;
	parent = (GstElement*)gst_object_get_parent((GstObject *)wfd_sink->fakesink);
	if (parent) {
		debug_log("Removing fakesink");

	gst_element_set_locked_state( wfd_sink->fakesink, TRUE );

	/* setting the state to NULL never returns async
	 * so no need to wait for completion of state transiton
	 */
	if ( GST_STATE_CHANGE_FAILURE == gst_element_set_state (wfd_sink->fakesink, GST_STATE_NULL) )
	{
		debug_error("fakesink state change failure!\n");

		/* FIXIT : should I return here? or try to proceed to next? */
		/* return FALSE; */
	}

	/* remove fakesink from it's parent */
	if ( ! gst_bin_remove( GST_BIN(parent), wfd_sink->fakesink ) )
	{
		debug_error("failed to remove fakesink\n");
		gst_object_unref( parent );
		return;
	}

	gst_object_unref( parent );

	debug_log("state-holder removed\n");
	gst_element_set_locked_state( wfd_sink->fakesink, FALSE );
	}
  } else {
    debug_critical("not handling.....\n\n\n");
  }

  gst_element_set_state(GST_ELEMENT(wfd_sink->sink_bin), GST_STATE_PLAYING);
  g_free (name);

  debug_log("end of pad_added");
  debug_log("================ dump pipeline");
  _mm_wfd_sink_dump_pipeline_state(wfd_sink);
}

static GstBusSyncReply
_mm_wfd_bus_sync_callback (GstBus * bus, GstMessage * message, gpointer data)
{
	mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
	gchar* message_structure_name;
	gchar** splited_message;
	const GstStructure* message_structure = gst_message_get_structure(message);
	switch (GST_MESSAGE_TYPE (message))
	{
		case GST_MESSAGE_TAG:
			break;
		case GST_MESSAGE_DURATION:
			break;
    case GST_MESSAGE_APPLICATION:
      message_structure_name = gst_structure_get_name(message_structure);
      splited_message = g_strsplit((gchar*)message_structure_name, "_", 2);
      if(g_strrstr(message_structure_name, "audiotype"))
      {
        wfd_sink->audio_type = splited_message[1];
        g_print("USING %s AUDIO FORMAT\n", wfd_sink->audio_type);
      } else
      if(g_strrstr(message_structure_name, "volume"))
      {
        gdouble volume_param = g_ascii_strtod(splited_message[1], NULL);
        g_object_set (wfd_sink->volume, "volume", volume_param/1.5, NULL);
      }

      break;

		case GST_MESSAGE_STATE_CHANGED:
			/* we only handle messages from pipeline */
			if( message->src != (GstObject *)wfd_sink->sink_bin )
				return GST_BUS_DROP;
		default:
			return GST_BUS_DROP;
	}

	gst_message_unref (message);

	return GST_BUS_DROP;
}

static gboolean
_mm_wfd_sink_msg_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
	mm_wfd_sink_t* wfd_sink = (mm_wfd_sink_t*) data;
	gboolean ret = TRUE;

	return_val_if_fail ( wfd_sink, FALSE );
	return_val_if_fail ( msg && GST_IS_MESSAGE(msg), FALSE );

	switch ( GST_MESSAGE_TYPE( msg ) )
	{
		case GST_MESSAGE_UNKNOWN:
			debug_warning("unknown message received\n");
		break;

		case GST_MESSAGE_EOS:
		{
			debug_log("GST_MESSAGE_EOS received\n");

		}
		break;

		case GST_MESSAGE_ERROR:
		{
			GError *error = NULL;
			gchar* debug = NULL;
			gchar *msg_src_element = NULL;

			/* get error code */
			gst_message_parse_error( msg, &error, &debug );

			msg_src_element = GST_ELEMENT_NAME( GST_ELEMENT_CAST( msg->src ) );
			if ( gst_structure_has_name ( gst_message_get_structure(msg), "streaming_error" ) )
			{
			}
			else
			{
			}

			SAFE_FREE( debug );
			g_error_free( error );
		}
		break;

		case GST_MESSAGE_WARNING:
		{
			char* debug = NULL;
			GError* error = NULL;

			gst_message_parse_warning(msg, &error, &debug);

			debug_warning("warning : %s\n", error->message);
			debug_warning("debug : %s\n", debug);

			SAFE_FREE( debug );
			g_error_free( error );
		}
		break;

		case GST_MESSAGE_INFO:
			debug_log("GST_MESSAGE_STATE_DIRTY\n"); break;

		case GST_MESSAGE_TAG:
		{
			debug_log("GST_MESSAGE_TAG\n");
		}
		break;

		case GST_MESSAGE_BUFFERING:
		{
		}
		break;

		case GST_MESSAGE_STATE_CHANGED:
		{
			const GValue *voldstate, *vnewstate, *vpending;
			GstState oldstate, newstate, pending;
			const GstStructure *msg_struct = NULL;

			/* we only handle messages from pipeline */
			if( msg->src != (GstObject *)wfd_sink->sink_bin )
				break;

			/* get state info from msg */
			msg_struct = gst_message_get_structure(msg);
			voldstate = gst_structure_get_value (msg_struct, "old-state");
			vnewstate = gst_structure_get_value (msg_struct, "new-state");
			vpending = gst_structure_get_value (msg_struct, "pending-state");

			oldstate = (GstState)voldstate->data[0].v_int;
			newstate = (GstState)vnewstate->data[0].v_int;
			pending = (GstState)vpending->data[0].v_int;

			debug_log("state changed [%s] : %s ---> %s     final : %s\n",
				GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)),
				gst_element_state_get_name( (GstState)oldstate ),
				gst_element_state_get_name( (GstState)newstate ),
				gst_element_state_get_name( (GstState)pending ) );

			if (oldstate == newstate)
			{
				debug_warning("pipeline reports state transition to old state");
				break;
			}

			switch(newstate)
			{
				case GST_STATE_VOID_PENDING:
				break;

				case GST_STATE_NULL:
				break;

				case GST_STATE_READY:
				break;

				case GST_STATE_PAUSED:
				break;

				case GST_STATE_PLAYING:
				{
				}
				break;

				default:
				break;
			}
		}
		break;

		case GST_MESSAGE_STATE_DIRTY:
			debug_log("GST_MESSAGE_STATE_DIRTY\n"); break;
		case GST_MESSAGE_STEP_DONE:
			debug_log("GST_MESSAGE_STEP_DONE\n"); break;
		case GST_MESSAGE_CLOCK_PROVIDE:
			debug_log("GST_MESSAGE_CLOCK_PROVIDE\n"); break;

		case GST_MESSAGE_CLOCK_LOST:
			{
				GstClock *clock = NULL;
				gst_message_parse_clock_lost (msg, &clock);
				debug_log("GST_MESSAGE_CLOCK_LOST : %s\n", (clock ? GST_OBJECT_NAME (clock) : "NULL"));
				g_print ("GST_MESSAGE_CLOCK_LOST : %s\n", (clock ? GST_OBJECT_NAME (clock) : "NULL"));

			}
			break;

		case GST_MESSAGE_NEW_CLOCK:
			{
				GstClock *clock = NULL;
				gst_message_parse_new_clock (msg, &clock);
				debug_log("GST_MESSAGE_NEW_CLOCK : %s\n", (clock ? GST_OBJECT_NAME (clock) : "NULL"));
			}
			break;

		case GST_MESSAGE_STRUCTURE_CHANGE:	debug_log("GST_MESSAGE_STRUCTURE_CHANGE\n"); break;
		case GST_MESSAGE_STREAM_STATUS:	debug_log("GST_MESSAGE_STREAM_STATUS\n"); break;

		case GST_MESSAGE_APPLICATION:		debug_log("GST_MESSAGE_APPLICATION\n");
											abort();
											break;


		case GST_MESSAGE_ELEMENT:
			{
				const gchar *structure_name = NULL;
				structure_name = gst_structure_get_name(gst_message_get_structure(msg));
				if (structure_name) debug_log("GST_MESSAGE_ELEMENT : %s\n", structure_name);
			}
			break;

		case GST_MESSAGE_SEGMENT_START:		debug_log("GST_MESSAGE_SEGMENT_START\n"); break;
		case GST_MESSAGE_SEGMENT_DONE:		debug_log("GST_MESSAGE_SEGMENT_DONE\n"); break;

		case GST_MESSAGE_DURATION:
		{
			debug_log("GST_MESSAGE_DURATION\n");
		}

		break;

		case GST_MESSAGE_LATENCY:				debug_log("GST_MESSAGE_LATENCY\n"); break;
		case GST_MESSAGE_ASYNC_START:		debug_log("GST_MESSAGE_ASYNC_START : %s\n", gst_element_get_name(GST_MESSAGE_SRC(msg))); break;

		case GST_MESSAGE_ASYNC_DONE:
		{
			debug_log("GST_MESSAGE_ASYNC_DONE");

		}
		break;

		case GST_MESSAGE_REQUEST_STATE:		debug_log("GST_MESSAGE_REQUEST_STATE\n"); break;
		case GST_MESSAGE_STEP_START:		debug_log("GST_MESSAGE_STEP_START\n"); break;
		case GST_MESSAGE_QOS:					debug_log("GST_MESSAGE_QOS\n"); break;
		case GST_MESSAGE_PROGRESS:			debug_log("GST_MESSAGE_PROGRESS\n"); break;
		case GST_MESSAGE_ANY:				debug_log("GST_MESSAGE_ANY\n"); break;

		default:
			debug_warning("unhandled message\n");
		break;
	}

	/* FIXIT : this cause so many warnings/errors from glib/gstreamer. we should not call it since
 * 	 * gst_element_post_message api takes ownership of the message.
 * 	 	 */
	//gst_message_unref( msg );

	return ret;
}

int _mm_wfd_sink_create_gst_pipeline(mm_wfd_sink_t *wfd_sink, const char *uri)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	GstBus	*bus = NULL;
	GstElement *sink_bin = NULL;
	GstElement *wfd_element = NULL;
	GstElement *depay_element = NULL;
	GstElement *demuxer_element = NULL;
	GstElement *fake_element = NULL;

	debug_fenter();

	sink_bin =gst_pipeline_new("wfdsink_bin");
	if (!sink_bin) {
		debug_critical("failed to create wfd sink bin...");
		goto CREATE_ERROR;
	}

	wfd_sink->sink_bin = sink_bin;

	wfd_element = gst_element_factory_make("wfdrtspsrc", "wfd_rtsp_source");
	if (!wfd_element)	{
		debug_critical("failed to create wfd streaming source element\n");
		goto CREATE_ERROR;
	}

	g_object_set(G_OBJECT(wfd_element), "location", uri, NULL);
	g_object_set(G_OBJECT(wfd_element), "debug", TRUE, NULL);
	g_object_set(G_OBJECT(wfd_element), "latency", 0, NULL);

	if (!wfd_element) {
		debug_critical("no source element was created.\n");
		goto CREATE_ERROR;
	}
	g_signal_connect (wfd_element, "pad-added", G_CALLBACK(wfd_pad_added), wfd_sink);
    gst_element_set_state(wfd_element, GST_STATE_READY);

	wfd_sink->wfd = wfd_element;

    depay_element = gst_element_factory_make("rtpmp2tdepay", "wfd_rtp_depay");
    if (!depay_element) {
      debug_critical ( "failed to create wfd rtp depay element\n" );
      goto CREATE_ERROR;
    }
    gst_element_set_state(depay_element, GST_STATE_READY);

	wfd_sink->depay = depay_element;

    demuxer_element = gst_element_factory_make("mpegtsdemux", "mpegts_demuxer");
    if (!demuxer_element) {
      debug_critical ( "failed to create mpegtsdemux element\n" );
      goto CREATE_ERROR;
    }

	g_signal_connect (demuxer_element, "pad-added", G_CALLBACK(pad_added), wfd_sink);
    gst_element_set_state(demuxer_element, GST_STATE_READY);

	wfd_sink->demux = demuxer_element;

	gst_bin_add_many(GST_BIN(sink_bin), wfd_element, depay_element, demuxer_element, NULL);

    fake_element = gst_element_factory_make("fakesink", "fake_sink");
    if (!depay_element) {
      debug_critical ( "failed to create wfd rtp depay element\n" );
      goto CREATE_ERROR;
    }
	gst_bin_add(GST_BIN(sink_bin), fake_element);

	wfd_sink->fakesink = fake_element;

	/* connect bus callback */
	bus = gst_pipeline_get_bus(GST_PIPELINE(sink_bin));
	if (!bus) {
		debug_error ("cannot get bus from pipeline.\n");
		goto CREATE_ERROR;
	}

	gst_bus_add_watch(bus, (GstBusFunc)_mm_wfd_sink_msg_callback, wfd_sink);

	/* set sync handler to get tag synchronously */
	gst_bus_set_sync_handler(bus, _mm_wfd_bus_sync_callback, wfd_sink, NULL);

	debug_log("End of _mm_wfd_sink_create_gst_pipeline");

	return result;

CREATE_ERROR:

	return result;
}

int _mm_wfd_sink_create_gst_audio_pipeline(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	GstElement *a_queue_element = NULL;
	GstElement *a_parse_element = NULL;
	GstElement *a_dec_element = NULL;
	GstElement *a_resample_element = NULL;
	GstElement *volume = NULL;
	GstElement *a_sink_element = NULL;

	a_queue_element = gst_element_factory_make("queue", "audio_queue");
	if (!a_queue_element) {
		debug_critical ( "failed to create audio_queue element\n" );
		goto CREATE_ERROR;
	}

	volume = gst_element_factory_make("volume", "volume");
	if (!volume) {
		debug_critical ( "failed to create volume element\n" );
		goto CREATE_ERROR;
	}

	a_sink_element = gst_element_factory_make("avsysaudiosink", "audio_sink");
	if (!a_sink_element) {
		debug_critical ( "failed to create audio_sink element\n" );
		goto CREATE_ERROR;
	}

	wfd_sink->a_queue = a_queue_element;
	wfd_sink->volume = volume;
	wfd_sink->a_sink = a_sink_element;

	if (g_strrstr(wfd_sink->audio_type,"AAC")) {

		a_parse_element = gst_element_factory_make("aacparse", "audio_parser");
		if (!a_parse_element) {
			debug_critical ( "failed to create audio_parser element\n" );
			goto CREATE_ERROR;
		}

		a_dec_element = gst_element_factory_make("savsdec_aac", "audio_dec");
		if (!a_dec_element) {
			debug_critical ( "failed to create audio_dec element\n" );
			goto CREATE_ERROR;
		}

		a_resample_element = gst_element_factory_make("audioresample", "audio_conv");
		if (!a_resample_element) {
			debug_critical ( "failed to create audio_conv element\n" );
			goto CREATE_ERROR;
		}

		wfd_sink->a_parse = a_parse_element;
		wfd_sink->a_dec = a_dec_element;


		gst_bin_add_many(GST_BIN(wfd_sink->sink_bin), a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL);
		if (!gst_element_link_many(a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL)) {
			debug_critical("Failed to link audio src elements...");
			goto CREATE_ERROR;
		}
	}
	else if (g_strrstr(wfd_sink->audio_type,"AC3")) {

		a_parse_element = gst_element_factory_make("ac3parse", "audio_parser");
		if (!a_parse_element) {
			debug_critical ( "failed to create audio_parser element\n" );
			goto CREATE_ERROR;
		}

		a_dec_element = gst_element_factory_make("savsdec_ac3", "audio_dec");
		if (!a_dec_element) {
			debug_critical ( "failed to create audio_dec element\n" );
			goto CREATE_ERROR;
		}

		a_resample_element = gst_element_factory_make("audioresample", "audio_conv");
		if (!a_resample_element) {
			debug_critical ( "failed to create audio_conv element\n" );
			goto CREATE_ERROR;
		}

		wfd_sink->a_parse = a_parse_element;
		wfd_sink->a_dec = a_dec_element;

		gst_bin_add_many(GST_BIN(wfd_sink->sink_bin), a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL);
		if (!gst_element_link_many(a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL)) {
			debug_critical("Failed to link audio src elements...");
			goto CREATE_ERROR;
		}
	}
	else if (g_strrstr(wfd_sink->audio_type,"LPCM")) {

		a_parse_element = gst_element_factory_make("wavparse", "audio_parser");
		if (!a_parse_element) {
			debug_critical ( "failed to create audio_parser element\n" );
			goto CREATE_ERROR;
		}

		wfd_sink->a_parse = a_parse_element;

		gst_bin_add_many(GST_BIN(wfd_sink->sink_bin), a_queue_element, a_parse_element, a_resample_element, volume, a_sink_element, NULL);
		if (!gst_element_link_many(a_queue_element, a_parse_element,a_resample_element, volume, a_sink_element, NULL)) {
			debug_critical("Failed to link audio src elements...");
			goto CREATE_ERROR;
		}
	}

	debug_log("End of _mm_wfd_sink_create_gst_audio_pipeline");

	return result;
CREATE_ERROR:

	return result;
}

int _mm_wfd_sink_create_gst_video_pipeline(mm_wfd_sink_t *wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	GstElement *v_queue_element = NULL;
	GstElement *v_parse_element = NULL;
	GstElement *v_dec_element = NULL;
	GstElement *v_sink_element = NULL;

	v_queue_element = gst_element_factory_make("queue", "video_queue");
	if (!v_queue_element) {
      debug_critical ( "failed to create video_queue element\n" );
		goto CREATE_ERROR;
	}

    v_parse_element = gst_element_factory_make("h264parse", "video_parser");
    if (!v_parse_element) {
      debug_critical ( "failed to create video_parser element\n" );
      goto CREATE_ERROR;
    }

    v_dec_element = gst_element_factory_make("omxh264dec", "video_dec");
    if (!v_dec_element) {
      debug_critical ( "failed to create video_dec element\n" );
      goto CREATE_ERROR;
    }

    v_sink_element = gst_element_factory_make("xvimagesink", "video_sink");
    if (!v_sink_element) {
      debug_critical ( "failed to create video_sink element\n" );
      goto CREATE_ERROR;
    }

	wfd_sink->v_queue = v_queue_element;

	gst_bin_add_many(GST_BIN(wfd_sink->sink_bin), v_queue_element, v_parse_element, v_dec_element, v_sink_element, NULL);
	if (!gst_element_link_many(v_queue_element, v_parse_element, v_dec_element, v_sink_element, NULL)) {
		debug_critical("Failed to link video src elements...");
		goto CREATE_ERROR;
	}

	debug_log("End of _mm_wfd_sink_create_gst_video_pipeline");
	return result;

CREATE_ERROR:

	return result;
}

