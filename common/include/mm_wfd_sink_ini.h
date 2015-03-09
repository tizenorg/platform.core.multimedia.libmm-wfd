/*
 * libmm-wfd
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, ByungWook Jang <bw.jang@samsung.com>,
 * Manoj Kumar K <manojkumar.k@samsung.com>, Hyunil Park <hyunil46.park@samsung.com>
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

#ifndef __MM_WFD_SINK_INI_H__
#define __MM_WFD_SINK_INI_H__

#include <glib.h>
#include <mmf/wfdconfigmessage.h>

#ifdef __cplusplus
  extern "C" {
#endif


enum WFDSinkINIProbeFlags
{
	WFD_SINK_INI_PROBE_DEFAULT = 0,
	WFD_SINK_INI_PROBE_TIMESTAMP = (1 << 0),
	WFD_SINK_INI_PROBE_BUFFERSIZE = (1 << 1),
	WFD_SINK_INI_PROBE_CAPS = (1 << 2),
	WFD_SINK_INI_PROBE_BUFFER_DURATION = (1 << 3),
};

#define MM_WFD_SINK_INI_DEFAULT_PATH	"/usr/etc/mmfw_wfd_sink.ini"

#define WFD_SINK_INI_MAX_STRLEN	256
#define WFD_SINK_INI_MAX_ELEMENT	10

/* NOTE : MMPlayer has no initalizing API for library itself
 * so we cannot decide when those ini values to be released.
 * this is the reason of all string items are static array.
 * make it do with malloc when MMPlayerInitialize() API created
 * before that time, we should be careful with size limitation
 * of each string item.
 */
