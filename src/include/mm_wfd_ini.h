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

#ifndef __MM_WFD_INI_H__
#define __MM_WFD_INI_H__

#include <glib.h>

#ifdef __cplusplus
  extern "C" {
#endif


#define MM_WFD_INI_DEFAULT_PATH	"/usr/etc/mmfw_wfd.ini"

#define WFD_INI() mm_wfd_ini_get_structure()

#define WFD_INI_MAX_STRLEN	80

typedef enum __wfd_ini_videosink_element
{
  WFD_INI_VSINK_V4l2SINK = 0,
  WFD_INI_VSINK_XIMAGESINK,
  WFD_INI_VSINK_XVIMAGESINK,
  WFD_INI_VSINK_FAKESINK,
  WFD_INI_VSINK_EVASIMAGESINK,
  WFD_INI_VSINK_GLIMAGESINK,
  WFD_INI_VSINK_NUM
}WFD_INI_VSINK_ELEMENT;

typedef enum __wfd_ini_videosrc_element
{
  WFD_INI_VSRC_XVIMAGESRC,
  WFD_INI_VSRC_FILESRC,
  WFD_INI_VSRC_CAMERASRC,
  WFD_INI_VSRC_VIDEOTESTSRC,
  WFD_INI_VSRC_NUM
}WFD_INI_VSRC_ELEMENT;

typedef enum __wfd_ini_session_mode
{
  WFD_INI_AUDIO_VIDEO_MUXED,
  WFD_INI_VIDEO_ONLY,
  WFD_INI_AUDIO_ONLY,
  WFD_INI_AUDIO_VIDEO_SAPERATE
}WFD_INI_SESSION_MODE;


/* NOTE : MMPlayer has no initalizing API for library itself
 * so we cannot decide when those ini values to be released.
 * this is the reason of all string items are static array.
 * make it do with malloc when MMPlayerInitialize() API created
 * before that time, we should be careful with size limitation
 * of each string item.
 */

/* @ mark means the item has tested */
typedef struct __mm_wfd_ini
{
  /* general */
  WFD_INI_VSRC_ELEMENT videosrc_element;
  gint session_mode;
  WFD_INI_VSINK_ELEMENT videosink_element; // @
  gchar name_of_video_converter[WFD_INI_MAX_STRLEN];
  gboolean skip_rescan; // @
  gboolean generate_dot; // @
  gboolean provide_clock; // @
  gint live_state_change_timeout; // @
  gint localplayback_state_change_timeout; // @
  gint delay_before_repeat;
  gint eos_delay; // @
  gint videobitrate;
  gint mtu_size;

  gchar gst_param[5][256]; // @
  gchar exclude_element_keyword[10][WFD_INI_MAX_STRLEN];
  gboolean async_start;
  gboolean disable_segtrap;
  gchar infile[256]; // @

  /* hw accelation */
  gboolean use_video_hw_accel; // @
} mm_wfd_ini_t;

/* default values if each values are not specified in inifile */
/* general */
#define DEFAULT_SKIP_RESCAN				TRUE
#define DEFAULT_GENERATE_DOT				FALSE
#define DEFAULT_PROVIDE_CLOCK				TRUE
#define DEFAULT_DELAY_BEFORE_REPEAT	 		50 /* msec */
#define DEFAULT_EOS_DELAY 				150 /* msec */
#define DEFAULT_VIDEOSINK				WFD_INI_VSINK_XVIMAGESINK
#define DEFAULT_VIDEOSRC				WFD_INI_VSRC_XVIMAGESRC
#define DEFAULT_VIDEO_BITRATE 				3072000 /* bps */
#define DEFAULT_MTU_SIZE        1400 /* bytes */
#define DEFAULT_SESSION_MODE				0
#define DEFAULT_GST_PARAM				""
#define DEFAULT_EXCLUDE_KEYWORD				""
#define DEFAULT_ASYNC_START				TRUE
#define DEFAULT_DISABLE_SEGTRAP				TRUE
#define DEFAULT_VIDEO_CONVERTER				""
#define DEFAULT_LIVE_STATE_CHANGE_TIMEOUT 		30 /* sec */
#define DEFAULT_LOCALPLAYBACK_STATE_CHANGE_TIMEOUT 	10 /* sec */
/* hw accel */
#define DEFAULT_USE_VIDEO_HW_ACCEL	FALSE
#define DEFAULT_INPUT_FILE ""

/* NOTE : following content should be same with above default values */
/* FIXIT : need smarter way to generate default ini file. */
/* FIXIT : finally, it should be an external file */
#define MM_WFD_DEFAULT_INI \
" \
[general] \n\
\n\
; set default video source element\n\
; 0: xvimagesrc, 1: filesrc, 2: camerasrc, 3: videotestsrc\n\
videosrc element = 0 \n\
\n\
; sending video only mirroring mode\n\
; 0: audio-video-muxed sending, 1:video-only, 2:audio-only, 3:audio-video-saperate\n\
session_mode = 0 \n\
disable segtrap = yes ; same effect with --gst-disable-segtrap \n\
\n\
; set default video sink when video is rendered on the WFD source as well\n\
; 0:v4l2sink, 1:ximagesink, 2:xvimagesink, 3:fakesink 4:evasimagesink 5:glimagesink\n\
videosink element = 2 \n\
\n\
videobitrate value = 6144000 \n\
\n\
mtu_size value = 1400 \n\
\n\
video converter element = \n\
\n\
; if yes. gstreamer will not update registry \n\
skip rescan = yes \n\
\n\
delay before repeat = 50 ; msec\n\
\n\
; comma separated list of tocken which elemnts has it in its name will not be used \n\
element exclude keyword = \n\
\n\
async start = yes \n\
\n\
; parameters for initializing gstreamer \n\
gstparam1 = \n\
gstparam2 = \n\
gstparam3 = \n\
gstparam4 = \n\
gstparam5 = \n\
\n\
; generating dot file representing pipeline state \n\
generate dot = no \n\
\n\
; parameter for clock provide in audiosink \n\
provide clock = yes \n\
\n\
; allowed timeout for changing pipeline state \n\
live state change timeout = 30 ; sec \n\
localplayback state change timeout = 4 ; sec \n\
\n\
; delay in msec for sending EOS \n\
eos delay = 150 ; msec \n\
\n\
\n\
[hw accelation] \n\
use video hw accel = yes \n\
\n\
\n\
\n\
"

int
mm_wfd_ini_load(void);

mm_wfd_ini_t*
mm_wfd_ini_get_structure(void);


#ifdef __cplusplus
  }
#endif

#endif

