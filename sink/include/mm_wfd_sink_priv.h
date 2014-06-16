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
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>

#include "mm_wfd_sink.h"

/* main pipeline's element id */
enum MainElementID
{
	MMPLAYER_M_PIPE = 0, /* NOTE : MMPLAYER_M_PIPE should be zero */
	MMPLAYER_M_SRC,
	MMPLAYER_M_SUBSRC,

	/* it could be a decodebin or could be a typefind. depends on player ini */
	MMPLAYER_M_AUTOPLUG,

	/* NOTE : we need two fakesink to autoplug without decodebin.
	 * first one will hold whole pipeline state. and second one will hold state of
	 * a sink-decodebin for an elementary stream. no metter if there's more then one
	 * elementary streams because MSL reuse it.
	 */
	MMPLAYER_M_SRC_FAKESINK,
	MMPLAYER_M_SRC_2ND_FAKESINK,

	/* streaming plugin */
	MMPLAYER_M_S_BUFFER,

	/* FIXIT : if there's really no usage for following IDs. remove it */
	MMPLAYER_M_DEC1,
	MMPLAYER_M_DEC2,
	MMPLAYER_M_Q1,
	MMPLAYER_M_Q2,
	MMPLAYER_M_DEMUX,
	MMPLAYER_M_SUBPARSE,
	MMPLAYER_M_DEMUX_EX,
	MMPLAYER_M_NUM
};

typedef struct
{
	int id;
	GstElement *gst;
} MMWfdSinkGstElement;

typedef struct
{
	MMWfdSinkGstElement 	*mainbin; /* Main bin */
	MMWfdSinkGstElement 	*asrcbin; /* audio source bin */
	MMWfdSinkGstElement 	*vsrcbin; /* video source bin */
} MMWfdSinkGstPipelineInfo;

typedef struct {
	/* Element */
	GstElement *sink_bin;
	GstElement *wfd;
	GstElement *depay;
	GstElement *demux;
	GstElement *a_queue;
	GstElement *a_parse;
	GstElement *a_dec;
	GstElement *volume;
	GstElement *a_sink;
	GstElement *v_queue;
	GstElement *fakesink;

	/* Content types */
	gchar *audio_type;

	/* STATE */
	MMWfdSinkStateType state;			// wfd current state
	MMWfdSinkStateType prev_state;		// wfd  previous state
	MMWfdSinkStateType pending_state;		// wfd  state which is going to now
	MMWfdSinkStateType target_state;		// wfd  state which user want to go to

	MMWfdSinkGstPipelineInfo	*pipeline;

	void *server;
	void *client;

	/* wfdsink attributes */
	MMHandleType attrs;

	/* message callback */
	MMMessageCallback msg_cb;
	void* msg_cb_param;

	void *mapping;
	void *factory;

} mm_wfd_sink_t;


int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink, const char *uri);
int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_stop(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_unrealize(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink);

int _mm_wfd_sink_gstreamer_init(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_create_gst_pipeline(mm_wfd_sink_t *wfd_sink, const char *uri);
int _mm_wfd_sink_create_gst_audio_pipeline(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_create_gst_video_pipeline(mm_wfd_sink_t *wfd_sink);

#endif

