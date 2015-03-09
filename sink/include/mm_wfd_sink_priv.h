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
#include <stdlib.h>
#include <glib.h>
#include <gst/gst.h>
#include <mm_types.h>
#include <mm_attrs.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>
#include <mm_wfd_sink_ini.h>
#include <mm_wfd_attrs.h>

#include "mm_wfd_sink.h"

/* main pipeline's element id */
enum WFDSinkMainElementID
{
	WFD_SINK_M_PIPE = 0, /* NOTE : WFD_SINK_M_PIPE should be zero */
	WFD_SINK_M_SRC,
	WFD_SINK_M_DEPAY,
	WFD_SINK_M_DEMUX,
	WFD_SINK_M_NUM
};

/* audio pipeline's element id */
enum WFDSinkAudioElementID
{
	WFD_SINK_A_BIN = 0, /* NOTE : WFD_SINK_A_BIN should be zero */
	WFD_SINK_A_QUEUE,
	WFD_SINK_A_HDCP,
	WFD_SINK_A_AAC_PARSE,
	WFD_SINK_A_AAC_DEC,
	WFD_SINK_A_AC3_PARSE,
	WFD_SINK_A_AC3_DEC,
	WFD_SINK_A_LPCM_CONVERTER,
	WFD_SINK_A_LPCM_FILTER,
	WFD_SINK_A_CAPSFILTER,
	WFD_SINK_A_RESAMPLER,
	WFD_SINK_A_VOLUME,
	WFD_SINK_A_SINK,
	WFD_SINK_A_NUM
};

/* video pipeline's element id */
enum WFDSinkVideoElementID
{
	WFD_SINK_V_BIN = 0, /* NOTE : WFD_SINK_V_BIN should be zero */
	WFD_SINK_V_QUEUE,
	WFD_SINK_V_HDCP,
	WFD_SINK_V_PARSE,
	WFD_SINK_V_DEC,
	WFD_SINK_V_CONVERT,
	WFD_SINK_V_SINK,
	WFD_SINK_V_NUM
};

/* audio codec : AAC, AC3, LPCM  */
enum WFDSinkAudioCodec
{
	WFD_SINK_AUDIO_CODEC_NONE,
	WFD_SINK_AUDIO_CODEC_AAC = 0x0F,
	WFD_SINK_AUDIO_CODEC_AC3 = 0x81,
	WFD_SINK_AUDIO_CODEC_LPCM = 0x83
};

/* video codec : H264  */
enum WFDSinkVideoCodec
{
	WFD_SINK_VIDEO_CODEC_NONE,
	WFD_SINK_VIDEO_CODEC_H264 = 0x1b
};

/**
 *  * Enumerations of wifi-display command.
 *   */
typedef enum {
	MM_WFD_SINK_COMMAND_NONE,		/**< command for nothing */
	MM_WFD_SINK_COMMAND_CREATE,		/**< command for creating wifi-display sink */
	MM_WFD_SINK_COMMAND_REALIZE,		/**< command for realizing wifi-display sink */
	MM_WFD_SINK_COMMAND_CONNECT,	/**< command for connecting wifi-display sink  */
	MM_WFD_SINK_COMMAND_START,	 /**< command for starting wifi-display sink  */
	MM_WFD_SINK_COMMAND_STOP,	/**< command for stopping wifi-display sink  */
	MM_WFD_SINK_COMMAND_UNREALIZE,		/**< command for unrealizing wifi-display sink  */
	MM_WFD_SINK_COMMAND_DESTROY,		/**< command for destroting wifi-display sink  */
	MM_WFD_SINK_COMMAND_NUM,		/**< Number of wifi-display commands */
} MMWfdSinkCommandType;

/**
 *  * Enumerations of thread command.
 *   */
typedef enum {
	WFD_SINK_MANAGER_CMD_NONE = 0,
	WFD_SINK_MANAGER_CMD_LINK_A_BIN = (1 << 0),
	WFD_SINK_MANAGER_CMD_LINK_V_BIN = (1 << 1),
	WFD_SINK_MANAGER_CMD_PREPARE_A_BIN = (1 << 2),
	WFD_SINK_MANAGER_CMD_PREPARE_V_BIN = (1 << 3),
	WFD_SINK_MANAGER_CMD_EXIT = (1 << 8)
} WFDSinkManagerCMDType;

typedef struct
{
	gint codec;
	gint width;
	gint height;
	gint frame_rate;
}MMWFDSinkVideoStreamInfo;

typedef struct
{
	gint codec;
	gint channels;
	gint sample_rate;
	gint bitwidth;
}MMWFDSinkAudioStreamInfo;

typedef struct
{
	MMWFDSinkAudioStreamInfo audio_stream_info;
	MMWFDSinkVideoStreamInfo video_stream_info;
}MMWFDSinkStreamInfo;


typedef struct
{
	gint id;
	GstElement *gst;
} MMWFDSinkGstElement;

typedef struct
{
	MMWFDSinkGstElement 	*mainbin;
	MMWFDSinkGstElement 	*audiobin;
	MMWFDSinkGstElement 	*videobin;
} MMWFDSinkGstPipelineInfo;

typedef struct
{
  	MMWfdSinkStateType state;         // wfd current state
  	MMWfdSinkStateType prev_state;    // wfd  previous state
  	MMWfdSinkStateType pending_state; // wfd  state which is going to now
} MMWFDSinkState;

#define MMWFDSINK_GET_ATTRS(x_wfd) ((x_wfd)? ((mm_wfd_sink_t*)x_wfd)->attrs : (MMHandleType)NULL)

typedef struct {
	/* gstreamer pipeline */
	MMWFDSinkGstPipelineInfo *pipeline;
	gboolean is_constructed;
	gint added_av_pad_num;
	gboolean audio_bin_is_linked;
	gboolean video_bin_is_linked;
	gboolean audio_bin_is_prepared;
	gboolean video_bin_is_prepared;
	GstPad * prev_audio_dec_src_pad;
	GstPad * next_audio_dec_sink_pad;
	guint demux_video_pad_probe_id;
	guint demux_audio_pad_probe_id;
	gboolean need_to_reset_basetime;
	/* attributes */
	MMHandleType attrs;

  	/* state */
	MMWFDSinkState state;

	/* initialize values */
	mm_wfd_sink_ini_t ini;

	/* command */
	MMWfdSinkCommandType cmd;
	GMutex* cmd_lock;


	/* stream information */
	MMWFDSinkStreamInfo stream_info;

	/* Message handling */
	MMWFDMessageCallback msg_cb;
	void *msg_user_data;

	/* audio session manager related variables */
	GList *resource_list;

	GThread         *manager_thread;
	GMutex *manager_thread_mutex;
	GCond* manager_thread_cond;
	WFDSinkManagerCMDType manager_thread_cmd;
} mm_wfd_sink_t;


int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink);
int _mm_wfd_sink_realize(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_connect(mm_wfd_sink_t *wfd_sink, const char *uri);
int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_stop(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_unrealize(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_set_message_callback(mm_wfd_sink_t *wfd_sink, MMWFDMessageCallback callback, void *user_data);
int _mm_wfd_sink_get_resource(mm_wfd_sink_t* wfd_sink);

int __mm_wfd_sink_link_audiobin(mm_wfd_sink_t *wfd_sink);
int __mm_wfd_sink_prepare_videobin (mm_wfd_sink_t *wfd_sink);
int __mm_wfd_sink_prepare_audiobin(mm_wfd_sink_t * wfd_sink);

#endif

