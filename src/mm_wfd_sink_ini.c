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

#include "mm_wfd_sink_dlog.h"
#include "mm_wfd_sink_enum.h"
#include "mm_wfd_sink_ini.h"

/* Audio */
#define DEFAULT_WFD_AUDIO_CODECS_CODEC WFD_AUDIO_LPCM | WFD_AUDIO_AAC
#define DEFAULT_WFD_AUDIO_CODECS_LATENCY 0x0
#define DEFAULT_WFD_AUDIO_CODECS_CHANNELS WFD_CHANNEL_2
#define DEFAULT_WFD_AUDIO_CODECS_SAMP_FREQUENCY WFD_FREQ_44100 | WFD_FREQ_48000

/* Video */
#define DEFAULT_WFD_VIDEO_FORMATS_CODEC WFD_VIDEO_H264
#define DEFAULT_WFD_VIDEO_FORMATS_NATIVE_RESOLUTION 0x20
/* CEA :  WFD_CEA_640x480P60  | WFD_CEA_720x480P60 |WFD_CEA_720x576P50 |WFD_CEA_1280x720P30 |
	WFD_CEA_1280x720P25 | WFD_CEA_1280x720P24 */
#define DEFAULT_WFD_VIDEO_FORMATS_CEA_SUPPORT "0x84ab"
/* VESA : WFD_VESA_800x600P30 */
#define DEFAULT_WFD_VIDEO_FORMATS_VESA_SUPPORT "0x1"
/* HH : WFD_HH_800x480P30 | WFD_HH_854x480P30 | WFD_HH_864x480P30 | WFD_HH_640x360P30 | WFD_HH_960x540P30 | WFD_HH_848x480P30 */
#define DEFAULT_WFD_VIDEO_FORMATS_HH_SUPPORT "0x555"
#define DEFAULT_WFD_VIDEO_FORMATS_PROFILE WFD_H264_BASE_PROFILE
#define DEFAULT_WFD_VIDEO_FORMATS_LEVEL WFD_H264_LEVEL_3_2
#define DEFAULT_WFD_VIDEO_FORMATS_LATENCY 0x0
#define DEFAULT_WFD_VIDEO_FORMATS_VERTICAL_RESOLUTION 720
#define DEFAULT_WFD_VIDEO_FORMATS_HORIZONTAL_RESOLUTION 1280
#define DEFAULT_WFD_VIDEO_FORMATS_MIN_SLICESIZE 0
#define DEFAULT_WFD_VIDEO_FORMATS_SLICE_ENC_PARAM 200
#define DEFAULT_WFD_VIDEO_FORMATS_FRAMERATE_CONTROL 11

/* HDCP */
#define DEFAULT_ENABLE_HDCP FALSE
#define DEFAULT_WFD_HDCP_CONTENT_PROTECTION 0x0
#define DEFAULT_WFD_HDCP_PORT_NO 0


static gboolean loaded = FALSE;

/* global variables here */
#ifdef MM_WFD_SINK_DEFAULT_INI
static gboolean	__generate_sink_default_ini(void);
#endif

static void __mm_wfd_sink_ini_check_status(const char *path);

/* macro */
#define MM_WFD_SINK_INI_GET_STRING(x_dict, x_item, x_ini, x_default) \
	do { \
		gchar *str = NULL; \
		gint length = 0; \
		\
		str = iniparser_getstring(x_dict, x_ini, (char *)x_default); \
		if (str) { \
			length = strlen(str); \
			if ((length > 1) && (length < WFD_SINK_INI_MAX_STRLEN)) \
				strncpy(x_item, str, WFD_SINK_INI_MAX_STRLEN-1); \
			else \
				strncpy(x_item, x_default, WFD_SINK_INI_MAX_STRLEN-1); \
		} else { \
			strncpy(x_item, x_default, WFD_SINK_INI_MAX_STRLEN-1); \
		} \
	} while (0);

