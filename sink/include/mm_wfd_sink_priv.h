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

#ifndef _MM_WFD_SINK_PRIV_H_
#define _MM_WFD_SINK_PRIV_H_

#include <string.h>
#include <glib.h>
#include <mm_types.h>
#include <mm_attrs.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>
#include <mm_wfd_ini.h>
#include <mm_wfd_attrs.h>
#include <gst/gst.h>

#include "mm_wfd_sink.h"

#define MMWFDSINK_FREEIF(x) \
if ( x ) \
  g_free( x ); \
x = NULL;

#define MMWFD_SINK_CMD_LOCK(x_wfd) \
if(x_wfd && ((mm_wfd_sink_t*)x_wfd)->cmd_lock) \
  g_mutex_lock(((mm_wfd_sink_t*)x_wfd)->cmd_lock);

#define MMWFD_SINK_CMD_UNLOCK(x_wfd) \
if(x_wfd && ((mm_wfd_sink_t*)x_wfd)->cmd_lock) \
  g_mutex_unlock(((mm_wfd_sink_t*)x_wfd)->cmd_lock);

#define MMWFDSINK_GET_ATTRS(x_wfd) ((x_wfd)? ((mm_wfd_sink_t*)x_wfd)->attrs : (MMHandleType)NULL)

/* audio & video mode */
/* NOTE : Currently, support only AV_MODE_AUDIO_VIDEO */
enum AVMode
{
  AV_MODE_AUDIO_OR_VIDEO_ONLY = 1,
  AV_MODE_AUDIO_AND_VIDEO
};

typedef struct {
  /* Element */
  GstElement *sink_pipeline;
  GstElement *sink_vbin;
  GstElement *sink_abin;

  GstElement *wfd;
  GstElement *depay;
  GstElement *demux;
  GstElement *a_queue;
  GstElement *a_parse;
  GstElement *a_dec;
  GstElement *a_conv;
  GstElement *volume;
  GstElement *a_sink;
  GstElement *v_sink;
  GstElement *v_queue;
  GstElement *fakesink;

  GstPad *rtp_pad_src;
  /* Content types */
  gchar *audio_type;

  MMHandleType attrs;

  GMutex* cmd_lock;

  /* STATE */
  MMWfdSinkStateType state;         // wfd current state
  MMWfdSinkStateType prev_state;    // wfd  previous state
  MMWfdSinkStateType pending_state; // wfd  state which is going to now
  MMWfdSinkStateType target_state;  // wfd  state which user want to go to

  gint AV_mode;      // Audio/Video mode
  gint added_av_pad_num;  // currently, added pad num.
  gboolean is_tcp_transport;

  /* Message handling */
  MMWFDMessageCallback msg_cb;
  void *msg_user_data;
} mm_wfd_sink_t;


int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink);
int _mm_wfd_sink_connect(mm_wfd_sink_t *wfd_sink, const char *uri);
int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_stop(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_unrealize(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_set_message_callback(mm_wfd_sink_t *wfd_sink, MMWFDMessageCallback callback, void *user_data);

int _mm_wfd_sink_gstreamer_init(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_create_gst_pipeline(mm_wfd_sink_t *wfd_sink, const char *uri);
int _mm_wfd_sink_create_gst_audio_pipeline(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_create_gst_video_pipeline(mm_wfd_sink_t *wfd_sink);

#endif

