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

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <iniparser.h>
#include <mm_error.h>
#include "mm_wfd_sink_ini.h"
#include "mm_wfd_sink_dlog.h"

static gboolean loaded = FALSE;

/* global variables here */
#ifdef MM_WFD_SINK_DEFAULT_INI
static gboolean	__generate_sink_default_ini(void);
#endif

static void __mm_wfd_sink_ini_check_status(void);

/* macro */
#define MM_WFD_SINK_INI_GET_STRING( x_dict, x_item, x_ini, x_default ) \
do \
{ \
	gchar* str = NULL; \
	gint length = 0; \
 \
	str = iniparser_getstring(x_dict, x_ini, x_default); \
	if ( str ) \
	{ \
		length = strlen (str); \
		if ( ( length > 1 ) && ( length < WFD_SINK_INI_MAX_STRLEN ) ) \
			strncpy ( x_item, str, length+1 ); \
		else \
			strncpy ( x_item, x_default, WFD_SINK_INI_MAX_STRLEN-1 ); \
	} \
	else \
	{ \
		strncpy ( x_item, x_default, WFD_SINK_INI_MAX_STRLEN-1 ); \
	} \
}while(0)

#ifdef MM_WFD_SINK_DEFAULT_INI
static
gboolean __generate_sink_default_ini (void)
{
	FILE* fp = NULL;
	gchar* default_ini = MM_WFD_SINK_DEFAULT_INI;


	/* create new file */
	fp = fopen (MM_WFD_SINK_INI_DEFAULT_PATH, "wt");

	if ( !fp )
	{
		return FALSE;
	}

	/* writing default ini file */
	if ( strlen(default_ini) != fwrite(default_ini, 1, strlen(default_ini), fp) )
	{
		fclose(fp);
		return FALSE;
	}

	fclose(fp);
	return TRUE;
}
#endif