#ifdef MM_WFD_SINK_DEFAULT_INI
static
gboolean __generate_sink_default_ini(void)
{
	FILE *fp = NULL;
	const gchar *default_ini = MM_WFD_SINK_DEFAULT_INI;


	/* create new file */
	fp = fopen(MM_WFD_SINK_INI_DEFAULT_PATH, "wt");

	if (!fp) {
		return FALSE;
	}

	/* writing default ini file */
	if (strlen(default_ini) != fwrite(default_ini, 1, strlen(default_ini), fp)) {
		fclose(fp);
		return FALSE;
	}

	fclose(fp);
	return TRUE;
}
#endif

int
mm_wfd_sink_ini_load(mm_wfd_sink_ini_t *ini, const char *path)
{
	dictionary *dict = NULL;
	char tempstr[WFD_SINK_INI_MAX_STRLEN] = {0,};

	wfd_sink_debug_fenter();

	__mm_wfd_sink_ini_check_status(path);

	wfd_sink_debug("ini path : %s", path);

	/* first, try to load existing ini file */
	dict = iniparser_load(path);

	/* if no file exists. create one with set of default values */
	if (!dict) {
#ifdef MM_WFD_SINK_DEFAULT_INI
		wfd_sink_debug("No inifile found. create default ini file.");
		if (FALSE == __generate_sink_default_ini()) {
			wfd_sink_error("Creating default ini file failed. Use default values.");
		} else {
			/* load default ini */
			dict = iniparser_load(MM_WFD_SINK_INI_DEFAULT_PATH);
		}
#else
		wfd_sink_error("No ini file found. ");

		return MM_ERROR_FILE_NOT_FOUND;
#endif
	}

	/* get ini values */
	memset(ini, 0, sizeof(mm_wfd_sink_ini_t));

	if (dict) { /* if dict is available */
		/* general */
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[0], "general:gstparam1", DEFAULT_GST_PARAM);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[1], "general:gstparam2", DEFAULT_GST_PARAM);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[2], "general:gstparam3", DEFAULT_GST_PARAM);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[3], "general:gstparam4", DEFAULT_GST_PARAM);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->gst_param[4], "general:gstparam5", DEFAULT_GST_PARAM);
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
		ini->enable_wfdsrc_pad_probe = iniparser_getboolean(dict, "general:enable wfdsrc pad probe", DEFAULT_ENABLE_WFDRTSPSRC_PAD_PROBE);


		/* pipeline */
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_source, "pipeline:wfdsrc element", DEFAULT_NAME_OF_SOURCE);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_tsdemux, "pipeline:tsdemux element", DEFAULT_NAME_OF_TSDEMUX);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_audio_hdcp, "pipeline:audio hdcp element", DEFAULT_NAME_OF_AUDIO_HDCP);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_aac_parser, "pipeline:aac parser element", DEFAULT_NAME_OF_AAC_PARSER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_aac_decoder, "pipeline:aac decoder element", DEFAULT_NAME_OF_AAC_DECODER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_ac3_parser, "pipeline:ac3 parser element", DEFAULT_NAME_OF_AC3_PARSER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_ac3_decoder, "pipeline:ac3 decoder element", DEFAULT_NAME_OF_AC3_DECODER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_lpcm_converter, "pipeline:lpcm converter element", DEFAULT_NAME_OF_LPCM_CONVERTER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_lpcm_filter, "pipeline:lpcm filter element", DEFAULT_NAME_OF_LPCM_FILTER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_audio_resampler, "pipeline:audio resampler element", DEFAULT_NAME_OF_AUDIO_RESAMPLER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_audio_volume, "pipeline:audio volume element", DEFAULT_NAME_OF_AUDIO_VOLUME);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_audio_sink, "pipeline:audio sink element", DEFAULT_NAME_OF_AUDIO_SINK);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_hdcp, "pipeline:video hdcp element", DEFAULT_NAME_OF_VIDEO_HDCP);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_parser, "pipeline:video parser element", DEFAULT_NAME_OF_VIDEO_PARSER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_capssetter, "pipeline:video capssetter element", DEFAULT_NAME_OF_VIDEO_CAPSSETTER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_decoder, "pipeline:video decoder element", DEFAULT_NAME_OF_VIDEO_DECODER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_converter, "pipeline:video converter element", DEFAULT_NAME_OF_VIDEO_CONVERTER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_filter, "pipeline:video filter element", DEFAULT_NAME_OF_VIDEO_FILTER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_sink, "pipeline:video sink element", DEFAULT_NAME_OF_VIDEO_SINK);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_evas_sink, "pipeline:video evas sink element", DEFAULT_NAME_OF_EVAS_VIDEO_SINK);

		/* audio parameter*/
		ini->wfd_audio_codecs.audio_codec = iniparser_getint(dict, "wfd audio codecs:audio codec", DEFAULT_WFD_AUDIO_CODECS_CODEC);
		ini->wfd_audio_codecs.audio_latency = iniparser_getint(dict, "wfd audio codecs:audio latency", DEFAULT_WFD_AUDIO_CODECS_LATENCY);
		ini->wfd_audio_codecs.audio_channel = iniparser_getint(dict, "wfd audio codecs:audio channels", DEFAULT_WFD_AUDIO_CODECS_CHANNELS);
		ini->wfd_audio_codecs.audio_sampling_frequency = iniparser_getint(dict, "wfd audio codecs:audio sampling frequency", DEFAULT_WFD_AUDIO_CODECS_SAMP_FREQUENCY);

		/* video parameter*/
		ini->wfd_video_formats.video_codec = iniparser_getint(dict, "wfd video formats:video codec", DEFAULT_WFD_VIDEO_FORMATS_CODEC);
		ini->wfd_video_formats.video_native_resolution = iniparser_getint(dict, "wfd video formats:video native resolution", DEFAULT_WFD_VIDEO_FORMATS_NATIVE_RESOLUTION);
		memset(tempstr, 0x00, WFD_SINK_INI_MAX_STRLEN);
		MM_WFD_SINK_INI_GET_STRING(dict, tempstr, "wfd video formats:video cea support", DEFAULT_WFD_VIDEO_FORMATS_CEA_SUPPORT);
		ini->wfd_video_formats.video_cea_support = strtoul(tempstr,NULL,16);
		memset(tempstr, 0x00, WFD_SINK_INI_MAX_STRLEN);
		MM_WFD_SINK_INI_GET_STRING(dict, tempstr, "wfd video formats:video vesa support", DEFAULT_WFD_VIDEO_FORMATS_VESA_SUPPORT);
		ini->wfd_video_formats.video_vesa_support = strtoul(tempstr,NULL,16);
		memset(tempstr, 0x00, WFD_SINK_INI_MAX_STRLEN);
		MM_WFD_SINK_INI_GET_STRING(dict, tempstr, "wfd video formats:video hh support", DEFAULT_WFD_VIDEO_FORMATS_HH_SUPPORT);
		ini->wfd_video_formats.video_hh_support = strtoul(tempstr,NULL,16);
		ini->wfd_video_formats.video_profile = iniparser_getint(dict, "wfd video formats:video profile", DEFAULT_WFD_VIDEO_FORMATS_PROFILE);
		ini->wfd_video_formats.video_level = iniparser_getint(dict, "wfd video formats:video level", DEFAULT_WFD_VIDEO_FORMATS_LEVEL);
		ini->wfd_video_formats.video_latency = iniparser_getint(dict, "wfd video formats:video latency", DEFAULT_WFD_VIDEO_FORMATS_LATENCY);
		ini->wfd_video_formats.video_vertical_resolution = iniparser_getint(dict, "wfd video formats:video vertical resolution", DEFAULT_WFD_VIDEO_FORMATS_VERTICAL_RESOLUTION);
		ini->wfd_video_formats.video_horizontal_resolution = iniparser_getint(dict, "wfd video formats:video horizontal resolution", DEFAULT_WFD_VIDEO_FORMATS_HORIZONTAL_RESOLUTION);
		ini->wfd_video_formats.video_minimum_slicing = iniparser_getint(dict, "wfd video formats:video minimum slicesize", DEFAULT_WFD_VIDEO_FORMATS_MIN_SLICESIZE);
		ini->wfd_video_formats.video_slice_enc_param = iniparser_getint(dict, "wfd video formats:video slice encoding params", DEFAULT_WFD_VIDEO_FORMATS_SLICE_ENC_PARAM);
		ini->wfd_video_formats.video_framerate_control_support = iniparser_getint(dict, "wfd video formats:video framerate control support", DEFAULT_WFD_VIDEO_FORMATS_FRAMERATE_CONTROL);

		/* hdcp parameter*/
		ini->wfd_content_protection.enable_hdcp = iniparser_getboolean(dict, "wfd hdcp content protection:enable hdcp", DEFAULT_ENABLE_HDCP);
		ini->wfd_content_protection.hdcp_content_protection = iniparser_getint(dict, "wfd hdcp content protection:hdcp content protection", DEFAULT_WFD_HDCP_CONTENT_PROTECTION);
		ini->wfd_content_protection.hdcp_port_no = iniparser_getint(dict, "wfd hdcp content protection:hdcp port no", DEFAULT_WFD_HDCP_PORT_NO);
	} else { /* if dict is not available just fill the structure with default value */
		wfd_sink_error("failed to load ini. using hardcoded default\n");

		/* general */
		strncpy(ini->gst_param[0], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[1], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[2], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[3], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[4], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
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
		ini->enable_wfdsrc_pad_probe = DEFAULT_ENABLE_WFDRTSPSRC_PAD_PROBE;

		/* pipeline */
		strncpy(ini->name_of_source, DEFAULT_NAME_OF_TSDEMUX, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_tsdemux, DEFAULT_NAME_OF_TSDEMUX, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_audio_hdcp, DEFAULT_NAME_OF_AUDIO_HDCP, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_aac_parser, DEFAULT_NAME_OF_AAC_PARSER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_aac_decoder, DEFAULT_NAME_OF_AAC_DECODER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_ac3_parser, DEFAULT_NAME_OF_AC3_PARSER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_ac3_decoder, DEFAULT_NAME_OF_AC3_DECODER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_lpcm_converter, DEFAULT_NAME_OF_LPCM_CONVERTER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_lpcm_filter, DEFAULT_NAME_OF_LPCM_FILTER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_audio_resampler, DEFAULT_NAME_OF_AUDIO_RESAMPLER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_audio_volume, DEFAULT_NAME_OF_AUDIO_VOLUME, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_audio_sink, DEFAULT_NAME_OF_AUDIO_SINK, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_hdcp, DEFAULT_NAME_OF_VIDEO_HDCP, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_parser, DEFAULT_NAME_OF_VIDEO_PARSER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_capssetter, DEFAULT_NAME_OF_VIDEO_CAPSSETTER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_decoder, DEFAULT_NAME_OF_VIDEO_DECODER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_converter, DEFAULT_NAME_OF_VIDEO_CONVERTER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_filter, DEFAULT_NAME_OF_VIDEO_FILTER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_sink, DEFAULT_NAME_OF_VIDEO_SINK, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_evas_sink, DEFAULT_NAME_OF_EVAS_VIDEO_SINK, WFD_SINK_INI_MAX_STRLEN - 1);

		/* audio parameter*/
		ini->wfd_audio_codecs.audio_codec = DEFAULT_WFD_AUDIO_CODECS_CODEC;
		ini->wfd_audio_codecs.audio_latency = DEFAULT_WFD_AUDIO_CODECS_LATENCY;
		ini->wfd_audio_codecs.audio_channel = DEFAULT_WFD_AUDIO_CODECS_CHANNELS;
		ini->wfd_audio_codecs.audio_sampling_frequency = DEFAULT_WFD_AUDIO_CODECS_SAMP_FREQUENCY;

		/* video parameter*/
		ini->wfd_video_formats.video_codec = DEFAULT_WFD_VIDEO_FORMATS_CODEC;
		ini->wfd_video_formats.video_native_resolution = DEFAULT_WFD_VIDEO_FORMATS_NATIVE_RESOLUTION;
		ini->wfd_video_formats.video_cea_support = strtoul(DEFAULT_WFD_VIDEO_FORMATS_CEA_SUPPORT, NULL, 16);
		ini->wfd_video_formats.video_vesa_support = strtoul(DEFAULT_WFD_VIDEO_FORMATS_VESA_SUPPORT, NULL, 16);
		ini->wfd_video_formats.video_hh_support = strtoul(DEFAULT_WFD_VIDEO_FORMATS_HH_SUPPORT, NULL, 16);
		ini->wfd_video_formats.video_profile = DEFAULT_WFD_VIDEO_FORMATS_PROFILE;
		ini->wfd_video_formats.video_level = DEFAULT_WFD_VIDEO_FORMATS_LEVEL;
		ini->wfd_video_formats.video_latency = DEFAULT_WFD_VIDEO_FORMATS_LATENCY;
		ini->wfd_video_formats.video_vertical_resolution = DEFAULT_WFD_VIDEO_FORMATS_VERTICAL_RESOLUTION;
		ini->wfd_video_formats.video_horizontal_resolution = DEFAULT_WFD_VIDEO_FORMATS_HORIZONTAL_RESOLUTION;
		ini->wfd_video_formats.video_minimum_slicing = DEFAULT_WFD_VIDEO_FORMATS_MIN_SLICESIZE;
		ini->wfd_video_formats.video_slice_enc_param = DEFAULT_WFD_VIDEO_FORMATS_SLICE_ENC_PARAM;
		ini->wfd_video_formats.video_framerate_control_support = DEFAULT_WFD_VIDEO_FORMATS_FRAMERATE_CONTROL;

		/* hdcp parameter*/
		ini->wfd_content_protection.enable_hdcp = DEFAULT_ENABLE_HDCP;
		ini->wfd_content_protection.hdcp_content_protection = DEFAULT_WFD_HDCP_CONTENT_PROTECTION;
		ini->wfd_content_protection.hdcp_port_no = DEFAULT_WFD_HDCP_PORT_NO;
	}

	/* free dict as we got our own structure */
	iniparser_freedict(dict);


	/* dump structure */
	wfd_sink_debug("W-Fi Display Sink Initial Settings-----------------------------------\n");

	/* general */
	wfd_sink_debug("gst_param1 : %s\n", ini->gst_param[0]);
	wfd_sink_debug("gst_param2 : %s\n", ini->gst_param[1]);
	wfd_sink_debug("gst_param3 : %s\n", ini->gst_param[2]);
	wfd_sink_debug("gst_param4 : %s\n", ini->gst_param[3]);
	wfd_sink_debug("gst_param5 : %s\n", ini->gst_param[4]);
	wfd_sink_debug("generate_dot : %d\n", ini->generate_dot);
	if (ini->generate_dot == TRUE) {
		wfd_sink_debug("generate_dot is TRUE, dot file will be stored into /tmp/\n");
		g_setenv("GST_DEBUG_DUMP_DOT_DIR", "/tmp/", FALSE);
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
	wfd_sink_debug("enable_wfdsrc_pad_probe : %d\n", ini->enable_wfdsrc_pad_probe);

	/* pipeline */
	wfd_sink_debug("name_of_source : %s\n", ini->name_of_source);
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
	wfd_sink_debug("name_of_audio_sink : %s\n", ini->name_of_audio_sink);
	wfd_sink_debug("name_of_video_hdcp : %s\n", ini->name_of_video_hdcp);
	wfd_sink_debug("name_of_video_parser : %s\n", ini->name_of_video_parser);
	wfd_sink_debug("name_of_video_capssetter : %s\n", ini->name_of_video_capssetter);
	wfd_sink_debug("name_of_video_decoder : %s\n", ini->name_of_video_decoder);
	wfd_sink_debug("name_of_video_converter : %s\n", ini->name_of_video_converter);
	wfd_sink_debug("name_of_video_filter : %s\n", ini->name_of_video_filter);
	wfd_sink_debug("name_of_video_sink : %s\n", ini->name_of_video_sink);
	wfd_sink_debug("name_of_video_evas_sink : %s\n", ini->name_of_video_evas_sink);

	/* audio parameter*/
	wfd_sink_debug("wfd_audio_codecs.audio_codec : %x", ini->wfd_audio_codecs.audio_codec);
	wfd_sink_debug("wfd_audio_codecs.audio_latency : %d", ini->wfd_audio_codecs.audio_latency);
	wfd_sink_debug("wfd_audio_codecs.audio_channel : %x", ini->wfd_audio_codecs.audio_channel);
	wfd_sink_debug("wfd_audio_codecs.audio_sampling_frequency : %x", ini->wfd_audio_codecs.audio_sampling_frequency);

	/* video parameter*/
	wfd_sink_debug("wfd_video_formats.video_codec : %x\n", ini->wfd_video_formats.video_codec);
	wfd_sink_debug("wfd_video_formats.video_native_resolution : %x\n", ini->wfd_video_formats.video_native_resolution);
	wfd_sink_debug("wfd_video_formats.video_cea_support : %llx\n", ini->wfd_video_formats.video_cea_support);
	wfd_sink_debug("wfd_video_formats.video_vesa_support : %llx\n", ini->wfd_video_formats.video_vesa_support);
	wfd_sink_debug("wfd_video_formats.video_hh_support : %llx\n", ini->wfd_video_formats.video_hh_support);
	wfd_sink_debug("wfd_video_formats.video_profile : %x\n", ini->wfd_video_formats.video_profile);
	wfd_sink_debug("wfd_video_formats.video_level : %x\n", ini->wfd_video_formats.video_level);
	wfd_sink_debug("wfd_video_formats.video_latency : %d\n", ini->wfd_video_formats.video_latency);
	wfd_sink_debug("wfd_video_formats.video_vertical_resolution : %d\n", ini->wfd_video_formats.video_vertical_resolution);
	wfd_sink_debug("wfd_video_formats.video_horizontal_resolution : %d\n", ini->wfd_video_formats.video_horizontal_resolution);
	wfd_sink_debug("wfd_video_formats.video_minimum_slicin : %d\n", ini->wfd_video_formats.video_minimum_slicing);
	wfd_sink_debug("wfd_video_formats.video_slice_enc_param : %d\n", ini->wfd_video_formats.video_slice_enc_param);
	wfd_sink_debug("wfd_video_formats.video_framerate_control_support : %d\n", ini->wfd_video_formats.video_framerate_control_support);

	/* hdcp parameter*/
	wfd_sink_debug("wfd_content_protection.enable_hdcp : %d", ini->wfd_content_protection.enable_hdcp);
	wfd_sink_debug("wfd_content_protection.hdcp_content_protection : %x", ini->wfd_content_protection.hdcp_content_protection);
	wfd_sink_debug("wfd_content_protection.hdcp_port_no : %d", ini->wfd_content_protection.hdcp_port_no);

	wfd_sink_debug("---------------------------------------------------\n");

	loaded = TRUE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}


static
void __mm_wfd_sink_ini_check_status(const char *path)
{
	struct stat ini_buff;

	wfd_sink_debug_fenter();

	if (g_stat(path, &ini_buff) < 0) {
		wfd_sink_error("failed to get [%s] ini status", path);
	} else {
		if (ini_buff.st_size < 5) {
			wfd_sink_error("%s file size=%d, Corrupted! So, Removed", path, (int)ini_buff.st_size);
			g_remove(path);
		}
	}

	wfd_sink_debug_fleave();
}

int
mm_wfd_sink_ini_unload(mm_wfd_sink_ini_t *ini)
{
	wfd_sink_debug_fenter();

	loaded = FALSE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}
