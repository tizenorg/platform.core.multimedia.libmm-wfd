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
#include <mm_debug.h>
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_util.h"

int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  mm_wfd_sink_t *new_wfd_sink = NULL;

  new_wfd_sink = g_malloc0(sizeof(mm_wfd_sink_t));
  if (!new_wfd_sink) {
    debug_error("Cannot allocate memory for wfdsink\n");
    goto ERROR;
  }
  new_wfd_sink->attrs = _mmwfd_construct_attribute((MMHandleType)new_wfd_sink);
  /* Initialize all states */
  new_wfd_sink->state = MM_WFD_SINK_STATE_NULL;
  new_wfd_sink->prev_state = MM_WFD_SINK_STATE_NULL;
  new_wfd_sink->pending_state = MM_WFD_SINK_STATE_NULL;
  new_wfd_sink->target_state = MM_WFD_SINK_STATE_NULL;

  /* initialize audio/video pad parameter */
  new_wfd_sink->AV_mode = AV_MODE_AUDIO_AND_VIDEO;
  new_wfd_sink->added_av_pad_num = 0;

  result = _mm_wfd_sink_gstreamer_init(new_wfd_sink);
  if (result < MM_ERROR_NONE) {
    debug_error("_mm_wfd_sink_gstreamer_init failed : %d", result);
  }

  *wfd_sink = new_wfd_sink;
  return result;
  ERROR:
  SAFE_FREE(new_wfd_sink);
  return result;
}

int _mm_wfd_sink_connect(mm_wfd_sink_t *wfd_sink, const char *uri)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
  return_val_if_fail(uri, MM_ERROR_WFD_NOT_INITIALIZED);

  result = _mm_wfd_sink_create_gst_pipeline(wfd_sink, uri);
  if (result < MM_ERROR_NONE) {
    debug_error("_mm_wfd_sink_create_gst_pipeline failed : %d", result);
  }

  return result;
}

int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  gst_element_set_state(wfd_sink->sink_pipeline, GST_STATE_PLAYING);
  return result;
}

int _mm_wfd_sink_stop(mm_wfd_sink_t *wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  gst_element_set_state(wfd_sink->sink_pipeline, GST_STATE_READY);
  return result;
}

int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  gst_element_set_state(wfd_sink->sink_pipeline, GST_STATE_NULL);

  /* release attributes */
  _mmwfd_deconstruct_attribute(wfd_sink->attrs);
  return result;
}

int _mm_wfd_set_message_callback(mm_wfd_sink_t *wfd_sink, MMWFDMessageCallback callback, void *user_data)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  wfd_sink->msg_cb = callback;
  wfd_sink->msg_user_data = user_data;

  return result;
}

void __mm_wfd_post_message(mm_wfd_sink_t* wfd_sink, MMWfdSinkStateType msgtype)
{
  if (wfd_sink->msg_cb) {
    debug_log("Message (type : %d) will be posted using user callback", msgtype);
    wfd_sink->msg_cb(msgtype, wfd_sink->msg_user_data);
  }
}