typedef struct __mm_wfd_sink_ini
{
	/* general */
	gchar gst_param[5][WFD_SINK_INI_MAX_STRLEN];
	gboolean generate_dot;
	gboolean enable_pad_probe;
	gint state_change_timeout;
	gboolean set_debug_property;
	gboolean enable_asm;
	gint jitter_buffer_latency;
	gint video_sink_max_lateness;
	gboolean enable_retransmission;
	gboolean enable_reset_basetime;
	gboolean enable_ts_data_dump;
	gboolean enable_wfdrtspsrc_pad_probe;

	/* pipeline */
	gchar name_of_tsdemux[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_audio_hdcp[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_aac_parser[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_aac_decoder[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_ac3_parser[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_ac3_decoder[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_lpcm_converter[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_lpcm_filter[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_audio_resampler[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_audio_volume[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_audio_sink[WFD_SINK_INI_MAX_STRLEN];

	gchar name_of_video_hdcp[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_parser[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_decoder[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_sink[WFD_SINK_INI_MAX_STRLEN];

	/* audio parameter for reponse of M3 request */
	guint audio_codec;
	guint audio_latency;
	guint audio_channel;
	guint audio_sampling_frequency;

	/* video parameter for reponse of M3 request */
	guint video_codec;
	guint video_native_resolution;
	guint video_cea_support;
	guint video_vesa_support;
	guint video_hh_support;
	guint video_profile;
	guint video_level;
	guint video_latency;
	gint video_vertical_resolution;
	gint video_horizontal_resolution;
	gint video_minimum_slicing;
	gint video_slice_enc_param;
	gint video_framerate_control_support;

	/* hdcp parameter for reponse of M3 request */
	gint hdcp_content_protection;
	gint hdcp_port_no;
}mm_wfd_sink_ini_t;


/*Default sink ini values*/
/* General*/
#define DEFAULT_GST_PARAM	""
#define DEFAULT_GENERATE_DOT	FALSE
#define DEFAULT_ENABLE_PAD_PROBE	FALSE
#define DEFAULT_STATE_CHANGE_TIMEOUT 5 /* sec */
#define DEFAULT_SET_DEBUG_PROPERTY	TRUE
#define DEFAULT_ENABLE_ASM	FALSE
#define DEFAULT_JITTER_BUFFER_LATENCY 33 /* msec */
#define DEFAULT_ENABLE_RETRANSMISSION	TRUE
#define DEFAULT_ENABLE_RESET_BASETIME	TRUE
#define DEFAULT_VIDEO_SINK_MAX_LATENESS 20000000 /* nsec */
#define DEFAULT_ENABLE_TS_DATA_DUMP		FALSE
#define DEFAULT_ENABLE_WFDRTSPSRC_PAD_PROBE FALSE

/* Pipeline */
#define DEFAULT_NAME_OF_TSDEMUX ""
#define DEFAULT_NAME_OF_AUDIO_HDCP ""
#define DEFAULT_NAME_OF_AAC_PARSER ""
#define DEFAULT_NAME_OF_AAC_DECODER ""
#define DEFAULT_NAME_OF_AC3_PARSER ""
#define DEFAULT_NAME_OF_AC3_DECODER ""
#define DEFAULT_NAME_OF_LPCM_CONVERTER ""
#define DEFAULT_NAME_OF_LPCM_FILTER ""
#define DEFAULT_NAME_OF_AUDIO_RESAMPLER ""
#define DEFAULT_NAME_OF_AUDIO_VOLUME ""
#define DEFAULT_NAME_OF_AUDIO_SPLITTER ""
#define DEFAULT_NAME_OF_AUDIO_SINK ""
#define DEFAULT_NAME_OF_VIDEO_HDCP ""
#define DEFAULT_NAME_OF_VIDEO_PARSER ""
#define DEFAULT_NAME_OF_VIDEO_DECODER ""
#define DEFAULT_NAME_OF_VIDEO_SINK ""

/* Audio */
#define DEFAULT_AUDIO_CODEC WFD_AUDIO_LPCM | WFD_AUDIO_AAC | WFD_AUDIO_AC3
#define DEFAULT_AUDIO_LATENCY 0x0
#define DEFAULT_AUDIO_CHANNELS WFD_CHANNEL_2
#define DEFAULT_AUDIO_SAMP_FREQUENCY WFD_FREQ_44100 | WFD_FREQ_48000

/* Video */
#define DEFAULT_VIDEO_CODEC WFD_VIDEO_H264
#define DEFAULT_VIDEO_NATIVE_RESOLUTION 0x20
/* CEA :  WFD_CEA_640x480P60  | WFD_CEA_720x480P60 |WFD_CEA_720x576P50 |WFD_CEA_1280x720P30 |WFD_CEA_1920x1080P30 |
	WFD_CEA_1280x720P25 |WFD_CEA_1920x1080P25 |WFD_CEA_1280x720P24 | WFD_CEA_1920x1080P24 */
#define DEFAULT_VIDEO_CEA_SUPPORT 0x194ab
/* VESA : WFD_VESA_800x600P30  |WFD_VESA_1024x768P30 |WFD_VESA_1152x864P30 | WFD_VESA_1280x768P30	 | WFD_VESA_1280x800P30 |
	WFD_VESA_1360x768P30 | WFD_VESA_1366x768P30 | WFD_VESA_1280x1024P30 |WFD_VESA_1400x1050P30	 | WFD_VESA_1440x900P30 |
	WFD_VESA_1600x900P30 | FD_VESA_1600x1200P30 | WFD_VESA_1680x1024P30 | WFD_VESA_1680x1050P30	 | WFD_VESA_1920x1200P30 | */
#define DEFAULT_VIDEO_VESA_SUPPORT 0x55555555
/* HH : WFD_HH_800x480P30 | WFD_HH_854x480P30 | WFD_HH_864x480P30 | WFD_HH_640x360P30 | WFD_HH_960x540P30 | WFD_HH_848x480P30 */
#define DEFAULT_VIDEO_HH_SUPPORT 0x555
#define DEFAULT_VIDEO_PROFILE WFD_H264_BASE_PROFILE
#define DEFAULT_VIDEO_LEVEL WFD_H264_LEVEL_3_2
#define DEFAULT_VIDEO_LATENCY 0x0
#define DEFAULT_VIDEO_VERTICAL_RESOLUTION 1200
#define DEFAULT_VIDEO_HORIZONTAL_RESOLUTION 1920
#define DEFAULT_VIDEO_MIN_SLICESIZE 0
#define DEFAULT_VIDEO_SLICE_ENC_PARAM 200
#define DEFAULT_VIDEO_FRAMERATE_CONTROL 11

/* HDCP */
#define DEFAULT_HDCP_CONTENT_PROTECTION 0x0
#define DEFAULT_HDCP_PORT_NO 0


#define MM_WFD_SINK_DEFAULT_INI \
" \
[general]\n\
; parameters for initializing gstreamer\n\
; DEFAULT SET (--gst-debug=2,*wfd*:5)\n\
gstparam1 = --gst-debug=2,*wfd*:5\n\
gstparam2 =\n\
gstparam3 =\n\
gstparam4 =\n\
gstparam5 =\n\
\n\
; generating dot file representing pipeline state\n\
; do export GST_DEBUG_DUMP_DOT_DIR=[dot file path] in the shell\n\
generate dot = no\n\
\n\
; enable pad probe\n\
enable pad probe = no\n\
\n\
; enable wfdrtspsrc inner pad probe\n\
enable wfdrtspsrc pad probe = no\n\
\n\
; enable ts data dump (/var/tmp/*.ts)\n\
enable ts data dump = no\n\
\n\
; allowed timeout for changing pipeline state\n\
state change timeout = 5 ; sec\n\
\n\
; set debug property to wfdrtspsrc plugin for debugging rtsp message\n\
set debug property = yes\n\
\n\
; for asm function enable = yes, disable = no\n\
enable asm = no\n\
\n\
; 0: default value set by wfdrtspsrc element, other: user define value.\n\
jitter buffer latency=33\n\
\n\
; for retransmission request enable = yes, disable = no\n\
enable retransmission = yes\n\
\n\
; for reset basetime, enable = yes, disable = no\n\
enable reset basetime = yes\n\
\n\
; Maximum number of nanoseconds that a buffer can be late before it is dropped by videosink (-1 unlimited)\n\
video sink max lateness=20000000\n\
\n\
\n\
\n\
[pipeline]\n\
tsdemux element = ffdemux_mpegts\n\
\n\
audio hdcp element = drm_hdcp\n\
\n\
aac parser element =\n\
\n\
aac decoder element = omx_tz_aacdec\n\
\n\
ac3 parser element = \n\
\n\
ac3 decoder element = ;omx_tz_ac3dec\n\
\n\
lpcm converter element = omx_tz_lpcmdec\n\
\n\
lpcm filter element = ;capsfilter\n\
\n\
audio resampler element =\n\
\n\
audio volume element =\n\
\n\
audio sink element = alsasink\n\
\n\
video hdcp element = drm_hdcp\n\
\n\
video parser element =\n\
\n\
video decoder element = omx_tz_h264dec\n\
\n\
video sink element = xvimagesink\n\
\n\
\n\
\n\
[audio param]\n\
; 0x1: LPCM, 0x2: aac, 0x4: ac3\n\
;default aac and LPCM\n\
audio codec=0x3\n\
\n\
audio latency=0x0\n\
\n\
;0x1 : 48000khz, 0x2: 44100khz\n\
audio sampling frequency=0x3\n\
\n\
; 0x1:2 channels, 0x2:4 channels, 0x4:6channels, 0x8:8channels\n\
audio channels=0x1\n\
\n\
\n\
\n\
[video param]\n\
; 0: H264CBP 1: H264CHP\n\
video codec=0x1\n\
\n\
video native resolution = 0x20\n\
\n\
video cea support=0x194ab\n\
\n\
video vesa support=0x15555555\n\
\n\
video hh support=0x555\n\
\n\
; 0x1:base, 0x2:high\n\
video profile=0x1\n\
\n\
; 0x1:level_3_1, 0x2:level_3_2, 0x4:level_4, 0x8:level_4_1, 0x10:level_4_2\n\
video level=0x2\n\
\n\
video latency=0x0\n\
\n\
video vertical resolution=1200\n\
\n\
video horizontal resolution=1920\n\
\n\
video minimum slicesize=0\n\
\n\
video slice encoding params=200\n\
\n\
video framerate control support=11\n\
\n\
\n\
\n\
[hdcp param]\n\
;0x0:none, 0x1:HDCP_2.0, 0x2:HDCP_2.1\n\
hdcp content protection=0x0\n\
\n\
hdcp port no=0\n\
\n\
"

int
mm_wfd_sink_ini_load(mm_wfd_sink_ini_t *ini);

int
mm_wfd_sink_ini_unload(mm_wfd_sink_ini_t *ini);

#ifdef __cplusplus
  }
#endif

#endif
