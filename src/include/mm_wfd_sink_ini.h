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

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE : Wi-Fi Display Sink has no initalizing API for library itself
 * so we cannot decide when those ini values to be released.
 * this is the reason of all string items are static array.
 * make it do with malloc when MMWFDSinkCreate() API created
 * before that time, we should be careful with size limitation
 * of each string item.
 */

#define MM_WFD_SINK_INI_DEFAULT_PATH	SYSCONFDIR"/multimedia/mmfw_wfd_sink.ini"

#define WFD_SINK_INI_MAX_STRLEN	256
#define WFD_SINK_INI_MAX_ELEMENT	10


typedef struct {
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
	gint video_non_transcoding_support;
} WFDVideoFormats;

typedef struct {
	guint audio_codec;
	guint audio_latency;
	guint audio_channel;
	guint audio_sampling_frequency;
} WFDAudioCodecs;

typedef struct {
	gboolean enable_hdcp;
	gint hdcp_content_protection;
	gint hdcp_port_no;
} WFDHDCPContentProtection;

typedef struct __mm_wfd_sink_ini {
	/* general */
	gchar gst_param[5][WFD_SINK_INI_MAX_STRLEN];
	gint state_change_timeout;
	gboolean enable_rm;
	gint jitter_buffer_latency;
	gint video_sink_max_lateness;
	gint sink_ts_offset;
	gboolean audio_sink_async;
	gboolean video_sink_async;
	gboolean enable_retransmission;
	gboolean enable_reset_basetime;
	gchar user_agent[WFD_SINK_INI_MAX_STRLEN];

	/* debug */
	gboolean generate_dot;
	gboolean dump_rtsp_message;
	gboolean trace_buffers;
	gboolean trace_first_buffer;
	gboolean trace_buffers_of_wfdsrc;
	gboolean dump_ts_data;
	gboolean dump_rtp_data;

	/* pipeline */
	gchar name_of_source[WFD_SINK_INI_MAX_STRLEN];
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
	gchar name_of_video_h264_parser[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_capssetter[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_h264_decoder[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_converter[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_filter[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_sink[WFD_SINK_INI_MAX_STRLEN];
	gchar name_of_video_evas_sink[WFD_SINK_INI_MAX_STRLEN];

	/* audio parameter for reponse of M3 request */
	WFDAudioCodecs wfd_audio_codecs;

	/* video parameter for reponse of M3 request */
	WFDVideoFormats wfd_video_formats;

	/* hdcp parameter for reponse of M3 request */
	WFDHDCPContentProtection wfd_content_protection;
} mm_wfd_sink_ini_t;

#define MM_WFD_SINK_DEFAULT_INI \
" \
[general]\n\
; parameters for initializing gstreamer\n\
; DEFAULT SET(--gst-debug=2, *wfd*:5)\n\
gstparam1 = --gst-debug=2, *wfd*:5, *wfdtsdemux:1, *wfdrtpbuffer:1\n\
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
; enable wfdsrc inner pad probe\n\
enable wfdsrc pad probe = no\n\
\n\
; enable ts data dump(eg. /var/tmp/*.ts)\n\
enable ts data dump = no\n\
\n\
; allowed timeout for changing pipeline state\n\
state change timeout = 5 ; sec\n\
\n\
; set debug property to wfdsrc plugin for debugging rtsp message\n\
set debug property = yes\n\
\n\
; for asm function enable = yes, disable = no\n\
enable rm = no\n\
\n\
; 0: default value set by wfdsrc element, other: user define value.\n\
jitter buffer latency=10\n\
\n\
; for retransmission request enable = yes, disable = no\n\
enable retransmission = no\n\
\n\
; for reset basetime, enable = yes, disable = no\n\
enable reset basetime = yes\n\
\n\
; Maximum number of nanoseconds that a buffer can be late before it is dropped by videosink(-1 unlimited)\n\
video sink max lateness=20000000\n\
\n\
; nanoseconds to be added to buffertimestamp by sink elements\n\
sink ts offset=150000000\n\
\n\
; if no, go asynchronously to PAUSED without preroll \n\
audio sink async=no\n\
\n\
; if no, go asynchronously to PAUSED without preroll \n\
video sink async=no\n\
\n\
; user agent\n\
user agent = TIZEN3_0/WFD-SINK\n\
\n\
\n\
\n\
\n\
[pipeline]\n\
wfdsrc element = wfdsrc\n\
\n\
tsdemux element = wfdtsdemux\n\
\n\
audio hdcp element = \n\
\n\
aac parser element = aacparse\n\
\n\
aac decoder element = avdec_aac\n\
\n\
ac3 parser element = ac3parse\n\
\n\
ac3 decoder element =\n\
\n\
lpcm converter element =\n\
\n\
lpcm filter element = capsfilter\n\
\n\
audio resampler element = audioconvert\n\
\n\
audio volume element =\n\
\n\
audio sink element = pulsesink\n\
\n\
video hdcp element =\n\
\n\
video h264 parser element = h264parse\n\
\n\
video capssetter element = capssetter\n\
\n\
video h264_decoder element = avdec_h264;sprddec_h264;omxh264dec\n\
\n\
video converter element =\n\
\n\
video filter element =\n\
\n\
video sink element = waylandsink;xvimagesink\n\
\n\
\n\
\n\
[wfd audio codecs]\n\
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
[wfd video formats]\n\
; 0: H264CBP 1: H264CHP\n\
video codec=0x1\n\
\n\
video native resolution = 0x20\n\
\n\
video cea support=0x842b\n\
\n\
video vesa support=0x1\n\
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
video vertical resolution=720\n\
\n\
video horizontal resolution=1280\n\
\n\
video minimum slicesize=0\n\
\n\
video slice encoding params=200\n\
\n\
video framerate control support=11\n\
\n\
\n\
\n\
[wfd hdcp content protection]\n\
;0x0:none, 0x1:HDCP_2.0, 0x2:HDCP_2.1\n\
hdcp content protection=0x0\n\
\n\
hdcp port no=0\n\
\n\
"

int
mm_wfd_sink_ini_load(mm_wfd_sink_ini_t *ini, const char *path);

int
mm_wfd_sink_ini_unload(mm_wfd_sink_ini_t *ini);

#ifdef __cplusplus
}
#endif

#endif
