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

#ifndef __MM_WFD_INI_C__
#define __MM_WFD_INI_C__ // TODO: why macro is needed ??

#include <glib.h>
#include <stdlib.h>
#include "iniparser.h"
#include <mm_wfd_ini.h>
#include "mm_debug.h"
#include <mm_error.h>
#include <glib/gstdio.h>

/* global variables here */
static mm_wfd_ini_t g_wfd_ini;
static void __mm_wfd_ini_check_ini_status(void);
static void	__get_string_list(gchar** out_list, gchar* str);

/* macro */
#define MMWFD_INI_GET_STRING( x_item, x_ini, x_default ) \
do \
{ \
  gchar* str = iniparser_getstring(dict, x_ini, x_default); \
 \
  if ( str &&  \
    ( strlen( str ) > 1 ) && \
    ( strlen( str ) < WFD_INI_MAX_STRLEN ) ) \
  { \
    strcpy ( x_item, str ); \
  } \
  else \
  { \
    strcpy ( x_item, x_default ); \
  } \
}while(0)

int
mm_wfd_ini_load(void)
{
  static gboolean loaded = FALSE;
  dictionary * dict = NULL;
  gint idx = 0;

  if ( loaded )
  return MM_ERROR_NONE;

  dict = NULL;

  /* disabling ini parsing for launching */
  #if 1 //debianize
  /* get wfd ini status because system will be crashed
  * if ini file is corrupted.
  */
  /* FIXIT : the api actually deleting illregular ini. but the function name said it's just checking. */
  __mm_wfd_ini_check_ini_status();

  /* first, try to load existing ini file */
  dict = iniparser_load(MM_WFD_INI_DEFAULT_PATH);

  /* if no file exists. create one with set of default values */
  if ( !dict )
  {
#if 0
    debug_log("No inifile found. wfd will create default inifile.\n");
    if ( FALSE == __generate_default_ini() )
    {
      debug_warning("Creating default inifile failed. Player will use default values.\n");
    }
    else
    {
      /* load default ini */
      dict = iniparser_load(MM_WFD_INI_DEFAULT_PATH);
    }
#else
    debug_log("No inifile found. \n");

    return MM_ERROR_FILE_NOT_FOUND;
#endif
  }
#endif

  /* get ini values */
  memset( &g_wfd_ini, 0, sizeof(mm_wfd_ini_t) );

  if ( dict ) /* if dict is available */
  {
    /* general */
    g_wfd_ini.videosrc_element = iniparser_getint(dict, "general:videosrc element", DEFAULT_VIDEOSRC);
    g_wfd_ini.session_mode = iniparser_getint(dict, "general:session_mode", DEFAULT_SESSION_MODE);
    g_wfd_ini.videosink_element = iniparser_getint(dict, "general:videosink element", DEFAULT_VIDEOSINK);
    g_wfd_ini.disable_segtrap = iniparser_getboolean(dict, "general:disable segtrap", DEFAULT_DISABLE_SEGTRAP);
    g_wfd_ini.skip_rescan = iniparser_getboolean(dict, "general:skip rescan", DEFAULT_SKIP_RESCAN);
    g_wfd_ini.videosink_element = iniparser_getint(dict, "general:videosink element", DEFAULT_VIDEOSINK);
    g_wfd_ini.videobitrate = iniparser_getint(dict, "general:videobitrate value", DEFAULT_VIDEO_BITRATE);
    g_wfd_ini.generate_dot = iniparser_getboolean(dict, "general:generate dot", DEFAULT_GENERATE_DOT);
    g_wfd_ini.provide_clock= iniparser_getboolean(dict, "general:provide clock", DEFAULT_PROVIDE_CLOCK);
    g_wfd_ini.live_state_change_timeout = iniparser_getint(dict, "general:live state change timeout", DEFAULT_LIVE_STATE_CHANGE_TIMEOUT);
    g_wfd_ini.localplayback_state_change_timeout = iniparser_getint(dict, "general:localplayback state change timeout", DEFAULT_LOCALPLAYBACK_STATE_CHANGE_TIMEOUT);
    g_wfd_ini.eos_delay = iniparser_getint(dict, "general:eos delay", DEFAULT_EOS_DELAY);
    g_wfd_ini.async_start = iniparser_getboolean(dict, "general:async start", DEFAULT_ASYNC_START);

    g_wfd_ini.delay_before_repeat = iniparser_getint(dict, "general:delay before repeat", DEFAULT_DELAY_BEFORE_REPEAT);

    MMWFD_INI_GET_STRING( g_wfd_ini.name_of_video_converter, "general:video converter element", DEFAULT_VIDEO_CONVERTER );

    __get_string_list( (gchar**) g_wfd_ini.exclude_element_keyword,
      iniparser_getstring(dict, "general:element exclude keyword", DEFAULT_EXCLUDE_KEYWORD));

    MMWFD_INI_GET_STRING( g_wfd_ini.gst_param[0], "general:gstparam1", DEFAULT_GST_PARAM );
    MMWFD_INI_GET_STRING( g_wfd_ini.gst_param[1], "general:gstparam2", DEFAULT_GST_PARAM );
    MMWFD_INI_GET_STRING( g_wfd_ini.gst_param[2], "general:gstparam3", DEFAULT_GST_PARAM );
    MMWFD_INI_GET_STRING( g_wfd_ini.gst_param[3], "general:gstparam4", DEFAULT_GST_PARAM );
    MMWFD_INI_GET_STRING( g_wfd_ini.gst_param[4], "general:gstparam5", DEFAULT_GST_PARAM );

    MMWFD_INI_GET_STRING( g_wfd_ini.infile, "general:input file", DEFAULT_INPUT_FILE );

    /* hw accelation */
    g_wfd_ini.use_video_hw_accel = iniparser_getboolean(dict, "hw accelation:use video hw accel", DEFAULT_USE_VIDEO_HW_ACCEL);
  }
  else /* if dict is not available just fill the structure with default value */
  {
    debug_warning("failed to load ini. using hardcoded default\n");

    /* general */
    g_wfd_ini.videosrc_element = DEFAULT_VIDEOSRC;
    g_wfd_ini.session_mode = DEFAULT_SESSION_MODE;
    g_wfd_ini.disable_segtrap = DEFAULT_DISABLE_SEGTRAP;
    g_wfd_ini.skip_rescan = DEFAULT_SKIP_RESCAN;
    g_wfd_ini.videosink_element = DEFAULT_VIDEOSINK;
    g_wfd_ini.videobitrate = DEFAULT_VIDEO_BITRATE;
    g_wfd_ini.generate_dot = DEFAULT_GENERATE_DOT;
    g_wfd_ini.provide_clock= DEFAULT_PROVIDE_CLOCK;
    g_wfd_ini.live_state_change_timeout = DEFAULT_LIVE_STATE_CHANGE_TIMEOUT;
    g_wfd_ini.localplayback_state_change_timeout = DEFAULT_LOCALPLAYBACK_STATE_CHANGE_TIMEOUT;
    g_wfd_ini.eos_delay = DEFAULT_EOS_DELAY;
    g_wfd_ini.async_start = DEFAULT_ASYNC_START;
    g_wfd_ini.delay_before_repeat = DEFAULT_DELAY_BEFORE_REPEAT;

    strcpy( g_wfd_ini.name_of_video_converter, DEFAULT_VIDEO_CONVERTER);

    {
      __get_string_list( (gchar**) g_wfd_ini.exclude_element_keyword, DEFAULT_EXCLUDE_KEYWORD);
    }


    strcpy( g_wfd_ini.gst_param[0], DEFAULT_GST_PARAM );
    strcpy( g_wfd_ini.gst_param[1], DEFAULT_GST_PARAM );
    strcpy( g_wfd_ini.gst_param[2], DEFAULT_GST_PARAM );
    strcpy( g_wfd_ini.gst_param[3], DEFAULT_GST_PARAM );
    strcpy( g_wfd_ini.gst_param[4], DEFAULT_GST_PARAM );

    strcpy( g_wfd_ini.infile, DEFAULT_INPUT_FILE );

    /* hw accelation */
    g_wfd_ini.use_video_hw_accel = DEFAULT_USE_VIDEO_HW_ACCEL;

  }

  /* free dict as we got our own structure */
  iniparser_freedict (dict);

  loaded = TRUE;

  /* The simulator uses a separate ini file. */
  //__mm_wfd_ini_force_setting();


  /* dump structure */
  debug_log("wfd settings -----------------------------------\n");

  /* general */
  debug_log("videosrc element : %d\n", g_wfd_ini.videosrc_element);
  debug_log("session mode in mirroring : %d\n", g_wfd_ini.session_mode);
  debug_log("disable_segtrap : %d\n", g_wfd_ini.disable_segtrap);
  debug_log("skip rescan : %d\n", g_wfd_ini.skip_rescan);
  debug_log("videosink element(0:v4l2sink, 1:ximagesink, 2:xvimagesink, 3:fakesink) : %d\n", g_wfd_ini.videosink_element);
    debug_log("video_bitrate : %d\n", g_wfd_ini.videobitrate);
  debug_log("generate_dot : %d\n", g_wfd_ini.generate_dot);
  debug_log("provide_clock : %d\n", g_wfd_ini.provide_clock);
  debug_log("live_state_change_timeout(sec) : %d\n", g_wfd_ini.live_state_change_timeout);
  debug_log("localplayback_state_change_timeout(sec) : %d\n", g_wfd_ini.localplayback_state_change_timeout);
  debug_log("eos_delay(msec) : %d\n", g_wfd_ini.eos_delay);
  debug_log("delay_before_repeat(msec) : %d\n", g_wfd_ini.delay_before_repeat);
  debug_log("name_of_video_converter : %s\n", g_wfd_ini.name_of_video_converter);
  debug_log("async_start : %d\n", g_wfd_ini.async_start);

  debug_log("gst_param1 : %s\n", g_wfd_ini.gst_param[0]);
  debug_log("gst_param2 : %s\n", g_wfd_ini.gst_param[1]);
  debug_log("gst_param3 : %s\n", g_wfd_ini.gst_param[2]);
  debug_log("gst_param4 : %s\n", g_wfd_ini.gst_param[3]);
  debug_log("gst_param5 : %s\n", g_wfd_ini.gst_param[4]);

  debug_log("input file : %s\n", g_wfd_ini.infile);

  for ( idx = 0; g_wfd_ini.exclude_element_keyword[idx][0] != '\0'; idx++ )
  {
  	debug_log("exclude_element_keyword [%d] : %s\n", idx, g_wfd_ini.exclude_element_keyword[idx]);
  }

  /* hw accel */
  debug_log("use_video_hw_accel : %d\n", g_wfd_ini.use_video_hw_accel);
  
  debug_log("---------------------------------------------------\n");

  return MM_ERROR_NONE;
}