int _mm_wfd_sink_gstreamer_init(mm_wfd_sink_t *wfd_sink)
{
  int result = MM_ERROR_NONE;

  gint* argc = NULL;
  gchar** argv = NULL;
  static const int max_argc = 50;
  GError *err = NULL;
  int i = 0;

  /* alloc */
  argc = calloc(1, sizeof(int) );
  argv = calloc(max_argc, sizeof(gchar*));
  if (!argc || !argv) {
    debug_error("Cannot allocate memory for wfdsink\n");
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

  if (wfd_sink->added_av_pad_num == 2) {
    debug_log("Another pad added!");
    GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (wfd_sink->sink_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "wfdsink");
  }

  if (!wfd_sink) {
    debug_error("wfd sink is NULL...\n");
    return;
  }

  if (wfd_sink->depay == NULL) {
    debug_error("depay element is NULL...\n");
    return;
  }

    caps = gst_pad_get_caps( pad );
    if (!caps) {
      debug_error("caps is NULL...\n");
    }

  spad = gst_element_get_static_pad(wfd_sink->depay, "sink");
  if (!spad) {
    debug_error("gst_element_get_pad failed ...\n");
    return;
  } else {
    debug_log("Success...\n");
  }

  if (gst_pad_is_linked(spad)) {
    debug_log("Unlink start!");
    if (!gst_pad_unlink(wfd_sink->rtp_pad_src, spad)) {
      debug_error("gst_pad_unlink failed...\n");
    } else {
      debug_log("Success...\n");
    }

  if (gst_pad_link(pad, spad)  != GST_PAD_LINK_OK) {
    debug_error("Failed to link wfdrtspsrc pad & depay sink pad...\n");
    return;
  } else {
    debug_log("Success to link wfdrtspsrc pad & depay sink pad...\n");
    wfd_sink->rtp_pad_src = pad;
  }
  gst_object_unref(spad);

    goto done;
  }

  if (gst_pad_link(pad, spad)  != GST_PAD_LINK_OK) {
    debug_error("Failed to link wfdrtspsrc pad & depay sink pad...\n");
    return;
  } else {
    debug_log("Success to link wfdrtspsrc pad & depay sink pad...\n");
    wfd_sink->rtp_pad_src = pad;
  }
  gst_object_unref(spad);

  GstPad *srcpad = NULL;
  srcpad = gst_element_get_static_pad(wfd_sink->depay, "src");
  if (!srcpad) {
    debug_error("gst_element_get_pad failed ...\n");
    return;
  } else {
    debug_log("Success...\n");
  }

  spad = gst_element_get_static_pad(wfd_sink->demux, "sink");
  if (!spad) {
    debug_error("gst_element_get_pad failed ...\n");
    return;
  } else {
    debug_log("Success...\n");
  }
  if (gst_pad_link(srcpad, spad)  != GST_PAD_LINK_OK) {
    debug_error("Failed to link depay src pad & demux sink pad...\n");
    return;
  } else {
    debug_log("Success to link depay src pad & demux sink pad...\n");
  }

  gst_object_unref(spad);
  gst_object_unref(srcpad);

done:
  g_free (name);

  //debug_log("================ dump pipeline");
  //_mm_wfd_sink_dump_pipeline_state(wfd_sink);
}

static void __remove_state_holder(gpointer data)
{
  mm_wfd_sink_t *wfd_sink = (mm_wfd_sink_t *)data;
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
      gst_element_set_locked_state( wfd_sink->fakesink, FALSE );
      return;
    }

    gst_object_unref( parent );

    debug_log("state-holder removed\n");
    gst_element_set_locked_state( wfd_sink->fakesink, FALSE );
  } else {
    debug_error("parent is NULL");
  }
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

    spad = gst_element_get_static_pad(GST_ELEMENT(wfd_sink->sink_vbin), "sink");
    if (spad == NULL) {
      debug_error("Failed to get pad in video bin\n");
      return;
    }

    if (gst_pad_link(pad, spad) != GST_PAD_LINK_OK) {
      debug_error("Failed to link video demuxer pad & video bin ...\n");
      gst_object_unref(spad);
      return;
    }
    gst_object_unref(spad);

    wfd_sink->added_av_pad_num++;
    if(wfd_sink->AV_mode == wfd_sink->added_av_pad_num){
      __remove_state_holder(data);
    }
  } else if (name[0] == 'a') {
    GstPad* spad = NULL;

    debug_log(" =========== >>>>>>>>>> Received AUDIO pad...\n");

    _mm_wfd_sink_create_gst_audio_pipeline(wfd_sink);

    spad = gst_element_get_static_pad(GST_ELEMENT(wfd_sink->sink_abin), "sink");
    if (spad == NULL) {
      debug_error("Failed to get pad in audio bin\n");
      return;
    }

    if (gst_pad_link(pad, spad) != GST_PAD_LINK_OK) {
      debug_error("Failed to link audio demuxer pad & audio bin ...\n");
      gst_object_unref(spad);
      return;
    }
    gst_object_unref(spad);

    wfd_sink->added_av_pad_num++;
    if(wfd_sink->AV_mode == wfd_sink->added_av_pad_num)
    {
      __remove_state_holder(data);
    }
  } else {
    debug_error("not handling.....\n\n\n");
  }

  g_free (name);

  debug_log("end of pad_added");
  //debug_log("================ dump pipeline");
  //_mm_wfd_sink_dump_pipeline_state(wfd_sink);
}

