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

#include "mm_wfd_sink_ini.h"
#include "mm_wfd_sink_attrs.h"
#include "mm_wfd_sink.h"

/* main pipeline's element id */
enum WFDSinkMainElementID {
	WFD_SINK_M_PIPE = 0, /* NOTE : WFD_SINK_M_PIPE should be zero */
	WFD_SINK_M_SRC,
	WFD_SINK_M_DEPAY,
	WFD_SINK_M_DEMUX,
	WFD_SINK_M_NUM
};

/* audio decodebin's element id */
enum WFDSinkAudioDecodeBinElementID {
	WFD_SINK_A_D_BIN = 0, /* NOTE : WFD_SINK_A_D_BIN should be zero */
	WFD_SINK_A_D_QUEUE,
	WFD_SINK_A_D_HDCP,
	WFD_SINK_A_D_AAC_PARSE,
	WFD_SINK_A_D_AAC_DEC,
	WFD_SINK_A_D_AC3_PARSE,
	WFD_SINK_A_D_AC3_DEC,
	WFD_SINK_A_D_LPCM_CONVERTER,
	WFD_SINK_A_D_LPCM_FILTER,
	WFD_SINK_A_D_NUM
};

/* audio sinkbin's element id */
enum WFDSinkAudioSinkBinElementID {
	WFD_SINK_A_S_BIN = 0, /* NOTE : WFD_SINK_A_S_BIN should be zero */
	WFD_SINK_A_S_RESAMPLER,
	WFD_SINK_A_S_VOLUME,
	WFD_SINK_A_S_SINK,
	WFD_SINK_A_S_NUM
};

/* video decodebin's element id */
enum WFDSinkVideoDecodeBinElementID {
	WFD_SINK_V_D_BIN = 0, /* NOTE : WFD_SINK_V_D_BIN should be zero */
	WFD_SINK_V_D_QUEUE,
	WFD_SINK_V_D_HDCP,
	WFD_SINK_V_D_H264_PARSE,
	WFD_SINK_V_D_CAPSSETTER,
	WFD_SINK_V_D_H264_DEC,
	WFD_SINK_V_D_NUM
};

/* video sinkbin's element id */
enum WFDSinkVideoSinkBinElementID {
	WFD_SINK_V_S_BIN = 0, /* NOTE : WFD_SINK_V_S_BIN should be zero */
	WFD_SINK_V_S_CONVERT,
	WFD_SINK_V_S_FILTER,
	WFD_SINK_V_S_SINK,
	WFD_SINK_V_S_NUM
};

/**
 *  * Enumerations of wifi-display command.
 *   */
typedef enum {
	MM_WFD_SINK_COMMAND_NONE,		/**< command for nothing */
	MM_WFD_SINK_COMMAND_CREATE,		/**< command for creating wifi-display sink */
	MM_WFD_SINK_COMMAND_PREPARE,		/**< command for preparing wifi-display sink */
	MM_WFD_SINK_COMMAND_CONNECT,	/**< command for connecting wifi-display sink  */
	MM_WFD_SINK_COMMAND_START,	 /**< command for starting wifi-display sink  */
	MM_WFD_SINK_COMMAND_PAUSE,	 /**< command for pausing wifi-display sink  */
	MM_WFD_SINK_COMMAND_RESUME,	 /**< command for resuming wifi-display sink  */
	MM_WFD_SINK_COMMAND_DISCONNECT,	/**< command for disconnecting wifi-display sink  */
	MM_WFD_SINK_COMMAND_UNPREPARE,		/**< command for unpreparing wifi-display sink  */
	MM_WFD_SINK_COMMAND_DESTROY,		/**< command for destroting wifi-display sink  */
	MM_WFD_SINK_COMMAND_NUM,		/**< Number of wifi-display commands */
} MMWFDSinkCommandType;

/**
 *  * Enumerations of thread command.
 *   */
typedef enum {
	WFD_SINK_MANAGER_CMD_NONE = 0,
	WFD_SINK_MANAGER_CMD_LINK_A_DECODEBIN,
	WFD_SINK_MANAGER_CMD_LINK_V_DECODEBIN,
	WFD_SINK_MANAGER_CMD_PREPARE_A_PIPELINE,
	WFD_SINK_MANAGER_CMD_PREPARE_V_PIPELINE,
	WFD_SINK_MANAGER_CMD_EXIT,
} WFDSinkManagerCMDType;

/**
 *  * Enumerations of resolution.
 *   */
typedef enum {
	MM_WFD_SINK_RESOLUTION_UNKNOWN = 0,
	MM_WFD_SINK_RESOLUTION_1920x1080_P30 = (1 << 0),  /**< W-1920, H-1080, 30 fps*/
	MM_WFD_SINK_RESOLUTION_1280x720_P30 = (1 << 1),   /**< W-1280, H-720, 30 fps*/
	MM_WFD_SINK_RESOLUTION_960x540_P30 = (1 << 2),    /**< W-960, H-540, 30 fps*/
	MM_WFD_SINK_RESOLUTION_864x480_P30 = (1 << 3),    /**< W-864, H-480, 30 fps*/
	MM_WFD_SINK_RESOLUTION_720x480_P60 = (1 << 4),    /**< W-720, H-480, 30 fps*/
	MM_WFD_SINK_RESOLUTION_640x480_P60 = (1 << 5),    /**< W-640, H-480, 60 fps*/
	MM_WFD_SINK_RESOLUTION_640x360_P30 = (1 << 6),    /**< W-640, H-360, 30 fps*/
	MM_WFD_SINK_RESOLUTION_MAX = 128,
} MMWFDSinkResolution;