static
void __mm_wfd_ini_check_ini_status(void)
{
  struct stat ini_buff;
  
  if ( g_stat(MM_WFD_INI_DEFAULT_PATH, &ini_buff) < 0 )
  {
    debug_warning("failed to get wfd ini status\n");
  }
  else
  {
    if ( ini_buff.st_size < 5 )
    {
      debug_warning("wfd.ini file size=%d, Corrupted! So, Removed\n", (int)ini_buff.st_size);
      g_remove( MM_WFD_INI_DEFAULT_PATH );
    }
  }
}


mm_wfd_ini_t*
mm_wfd_ini_get_structure(void)
{
  return &g_wfd_ini;
}


static
void __get_string_list(gchar** out_list, gchar* str)
{
  gchar** list = NULL;
  gchar** walk = NULL;
  gint i = 0;
  gchar* strtmp = NULL;

  if ( ! str )
  return;

  if ( strlen( str ) < 1 )
    return;

  strtmp = g_strdup (str);

  /* trimming. it works inplace */
  g_strstrip( strtmp );

  /* split */
  list = g_strsplit( strtmp, ",", 10 );

  g_return_if_fail ( list != NULL );

  /* copy list */
  for( walk = list; *walk; walk++ )
  {
    strncpy( g_wfd_ini.exclude_element_keyword[i], *walk, (WFD_INI_MAX_STRLEN - 1) );
    g_strstrip( g_wfd_ini.exclude_element_keyword[i] );
    g_wfd_ini.exclude_element_keyword[i][WFD_INI_MAX_STRLEN - 1] = '\0';
    i++;
  }

  /* mark last item to NULL */
  g_wfd_ini.exclude_element_keyword[i][0] = '\0';

  g_strfreev( list );
  if (strtmp)
  g_free (strtmp);
}

#endif