static void
_mm_wfd_sink_correct_pipeline_latency (mm_wfd_sink_t *wfd_sink, gboolean is_tcp)
{
  GstQuery *qlatency;
  GstClockTime min_latency;

  qlatency = gst_query_new_latency();
  gst_element_query (wfd_sink->sink_pipeline, qlatency);
  gst_query_parse_latency (qlatency, NULL, &min_latency, NULL);

  debug_msg ("Correct manually pipeline latency: current=%"GST_TIME_FORMAT, GST_TIME_ARGS (min_latency));
  g_object_set (wfd_sink->v_sink, "ts-offset", -(gint64)min_latency, NULL);
  g_object_set (wfd_sink->a_sink, "ts-offset", -(gint64)min_latency, NULL);
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
      if( message->src != (GstObject *)wfd_sink->sink_pipeline )
        ret = GST_BUS_DROP;
    }
      break;
    case GST_MESSAGE_ASYNC_DONE:
      if( message->src != (GstObject *)wfd_sink->sink_pipeline )
        ret = GST_BUS_DROP;
      break;
    default:
      break;
  }

  return ret;
}

static gboolean
_mm_wfd_sink_msg_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
  mm_wfd_sink_t* wfd_sink = (mm_wfd_sink_t*) data;
  gboolean ret = TRUE;
  gchar** splited_message;
  const gchar* message_structure_name;
  const GstStructure* message_structure = gst_message_get_structure(msg);

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

      /* get error code */
      gst_message_parse_error( msg, &error, &debug );

      debug_warning("error : %s\n", error->message);
      debug_warning("debug : %s\n", debug);

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

      /* we only handle messages from pipeline */
      if( msg->src != (GstObject *)wfd_sink->sink_pipeline )
        break;

      /* get state info from msg */
      voldstate = gst_structure_get_value (msg->structure, "old-state");
      vnewstate = gst_structure_get_value (msg->structure, "new-state");
      vpending = gst_structure_get_value (msg->structure, "pending-state");

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
        _mm_wfd_sink_correct_pipeline_latency (wfd_sink, wfd_sink->is_tcp_transport);
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

    case GST_MESSAGE_APPLICATION:
      message_structure_name = gst_structure_get_name(message_structure);
      debug_log("message name : %s", message_structure_name);
      splited_message = g_strsplit((gchar*)message_structure_name, "_", 2);
      if(g_strrstr(message_structure_name, "audiotype"))
      {
        wfd_sink->audio_type = g_strdup (splited_message[1]);
        g_print("USING %s AUDIO FORMAT\n", wfd_sink->audio_type);
      }
      else if(g_strrstr(message_structure_name, "volume"))
      {
        gdouble volume_param = g_ascii_strtod(splited_message[1], NULL);
        g_object_set (wfd_sink->volume, "volume", volume_param/1.5, NULL);
      }
      else if(g_strrstr(message_structure_name, "PLAY"))
      {
        gst_element_set_state(wfd_sink->sink_pipeline, GST_STATE_PLAYING);
      }
      else if(g_strrstr(message_structure_name, "PAUSE"))
      {
        gst_element_set_state(wfd_sink->sink_pipeline, GST_STATE_PAUSED);
      }
      else if(g_strrstr(message_structure_name, "TEARDOWN"))
      {
        debug_warning ("Got TEARDOWN.. Closing..\n");
        _mm_wfd_sink_stop (wfd_sink);
        __mm_wfd_post_message(wfd_sink, MM_WFD_SINK_STATE_TEARDOWN);
      }
      g_strfreev(splited_message);

      break;

    case GST_MESSAGE_ELEMENT:
      {
        const gchar *structure_name = NULL;
        if(msg->structure == NULL)
          break;
        structure_name = gst_structure_get_name(msg->structure);
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

    case GST_MESSAGE_REQUEST_STATE:		debug_log("GST_MESSAGE_REQUEST_STATE\n");
    break;
    case GST_MESSAGE_STEP_START:		debug_log("GST_MESSAGE_STEP_START\n");
    break;
    case GST_MESSAGE_QOS:					//debug_log("GST_MESSAGE_QOS\n");
    break;
    case GST_MESSAGE_PROGRESS:			debug_log("GST_MESSAGE_PROGRESS\n");
    break;
    case GST_MESSAGE_ANY:				debug_log("GST_MESSAGE_ANY\n");
    break;

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
  GstElement *sink_pipeline = NULL;
  GstElement *wfd_element = NULL;
  GstElement *depay_element = NULL;
  GstElement *demuxer_element = NULL;
  GstElement *fake_element = NULL;

  GstStructure *audio_param = NULL;
  GstStructure *video_param = NULL;
  GstStructure *hdcp_param = NULL;

  mm_wfd_ini_t_sink *sink_str = mm_wfd_ini_get_structure_sink();

  debug_fenter();

  sink_pipeline =gst_pipeline_new("wfdsink_pipeline");
  if (!sink_pipeline) {
    debug_error("failed to create wfd sink bin...");
    goto CREATE_ERROR;
  }

  wfd_sink->sink_pipeline = sink_pipeline;

  wfd_element = gst_element_factory_make("wfdrtspsrc", "wfd_rtsp_source");
  if (!wfd_element)	{
    debug_error("failed to create wfd streaming source element\n");
    goto CREATE_ERROR;
  }

  g_object_set(G_OBJECT(wfd_element), "location", uri, NULL);
  g_object_set(G_OBJECT(wfd_element), "debug", TRUE, NULL);
  g_object_set(G_OBJECT(wfd_element), "latency", 0, NULL);

  audio_param = gst_structure_new ("audio_param",
      "audio_codec", G_TYPE_UINT, sink_str->audio_codec,
      "audio_latency", G_TYPE_UINT, sink_str->audio_latency,
      "audio_channels", G_TYPE_UINT, sink_str->audio_channel,
      "audio_sampling_frequency", G_TYPE_UINT, sink_str->audio_sampling_frequency,
      NULL);

  video_param = gst_structure_new ("video_param",
      "video_codec", G_TYPE_UINT, sink_str->video_codec,
      "video_native_resolution", G_TYPE_UINT, sink_str->video_native_resolution,
      "video_cea_support", G_TYPE_UINT, sink_str->video_cea_support,
      "video_vesa_support", G_TYPE_UINT, sink_str->video_vesa_support,
      "video_hh_support", G_TYPE_UINT, sink_str->video_hh_support,
      "video_profile", G_TYPE_UINT, sink_str->video_profile,
      "video_level", G_TYPE_UINT, sink_str->video_level,
      "video_latency", G_TYPE_UINT, sink_str->video_latency,
      "video_vertical_resolution", G_TYPE_INT, sink_str->video_vertical_resolution,
      "video_horizontal_resolution", G_TYPE_INT, sink_str->video_horizontal_resolution,
      "video_minimum_slicing", G_TYPE_INT, sink_str->video_minimum_slicing,
      "video_slice_enc_param", G_TYPE_INT, sink_str->video_slice_enc_param,
      "video_framerate_control_support", G_TYPE_INT, sink_str->video_framerate_control_support,
      NULL);

  hdcp_param = gst_structure_new ("video_sink_param",
      "hdcp_content_protection", G_TYPE_INT, sink_str->hdcp_content_protection,
      "hdcp_port_no", G_TYPE_INT, sink_str->hdcp_port_no,
      NULL);

  g_object_set(G_OBJECT(wfd_element), "audio-param", audio_param, NULL);
  g_object_set(G_OBJECT(wfd_element), "video-param", video_param, NULL);
  g_object_set(G_OBJECT(wfd_element), "hdcp-param", hdcp_param, NULL);

  if (!wfd_element) {
    debug_error("no source element was created.\n");
    goto CREATE_ERROR;
  }
  g_signal_connect (wfd_element, "pad-added", G_CALLBACK(wfd_pad_added), wfd_sink);
    gst_element_set_state(wfd_element, GST_STATE_READY);

  wfd_sink->wfd = wfd_element;

    depay_element = gst_element_factory_make("rtpmp2tdepay", "wfd_rtp_depay");
    if (!depay_element) {
      debug_error ( "failed to create wfd rtp depay element\n" );
      goto CREATE_ERROR;
    }
    gst_element_set_state(depay_element, GST_STATE_READY);

  wfd_sink->depay = depay_element;

    demuxer_element = gst_element_factory_make("mpegtsdemux", "mpegts_demuxer");
    if (!demuxer_element) {
      debug_error ( "failed to create mpegtsdemux element\n" );
      goto CREATE_ERROR;
    }

  g_object_set(G_OBJECT(demuxer_element), "enable-wfd", TRUE, NULL);

  g_signal_connect (demuxer_element, "pad-added", G_CALLBACK(pad_added), wfd_sink);
    gst_element_set_state(demuxer_element, GST_STATE_READY);

  wfd_sink->demux = demuxer_element;

  g_object_set(G_OBJECT(wfd_element), "demux-handle", demuxer_element, NULL);

  gst_bin_add_many(GST_BIN(sink_pipeline), wfd_element, depay_element, demuxer_element, NULL);

    fake_element = gst_element_factory_make("fakesink", "fake_sink");
    if (!depay_element) {
      debug_error ( "failed to create wfd rtp depay element\n" );
      goto CREATE_ERROR;
    }
  gst_bin_add(GST_BIN(sink_pipeline), fake_element);

  wfd_sink->fakesink = fake_element;

  /* connect bus callback */
  bus = gst_pipeline_get_bus(GST_PIPELINE(sink_pipeline));
  if (!bus) {
    debug_error ("cannot get bus from pipeline.\n");
    goto CREATE_ERROR;
  }

  gst_bus_add_watch(bus, (GstBusFunc)_mm_wfd_sink_msg_callback, wfd_sink);

  /* set sync handler to get tag synchronously */
  gst_bus_set_sync_handler(bus, _mm_wfd_bus_sync_callback, wfd_sink);

  debug_log("End of _mm_wfd_sink_create_gst_pipeline");

  return result;

CREATE_ERROR:

  return result;
  }