int
mm_wfd_sink_ini_load (mm_wfd_sink_ini_t* ini)
{
	dictionary * dict = NULL;

	wfd_sink_debug_fenter();


	__mm_wfd_sink_ini_check_status();

	/* first, try to load existing ini file */
	dict = iniparser_load (MM_WFD_SINK_INI_DEFAULT_PATH);

	/* if no file exists. create one with set of default values */
	if ( !dict )
	{
#ifdef MM_WFD_SINK_DEFAULT_INI
		wfd_sink_debug("No inifile found. create default ini file.\n");
		if ( FALSE == __generate_sink_default_ini() )
		{
			wfd_sink_error("Creating default ini file failed. Use default values.\n");
		}
		else
		{
			/* load default ini */
			dict = iniparser_load (MM_WFD_SINK_INI_DEFAULT_PATH);
		}
#else
		wfd_sink_error("No ini file found. \n");

		return MM_ERROR_FILE_NOT_FOUND;
#endif
	}

	/* get ini values */
	memset (ini, 0, sizeof(mm_wfd_sink_ini_t) );

	if ( dict ) /* if dict is available */
	{
		/* general */
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[0], "general:gstparam1", DEFAULT_GST_PARAM );
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[1], "general:gstparam2", DEFAULT_GST_PARAM );
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[2], "general:gstparam3", DEFAULT_GST_PARAM );
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[3], "general:gstparam4", DEFAULT_GST_PARAM );
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[4], "general:gstparam5", DEFAULT_GST_PARAM );
		ini->generate_dot = iniparser_getboolean(dict, "general:generate dot", DEFAULT_GENERATE_DOT);
		ini->enable_pad_probe = iniparser_getboolean(dict, "general:enable pad probe", DEFAULT_ENABLE_PAD_PROBE);
		ini->state_change_timeout = iniparser_getint(dict, "general:state change timeout", DEFAULT_STATE_CHANGE_TIMEOUT);
		ini->set_debug_property = iniparser_getboolean(dict, "general:set debug property", DEFAULT_SET_DEBUG_PROPERTY);
		ini->enable_asm = iniparser_getboolean(dict, "general:enable asm", DEFAULT_ENABLE_ASM);
		ini->jitter_buffer_latency = iniparser_getint(dict, "general:jitter buffer latency", DEFAULT_JITTER_BUFFER_LATENCY);
		ini->enable_retransmission = iniparser_getboolean(dict, "general:enable retransmission", DEFAULT_ENABLE_RETRANSMISSION);
		ini->enable_reset_basetime = iniparser_getboolean(dict, "general:enable reset basetime", DEFAULT_ENABLE_RESET_BASETIME);
		ini->video_sink_max_lateness = iniparser_getint(dict, "general:video sink max lateness", DEFAULT_VIDEO_SINK_MAX_LATENESS);
		ini->sink_ts_offset = iniparser_getint(dict, "general:sink ts offset", DEFAULT_SINK_TS_OFFSET);
		ini->audio_sink_async = iniparser_getboolean(dict, "general:audio sink async", DEFAULT_AUDIO_SINK_ASYNC);
		ini->video_sink_async = iniparser_getboolean(dict, "general:video sink async", DEFAULT_VIDEO_SINK_ASYNC);
		ini->enable_ts_data_dump = iniparser_getboolean(dict, "general:enable ts data dump", DEFAULT_ENABLE_TS_DATA_DUMP);
		ini->enable_wfdrtspsrc_pad_probe = iniparser_getboolean(dict, "general:enable wfdrtspsrc pad probe", DEFAULT_ENABLE_WFDRTSPSRC_PAD_PROBE);


		/* pipeline */
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_tsdemux, "pipeline:tsdemux element", DEFAULT_NAME_OF_TSDEMUX );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_audio_hdcp, "pipeline:audio hdcp element", DEFAULT_NAME_OF_AUDIO_HDCP );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_aac_parser, "pipeline:aac parser element", DEFAULT_NAME_OF_AAC_PARSER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_aac_decoder, "pipeline:aac decoder element", DEFAULT_NAME_OF_AAC_DECODER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_ac3_parser, "pipeline:ac3 parser element", DEFAULT_NAME_OF_AC3_PARSER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_ac3_decoder, "pipeline:ac3 decoder element", DEFAULT_NAME_OF_AC3_DECODER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_lpcm_converter, "pipeline:lpcm converter element", DEFAULT_NAME_OF_LPCM_CONVERTER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_lpcm_filter, "pipeline:lpcm filter element", DEFAULT_NAME_OF_LPCM_FILTER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_audio_resampler, "pipeline:audio resampler element", DEFAULT_NAME_OF_AUDIO_RESAMPLER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_audio_volume, "pipeline:audio volume element", DEFAULT_NAME_OF_AUDIO_VOLUME );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_audio_sink, "pipeline:audio sink element", DEFAULT_NAME_OF_AUDIO_SINK );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_video_hdcp, "pipeline:video hdcp element", DEFAULT_NAME_OF_VIDEO_HDCP );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_video_parser, "pipeline:video parser element", DEFAULT_NAME_OF_VIDEO_PARSER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_video_decoder, "pipeline:video decoder element", DEFAULT_NAME_OF_VIDEO_DECODER );
		MM_WFD_SINK_INI_GET_STRING( dict, ini->name_of_video_sink, "pipeline:video sink element", DEFAULT_NAME_OF_VIDEO_SINK );

		/* audio parameter*/
		ini->audio_codec = iniparser_getint(dict, "audio param:audio codec", DEFAULT_AUDIO_CODEC);
		ini->audio_latency = iniparser_getint(dict, "audio param:audio latency", DEFAULT_AUDIO_LATENCY);
		ini->audio_channel = iniparser_getint(dict, "audio param:audio channels", DEFAULT_AUDIO_CHANNELS);
		ini->audio_sampling_frequency = iniparser_getint(dict, "audio param:audio sampling frequency", DEFAULT_AUDIO_SAMP_FREQUENCY);

		/* video parameter*/
		ini->video_codec = iniparser_getint(dict, "video param:video codec", DEFAULT_VIDEO_CODEC);
		ini->video_native_resolution = iniparser_getint(dict, "video param:video native resolution", DEFAULT_VIDEO_NATIVE_RESOLUTION);
		ini->video_cea_support = iniparser_getint(dict, "video param:video cea support", DEFAULT_VIDEO_CEA_SUPPORT);
		ini->video_vesa_support = iniparser_getint(dict, "video param:video vesa support", DEFAULT_VIDEO_VESA_SUPPORT);
		ini->video_hh_support = iniparser_getint(dict, "video param:video hh support", DEFAULT_VIDEO_HH_SUPPORT);
		ini->video_profile = iniparser_getint(dict, "video param:video profile", DEFAULT_VIDEO_PROFILE);
		ini->video_level = iniparser_getint(dict, "video param:video level", DEFAULT_VIDEO_LEVEL);
		ini->video_latency = iniparser_getint(dict, "video param:video latency", DEFAULT_VIDEO_LATENCY);
		ini->video_vertical_resolution = iniparser_getint(dict, "video param:video vertical resolution", DEFAULT_VIDEO_VERTICAL_RESOLUTION);
		ini->video_horizontal_resolution = iniparser_getint(dict, "video param:video horizontal resolution", DEFAULT_VIDEO_HORIZONTAL_RESOLUTION);
		ini->video_minimum_slicing = iniparser_getint(dict, "video param:video minimum slicesize", DEFAULT_VIDEO_MIN_SLICESIZE);
		ini->video_slice_enc_param = iniparser_getint(dict, "video param:video slice encoding params", DEFAULT_VIDEO_SLICE_ENC_PARAM);
		ini->video_framerate_control_support = iniparser_getint(dict, "video param:video framerate control support", DEFAULT_VIDEO_FRAMERATE_CONTROL);

		/* hdcp parameter*/
		ini->hdcp_content_protection = iniparser_getint(dict, "hdcp param:hdcp content protection", DEFAULT_HDCP_CONTENT_PROTECTION);
		ini->hdcp_port_no = iniparser_getint(dict, "hdcp param:hdcp port no", DEFAULT_HDCP_PORT_NO);
	}
	else /* if dict is not available just fill the structure with default value */
	{
		wfd_sink_error("failed to load ini. using hardcoded default\n");

		/* general */
		strncpy( ini->gst_param[0], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->gst_param[1], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->gst_param[2], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->gst_param[3], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->gst_param[4], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1 );
		ini->generate_dot =  DEFAULT_GENERATE_DOT;
		ini->enable_pad_probe = DEFAULT_ENABLE_PAD_PROBE;
		ini->state_change_timeout = DEFAULT_STATE_CHANGE_TIMEOUT;
		ini->set_debug_property =  DEFAULT_SET_DEBUG_PROPERTY;
		ini->enable_asm =  DEFAULT_ENABLE_ASM;
		ini->jitter_buffer_latency = DEFAULT_JITTER_BUFFER_LATENCY;
		ini->enable_retransmission =  DEFAULT_ENABLE_RETRANSMISSION;
		ini->enable_reset_basetime =  DEFAULT_ENABLE_RESET_BASETIME;
		ini->video_sink_max_lateness = DEFAULT_VIDEO_SINK_MAX_LATENESS;
		ini->sink_ts_offset = DEFAULT_SINK_TS_OFFSET;
		ini->enable_ts_data_dump = DEFAULT_ENABLE_TS_DATA_DUMP;
		ini->enable_wfdrtspsrc_pad_probe = DEFAULT_ENABLE_WFDRTSPSRC_PAD_PROBE;

		/* pipeline */
		strncpy( ini->name_of_tsdemux, DEFAULT_NAME_OF_TSDEMUX, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_audio_hdcp, DEFAULT_NAME_OF_AUDIO_HDCP, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_aac_parser, DEFAULT_NAME_OF_AAC_PARSER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_aac_decoder, DEFAULT_NAME_OF_AAC_DECODER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_ac3_parser, DEFAULT_NAME_OF_AC3_PARSER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_ac3_decoder, DEFAULT_NAME_OF_AC3_DECODER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_lpcm_converter, DEFAULT_NAME_OF_LPCM_CONVERTER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_lpcm_filter, DEFAULT_NAME_OF_LPCM_FILTER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_audio_resampler, DEFAULT_NAME_OF_AUDIO_RESAMPLER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_audio_volume, DEFAULT_NAME_OF_AUDIO_VOLUME, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_audio_sink, DEFAULT_NAME_OF_AUDIO_SINK, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_video_hdcp, DEFAULT_NAME_OF_VIDEO_HDCP, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_video_parser, DEFAULT_NAME_OF_VIDEO_PARSER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_video_decoder, DEFAULT_NAME_OF_VIDEO_DECODER, WFD_SINK_INI_MAX_STRLEN - 1 );
		strncpy( ini->name_of_video_sink, DEFAULT_NAME_OF_VIDEO_SINK, WFD_SINK_INI_MAX_STRLEN - 1 );

		/* audio parameter*/
		ini->audio_codec = DEFAULT_AUDIO_CODEC;
		ini->audio_latency = DEFAULT_AUDIO_LATENCY;
		ini->audio_channel = DEFAULT_AUDIO_CHANNELS;
		ini->audio_sampling_frequency = DEFAULT_AUDIO_SAMP_FREQUENCY;

		/* video parameter*/
		ini->video_codec = DEFAULT_VIDEO_CODEC;
		ini->video_native_resolution = DEFAULT_VIDEO_NATIVE_RESOLUTION;
		ini->video_cea_support = DEFAULT_VIDEO_CEA_SUPPORT;
		ini->video_vesa_support = DEFAULT_VIDEO_VESA_SUPPORT;
		ini->video_hh_support = DEFAULT_VIDEO_HH_SUPPORT;
		ini->video_profile = DEFAULT_VIDEO_PROFILE;
		ini->video_level = DEFAULT_VIDEO_LEVEL;
		ini->video_latency = DEFAULT_VIDEO_LATENCY;
		ini->video_vertical_resolution = DEFAULT_VIDEO_VERTICAL_RESOLUTION;
		ini->video_horizontal_resolution = DEFAULT_VIDEO_HORIZONTAL_RESOLUTION;
		ini->video_minimum_slicing = DEFAULT_VIDEO_MIN_SLICESIZE;
		ini->video_slice_enc_param = DEFAULT_VIDEO_SLICE_ENC_PARAM;
		ini->video_framerate_control_support = DEFAULT_VIDEO_FRAMERATE_CONTROL;

		/* hdcp parameter*/
		ini->hdcp_content_protection = DEFAULT_HDCP_CONTENT_PROTECTION;
		ini->hdcp_port_no = DEFAULT_HDCP_PORT_NO;
	}

	/* free dict as we got our own structure */
	iniparser_freedict (dict);


	/* dump structure */
	wfd_sink_debug("W-Fi Display Sink Initial Settings -----------------------------------\n");

	/* general */
	wfd_sink_debug("gst_param1 : %s\n", ini->gst_param[0]);
	wfd_sink_debug("gst_param2 : %s\n", ini->gst_param[1]);
	wfd_sink_debug("gst_param3 : %s\n", ini->gst_param[2]);
	wfd_sink_debug("gst_param4 : %s\n", ini->gst_param[3]);
	wfd_sink_debug("gst_param5 : %s\n", ini->gst_param[4]);
	wfd_sink_debug("generate_dot : %d\n", ini->generate_dot);
	if (ini->generate_dot == TRUE)
	{
		wfd_sink_debug("generate_dot is TRUE, dot file will be stored into /tmp/\n");
		g_setenv ("GST_DEBUG_DUMP_DOT_DIR", "/tmp/", FALSE);
	}
	wfd_sink_debug("enable_pad_probe : %d\n", ini->enable_pad_probe);
	wfd_sink_debug("state_change_timeout(sec) : %d\n", ini->state_change_timeout);
	wfd_sink_debug("set_debug_property : %d\n", ini->set_debug_property);
	wfd_sink_debug("enable_asm : %d\n", ini->enable_asm);
	wfd_sink_debug("jitter_buffer_latency(msec) : %d\n", ini->jitter_buffer_latency);
	wfd_sink_debug("enable_retransmission : %d\n", ini->enable_retransmission);
	wfd_sink_debug("enable_reset_basetime : %d\n", ini->enable_reset_basetime);
	wfd_sink_debug("video_sink_max_lateness(nsec) : %d\n", ini->video_sink_max_lateness);
	wfd_sink_debug("sink_ts_offset(nsec) : %d\n", ini->sink_ts_offset);
	wfd_sink_debug("audio_sink_async : %d\n", ini->audio_sink_async);
	wfd_sink_debug("video_sink_async : %d\n", ini->video_sink_async);
	wfd_sink_debug("enable_ts_data_dump : %d\n", ini->enable_ts_data_dump);
	wfd_sink_debug("enable_wfdrtspsrc_pad_probe : %d\n", ini->enable_wfdrtspsrc_pad_probe);

	/* pipeline */
	wfd_sink_debug("name_of_tsdemux : %s\n", ini->name_of_tsdemux);
	wfd_sink_debug("name_of_audio_hdcp : %s\n", ini->name_of_audio_hdcp);
	wfd_sink_debug("name_of_aac_parser : %s\n", ini->name_of_aac_parser);
	wfd_sink_debug("name_of_aac_decoder : %s\n", ini->name_of_aac_decoder);
	wfd_sink_debug("name_of_ac3_parser : %s\n", ini->name_of_ac3_parser);
	wfd_sink_debug("name_of_ac3_decoder : %s\n", ini->name_of_ac3_decoder);
	wfd_sink_debug("name_of_lpcm_converter : %s\n", ini->name_of_lpcm_converter);
	wfd_sink_debug("name_of_lpcm_filter : %s\n", ini->name_of_lpcm_filter);
	wfd_sink_debug("name_of_audio_resampler : %s\n", ini->name_of_audio_resampler);
	wfd_sink_debug("name_of_audio_volume : %s\n", ini->name_of_audio_volume);