typedef struct {
	gint codec;
	gint width;
	gint height;
	gint frame_rate;
} MMWFDSinkVideoStreamInfo;

typedef struct {
	gint codec;
	gint channels;
	gint sample_rate;
	gint bitwidth;
} MMWFDSinkAudioStreamInfo;

typedef struct {
	MMWFDSinkAudioStreamInfo audio_stream_info;
	MMWFDSinkVideoStreamInfo video_stream_info;
} MMWFDSinkStreamInfo;


typedef struct {
	gint id;
	GstElement *gst;
} MMWFDSinkGstElement;

typedef struct {
	MMWFDSinkGstElement 	*mainbin;
	MMWFDSinkGstElement 	*a_decodebin;
	MMWFDSinkGstElement 	*v_decodebin;
	MMWFDSinkGstElement 	*a_sinkbin;
	MMWFDSinkGstElement 	*v_sinkbin;
} MMWFDSinkGstPipelineInfo;

typedef struct {
	MMWFDSinkStateType state;         /* wfd current state */
	MMWFDSinkStateType prev_state;    /* wfd  previous state */
	MMWFDSinkStateType pending_state; /* wfd  state which is going to now */
} MMWFDSinkState;

/* priv func */
typedef struct {
	int(*prepare)(MMHandleType wfd_sink);
	int(*connect)(MMHandleType wfd_sink, const char *uri);
	int(*start)(MMHandleType wfd_sink);
	int(*pause)(MMHandleType wfd_sink);
	int(*resume)(MMHandleType wfd_sink);
	int(*disconnect)(MMHandleType wfd_sink);
	int(*unprepare)(MMHandleType wfd_sink);
} MMWFDSinkPrivateFunc;

#define MMWFDSINK_GET_ATTRS(x_wfd) ((x_wfd)? ((mm_wfd_sink_t*)x_wfd)->attrs : (MMHandleType)NULL)

typedef struct {
	/* gstreamer pipeline */
	MMWFDSinkGstPipelineInfo *pipeline;
	gboolean audio_decodebin_is_linked;
	gboolean video_decodebin_is_linked;

	/* timestamp compensation */
	gboolean need_to_reset_basetime;

	GstClock *clock;
	gint64 video_average_gap;
	gint64 video_accumulated_gap;
	gint64 video_buffer_count;
	gint64 audio_average_gap;
	gint64 audio_accumulated_gap;
	gint64 audio_buffer_count;
	GstClockTime last_buffer_timestamp;

	/* attributes */
	MMHandleType attrs;

	/* state */
	MMWFDSinkState state;

	/* initialize values */
	mm_wfd_sink_ini_t ini;

	/* command */
	MMWFDSinkCommandType cmd;
	GMutex cmd_lock;
	gboolean waiting_cmd;

	/* stream information */
	MMWFDSinkStreamInfo stream_info;

	/* Message handling */
	MMWFDMessageCallback msg_cb;
	void *msg_user_data;

	/* video resolution for negotiation */
	MMWFDSinkResolution supportive_resolution;

	GThread         *manager_thread;
	GMutex manager_thread_mutex;
	GCond manager_thread_cond;
	GList *manager_thread_cmd;
	gboolean manager_thread_exit;

	MMWFDSinkPrivateFunc *func_table;
} mm_wfd_sink_t;


int _mm_wfd_sink_create(mm_wfd_sink_t **wfd_sink, const char *ini_path);
int _mm_wfd_sink_destroy(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_prepare(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_unprepare(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_connect(mm_wfd_sink_t *wfd_sink, const char *uri);
int _mm_wfd_sink_disconnect(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_start(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_pause(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_sink_resume(mm_wfd_sink_t *wfd_sink);
int _mm_wfd_set_message_callback(mm_wfd_sink_t *wfd_sink, MMWFDMessageCallback callback, void *user_data);
int _mm_wfd_sink_set_resolution(mm_wfd_sink_t *wfd_sink, MMWFDSinkResolution resolution);

int __mm_wfd_sink_link_audio_decodebin(mm_wfd_sink_t *wfd_sink);
int __mm_wfd_sink_link_video_decodebin(mm_wfd_sink_t *wfd_sink);
int __mm_wfd_sink_prepare_video_pipeline(mm_wfd_sink_t *wfd_sink);
int __mm_wfd_sink_prepare_audio_pipeline(mm_wfd_sink_t *wfd_sink);

const gchar *_mm_wfds_sink_get_state_name(MMWFDSinkStateType state);

#endif