int _mm_wfd_sink_create_gst_audio_pipeline(mm_wfd_sink_t *wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  GstElement *a_bin = NULL;
  GstElement *a_queue_element = NULL;
  GstElement *a_parse_element = NULL;
  GstElement *a_dec_element = NULL;
  GstElement *a_resample_element = NULL;
  GstElement *volume = NULL;
  GstElement *a_sink_element = NULL;

  a_bin = gst_bin_new("audio_bin");
  if (!a_bin) {
      debug_error ( "failed to create audio bin\n" );
    goto CREATE_ERROR;
  }
  gst_element_set_state(a_bin, GST_STATE_READY);

  a_queue_element = gst_element_factory_make("queue", "audio_queue");
  if (!a_queue_element) {
    debug_error ( "failed to create audio_queue element\n" );
    goto CREATE_ERROR;
  }

  g_object_set(G_OBJECT(wfd_sink->wfd), "audio-queue-handle", a_queue_element, NULL);

  volume = gst_element_factory_make("volume", "volume");
  if (!volume) {
    debug_error ( "failed to create volume element\n" );
    goto CREATE_ERROR;
  }

  a_sink_element = gst_element_factory_make("avsysaudiosink", "audio_sink");
  if (!a_sink_element) {
    debug_error ( "failed to create audio_sink element\n" );
    goto CREATE_ERROR;
  }
  g_object_set (a_sink_element, "buffer-time", 2000LL, "latency-time", 1000LL, NULL);
  g_object_set (a_sink_element, "latency", 0, NULL);

  a_resample_element = gst_element_factory_make("audioresample", "audio_conv");
  if (!a_resample_element) {
    debug_error ( "failed to create audio_conv element\n" );
    goto CREATE_ERROR;
  }

  wfd_sink->a_queue = a_queue_element;
  wfd_sink->a_conv = a_resample_element;
  wfd_sink->volume = volume;
  wfd_sink->a_sink = a_sink_element;
  wfd_sink->sink_abin = a_bin;

  if (g_strrstr(wfd_sink->audio_type,"AAC")) {

    a_parse_element = gst_element_factory_make("aacparse", "audio_parser");
    if (!a_parse_element) {
      debug_error ( "failed to create audio_parser element\n" );
      goto CREATE_ERROR;
    }

    a_dec_element = gst_element_factory_make("savsdec_aac", "audio_dec");
    if (!a_dec_element) {
      debug_error ( "failed to create audio_dec element\n" );
      goto CREATE_ERROR;
    }

    wfd_sink->a_parse = a_parse_element;
    wfd_sink->a_dec = a_dec_element;

    gst_bin_add_many(GST_BIN(wfd_sink->sink_abin), a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL);
    if (!gst_element_link_many(a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL)) {
      debug_error("Failed to link audio src elements...");
      goto CREATE_ERROR;
    }
  }
  else if (g_strrstr(wfd_sink->audio_type,"AC3")) {

    a_parse_element = gst_element_factory_make("ac3parse", "audio_parser");
    if (!a_parse_element) {
      debug_error ( "failed to create audio_parser element\n" );
      goto CREATE_ERROR;
    }

    a_dec_element = gst_element_factory_make("savsdec_ac3", "audio_dec");
    if (!a_dec_element) {
      debug_error ( "failed to create audio_dec element\n" );
      goto CREATE_ERROR;
    }

    wfd_sink->a_parse = a_parse_element;
    wfd_sink->a_dec = a_dec_element;

    gst_bin_add_many(GST_BIN(wfd_sink->sink_abin), a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL);
    if (!gst_element_link_many(a_queue_element, a_parse_element, a_dec_element, a_resample_element, volume, a_sink_element, NULL)) {
      debug_error("Failed to link audio src elements...");
      goto CREATE_ERROR;
    }
  }
  else if (g_strrstr(wfd_sink->audio_type,"LPCM")) {

    a_parse_element = gst_element_factory_make("audioconvert", "audio_convert");
    if (NULL == a_parse_element) {
        debug_error(, "failed to create audio convert element");
        goto CREATE_ERROR;
    }
    wfd_sink->a_parse = a_parse_element;

    GstElement *acaps;
    acaps  = gst_element_factory_make ("capsfilter", "audiocaps");
    if (NULL == acaps) {
      debug_error ("failed to create audio capsilfter element");
      goto CREATE_ERROR;
    }

	g_object_set (G_OBJECT (acaps), "caps",
        gst_caps_new_simple ("audio/x-raw-int",
            "width", G_TYPE_INT, 16,
            "endianness", G_TYPE_INT, 1234,
            "signed", G_TYPE_BOOLEAN, TRUE,
            "depth", G_TYPE_INT, 16,
            "rate", G_TYPE_INT, 48000,
            "channels", G_TYPE_INT, 2, NULL), NULL);

    gst_bin_add_many(GST_BIN(wfd_sink->sink_abin), a_queue_element, a_parse_element, acaps, a_resample_element, volume, a_sink_element, NULL);
    if (!gst_element_link_many(a_queue_element, a_parse_element, acaps, a_resample_element, volume, a_sink_element, NULL)) {
      debug_error("Failed to link audio src elements...");
      goto CREATE_ERROR;
    }
  }

  GstPad *spad = NULL;
  GstPad *ghostpad = NULL;

  spad = gst_element_get_static_pad(GST_ELEMENT(wfd_sink->a_queue), "sink");
  if (spad == NULL) {
      debug_error("Failed to get pad in audio queue\n");
      goto CREATE_ERROR;
  }

  ghostpad = gst_ghost_pad_new("sink", spad);
  if (ghostpad == NULL) {
      debug_error("Failed to create ghost pad \n");
      gst_object_unref(spad);
      goto CREATE_ERROR;
  }
    gst_object_unref(spad);

  if (FALSE == gst_element_add_pad(wfd_sink->sink_abin, ghostpad)) {
    debug_error("failed to add ghostpad to video bin\n");
        goto CREATE_ERROR;
  }

  if (gst_bin_add(GST_BIN(wfd_sink->sink_pipeline), wfd_sink->sink_abin) == FALSE) {
    debug_error("failed to add videobin to pipeline \n");
        goto CREATE_ERROR;
  }

  gst_element_set_state(GST_ELEMENT(wfd_sink->sink_abin), GST_STATE_PLAYING);
  debug_log("End of _mm_wfd_sink_create_gst_audio_pipeline");

  return result;
CREATE_ERROR:

  return result;
}