#ifdef ENABLE_WFD_VD_FEATURES
	wfd_sink_debug("name_of_audio_splitter : %s\n", ini->name_of_audio_splitter);
#endif
	wfd_sink_debug("name_of_audio_sink : %s\n", ini->name_of_audio_sink);
	wfd_sink_debug("name_of_video_hdcp : %s\n", ini->name_of_video_hdcp);
	wfd_sink_debug("name_of_video_parser : %s\n", ini->name_of_video_parser);
	wfd_sink_debug("name_of_video_decoder : %s\n", ini->name_of_video_decoder);
	wfd_sink_debug("name_of_video_sink : %s\n", ini->name_of_video_sink);

	/* audio parameter*/
	wfd_sink_debug("audio_codec : %x\n", ini->audio_codec);
	wfd_sink_debug("audio_latency : %d\n", ini->audio_latency);
	wfd_sink_debug("audio_channel : %x\n", ini->audio_channel);
	wfd_sink_debug("audio_sampling_frequency : %x\n", ini->audio_sampling_frequency);

	/* video parameter*/
	wfd_sink_debug("video_codec : %x\n", ini->video_codec);
	wfd_sink_debug("video_native_resolution : %x\n", ini->video_native_resolution);
	wfd_sink_debug("video_cea_support : %x\n", ini->video_cea_support);
	wfd_sink_debug("video_vesa_support : %x\n", ini->video_vesa_support);
	wfd_sink_debug("video_hh_support : %x\n", ini->video_hh_support);
	wfd_sink_debug("video_profile : %x\n", ini->video_profile);
	wfd_sink_debug("video_level : %x\n", ini->video_level);
	wfd_sink_debug("video_latency : %d\n", ini->video_latency);
	wfd_sink_debug("video_vertical_resolution : %d\n", ini->video_vertical_resolution);
	wfd_sink_debug("video_horizontal_resolution : %d\n", ini->video_horizontal_resolution);
	wfd_sink_debug("video_minimum_slicing : %d\n", ini->video_minimum_slicing);
	wfd_sink_debug("video_slice_enc_param : %d\n", ini->video_slice_enc_param);
	wfd_sink_debug("video_framerate_control_support : %d\n", ini->video_framerate_control_support);

	/* hdcp parameter*/
	wfd_sink_debug("hdcp_content_protection : %x\n", ini->hdcp_content_protection);
	wfd_sink_debug("hdcp_port_no : %d\n", ini->hdcp_port_no);

	wfd_sink_debug("---------------------------------------------------\n");

	loaded = TRUE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}


static
void __mm_wfd_sink_ini_check_status (void)
{
	struct stat ini_buff;

	wfd_sink_debug_fenter();

	if ( g_stat(MM_WFD_SINK_INI_DEFAULT_PATH, &ini_buff) < 0 )
	{
		wfd_sink_error("failed to get mmfw_wfd_sink ini status\n");
	}
	else
	{
		if ( ini_buff.st_size < 5 )
		{
			wfd_sink_error("mmfw_wfd_sink.ini file size=%d, Corrupted! So, Removed\n", (int)ini_buff.st_size);
			g_remove( MM_WFD_SINK_INI_DEFAULT_PATH );
		}
	}

	wfd_sink_debug_fleave();
}

int
mm_wfd_sink_ini_unload (mm_wfd_sink_ini_t* ini)
{
	wfd_sink_debug_fenter();

	loaded = FALSE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}