int _mm_wfd_sink_update_video_param(mm_wfd_sink_t *wfd_sink) // @
{
  int surface_type = 0;

  /* check video sinkbin is created */
  return_val_if_fail ( wfd_sink &&
    wfd_sink->sink_pipeline &&
    wfd_sink->sink_vbin &&
    wfd_sink->v_sink,
    MM_ERROR_PLAYER_NOT_INITIALIZED );

  if ( !wfd_sink->attrs )
  {
    debug_error("cannot get content attribute");
    return MM_ERROR_PLAYER_INTERNAL;
  }

  /* update display surface */
  mm_attrs_get_int_by_name(wfd_sink->attrs, "display_surface_type", &surface_type);
  debug_log("check display surface type attribute: %d", surface_type);

  /* configuring display */
  switch ( surface_type )
  {
    case MM_DISPLAY_SURFACE_EVAS:
    {
      void *object = NULL;
      int scaling = 0;
      gboolean visible = TRUE;

      /* common case if using evas surface */
      mm_attrs_get_data_by_name(wfd_sink->attrs, "display_overlay", &object);
      mm_attrs_get_int_by_name(wfd_sink->attrs, "display_visible", &visible);
      mm_attrs_get_int_by_name(wfd_sink->attrs, "display_evas_do_scaling", &scaling);
      if (object)
      {
        g_object_set(wfd_sink->v_sink,
            "evas-object", object,
            "visible", visible,
            NULL);
        debug_log("set video param : evas-object %x, visible %d", object, visible);
      }
      else
      {
        debug_error("no evas object");
        return MM_ERROR_PLAYER_INTERNAL;
      }
    }
    break;
    case MM_DISPLAY_SURFACE_NULL:
    {
      /* do nothing */
    }
    break;
  }

  return MM_ERROR_NONE;
}

int _mm_wfd_sink_create_gst_video_pipeline(mm_wfd_sink_t *wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  GstElement *v_bin = NULL;
  GstElement *v_queue_element = NULL;
  GstElement *v_parse_element = NULL;
  GstElement *v_dec_element = NULL;
  GstElement *v_sink_element = NULL;

  mm_wfd_ini_t_sink *sink_str = mm_wfd_ini_get_structure_sink();

  v_bin = gst_bin_new("video_bin");
  if (!v_bin) {
      debug_error ( "failed to create video bin\n" );
    goto CREATE_ERROR;
  }

  gst_element_set_state(v_bin, GST_STATE_READY);

  v_queue_element = gst_element_factory_make("queue", "video_queue");
  if (!v_queue_element) {
      debug_error ( "failed to create video_queue element\n" );
    goto CREATE_ERROR;
  }

  v_parse_element = gst_element_factory_make("h264parse", "video_parser");
  if (!v_parse_element) {
    debug_error ( "failed to create video_parser element\n" );
    goto CREATE_ERROR;
  }
  g_object_set(v_parse_element, "wfd-mode", TRUE, NULL);

  v_dec_element = gst_element_factory_make("omx_h264dec", "video_dec");
  if (!v_dec_element) {
    debug_error ( "failed to create video_dec element\n" );
    goto CREATE_ERROR;
  }

  if(sink_str->video_sink == 0) {
    v_sink_element = gst_element_factory_make("xvimagesink", "video_sink");
  } else if (sink_str->video_sink == 1) {
    v_sink_element = gst_element_factory_make("evasimagesink", "video_sink");
  }
  if (!v_sink_element) {
    debug_error ( "failed to create video_sink element\n" );
    goto CREATE_ERROR;
  }
  wfd_sink->v_sink = v_sink_element;
  wfd_sink->sink_vbin = v_bin;
  wfd_sink->v_queue = v_queue_element;
  //g_object_set(v_sink_element, "sync", FALSE, NULL);

  _mm_wfd_sink_update_video_param(wfd_sink);

  gst_bin_add_many(GST_BIN(wfd_sink->sink_vbin), v_queue_element, v_parse_element, v_dec_element, v_sink_element, NULL);
  if (!gst_element_link_many(v_queue_element, v_parse_element, v_dec_element, v_sink_element, NULL)) {
    debug_error("Failed to link video elements...");
    goto CREATE_ERROR;
  }

  GstPad *spad = NULL;
  GstPad *ghostpad = NULL;

  spad = gst_element_get_static_pad(GST_ELEMENT(wfd_sink->v_queue), "sink");
  if (spad == NULL) {
    debug_error("Failed to get pad in video queue\n");
    goto CREATE_ERROR;
  }

  ghostpad = gst_ghost_pad_new("sink", spad);
  if (ghostpad == NULL) {
    debug_error("Failed to create ghost pad \n");
    gst_object_unref(spad);
    goto CREATE_ERROR;
  }
  gst_object_unref(spad);

  if (FALSE == gst_element_add_pad(wfd_sink->sink_vbin, ghostpad)) {
    debug_error("failed to add ghostpad to video bin\n");
    goto CREATE_ERROR;
  }

  if (gst_bin_add(GST_BIN(wfd_sink->sink_pipeline), wfd_sink->sink_vbin) == FALSE) {
    debug_error("failed to add videobin to pipeline \n");
    goto CREATE_ERROR;
  }

  gst_element_set_state(GST_ELEMENT(wfd_sink->sink_vbin), GST_STATE_PLAYING);
  debug_log("End of _mm_wfd_sink_create_gst_video_pipeline");
  return result;

CREATE_ERROR:

  return result;
}

