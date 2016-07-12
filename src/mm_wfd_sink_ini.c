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

/*Default sink ini values*/
/* General*/
#define DEFAULT_ENABLE_RM	TRUE
#define DEFAULT_USER_AGENT "TIZEN3_0/WFD-SINK"

/* Debug */
#define DEFAULT_GENERATE_DOT	FALSE
#define DEFAULT_DUMP_RTSP_MESSAGE	TRUE
#define DEFAULT_TRACE_BUFFERS	FALSE
#define DEFAULT_TRACE_FIRST_BUFFER   TRUE
#define DEFAULT_TRACE_BUFFERS_OF_WFDSRC	FALSE
#define DEFAULT_DUMP_TS_DATA		FALSE
#define DEFAULT_DUMP_RTP_DATA FALSE

/* Pipeline */
#define DEFAULT_NAME_OF_VIDEO_H264_PARSER ""
#define DEFAULT_NAME_OF_VIDEO_H264_DECODER ""

/* Audio */
#define DEFAULT_WFD_AUDIO_CODECS_CODEC WFD_AUDIO_LPCM | WFD_AUDIO_AAC
#define DEFAULT_WFD_AUDIO_CODECS_LATENCY 0x0
#define DEFAULT_WFD_AUDIO_CODECS_CHANNELS WFD_CHANNEL_2
#define DEFAULT_WFD_AUDIO_CODECS_SAMP_FREQUENCY WFD_FREQ_44100 | WFD_FREQ_48000

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

	if (!fp)
		return FALSE;

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
		ini->state_change_timeout = iniparser_getint(dict, "general:state change timeout", DEFAULT_STATE_CHANGE_TIMEOUT);
		ini->enable_rm = iniparser_getboolean(dict, "general:enable rm", DEFAULT_ENABLE_RM);
		ini->jitter_buffer_latency = iniparser_getint(dict, "general:jitter buffer latency", DEFAULT_JITTER_BUFFER_LATENCY);
		ini->enable_retransmission = iniparser_getboolean(dict, "general:enable retransmission", DEFAULT_ENABLE_RETRANSMISSION);
		ini->enable_reset_basetime = iniparser_getboolean(dict, "general:enable reset basetime", DEFAULT_ENABLE_RESET_BASETIME);
		ini->video_sink_max_lateness = iniparser_getint(dict, "general:video sink max lateness", DEFAULT_VIDEO_SINK_MAX_LATENESS);
		ini->sink_ts_offset = iniparser_getint(dict, "general:sink ts offset", DEFAULT_SINK_TS_OFFSET);
		ini->audio_sink_async = iniparser_getboolean(dict, "general:audio sink async", DEFAULT_AUDIO_SINK_ASYNC);
		ini->video_sink_async = iniparser_getboolean(dict, "general:video sink async", DEFAULT_VIDEO_SINK_ASYNC);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->user_agent, "general:user agent", DEFAULT_USER_AGENT);

		/* debug */
		ini->generate_dot = iniparser_getboolean(dict, "debug:generate dot", DEFAULT_GENERATE_DOT);
		ini->dump_rtsp_message = iniparser_getboolean(dict, "debug:dump rtsp message", DEFAULT_DUMP_RTSP_MESSAGE);
		ini->trace_buffers = iniparser_getboolean(dict, "debug:trace buffers", DEFAULT_TRACE_BUFFERS);
		ini->trace_first_buffer = iniparser_getboolean(dict, "debug:trace first buffer", DEFAULT_TRACE_FIRST_BUFFER);
		ini->trace_buffers_of_wfdsrc = iniparser_getboolean(dict, "debug:trace buffers of wfdsrc", DEFAULT_TRACE_BUFFERS_OF_WFDSRC);
		ini->dump_ts_data = iniparser_getboolean(dict, "debug:dump ts data", DEFAULT_DUMP_TS_DATA);
		ini->dump_rtp_data = iniparser_getboolean(dict, "debug:dump rtp data", DEFAULT_DUMP_RTP_DATA);

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
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_h264_parser, "pipeline:video h264 parser element", DEFAULT_NAME_OF_VIDEO_H264_PARSER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_capssetter, "pipeline:video capssetter element", DEFAULT_NAME_OF_VIDEO_CAPSSETTER);
		MM_WFD_SINK_INI_GET_STRING(dict, ini->name_of_video_h264_decoder, "pipeline:video h264 decoder element", DEFAULT_NAME_OF_VIDEO_H264_DECODER);
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
		ini->wfd_content_protection.enable_hdcp = iniparser_getboolean(dict, "wfd hdcp content protection:enable hdcp", DEFAULT_ENABLE_HDCP);
		ini->wfd_content_protection.hdcp_content_protection = iniparser_getint(dict, "wfd hdcp content protection:hdcp content protection", DEFAULT_WFD_HDCP_CONTENT_PROTECTION);
		ini->wfd_content_protection.hdcp_port_no = iniparser_getint(dict, "wfd hdcp content protection:hdcp port no", DEFAULT_WFD_HDCP_PORT_NO);
	} else { /* if dict is not available just fill the structure with default value */
		wfd_sink_error("failed to load ini. using hardcoded default");

		/* general */
		strncpy(ini->gst_param[0], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[1], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[2], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[3], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->gst_param[4], DEFAULT_GST_PARAM, WFD_SINK_INI_MAX_STRLEN - 1);
		ini->state_change_timeout = DEFAULT_STATE_CHANGE_TIMEOUT;
		ini->enable_rm =  DEFAULT_ENABLE_RM;
		ini->jitter_buffer_latency = DEFAULT_JITTER_BUFFER_LATENCY;
		ini->enable_retransmission =  DEFAULT_ENABLE_RETRANSMISSION;
		ini->enable_reset_basetime =  DEFAULT_ENABLE_RESET_BASETIME;
		ini->video_sink_max_lateness = DEFAULT_VIDEO_SINK_MAX_LATENESS;
		ini->sink_ts_offset = DEFAULT_SINK_TS_OFFSET;
		strncpy(ini->user_agent, DEFAULT_USER_AGENT, WFD_SINK_INI_MAX_STRLEN - 1);

		/* debug */
		ini->generate_dot =  DEFAULT_GENERATE_DOT;
		ini->dump_rtsp_message =  DEFAULT_DUMP_RTSP_MESSAGE;
		ini->trace_buffers = DEFAULT_TRACE_BUFFERS;
		ini->trace_first_buffer = DEFAULT_TRACE_FIRST_BUFFER;
		ini->trace_buffers_of_wfdsrc = DEFAULT_TRACE_BUFFERS_OF_WFDSRC;
		ini->dump_ts_data = DEFAULT_DUMP_TS_DATA;
		ini->dump_rtp_data = DEFAULT_DUMP_RTP_DATA;

		/* pipeline */
		strncpy(ini->name_of_source, DEFAULT_NAME_OF_SOURCE, WFD_SINK_INI_MAX_STRLEN - 1);
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
		strncpy(ini->name_of_video_h264_parser, DEFAULT_NAME_OF_VIDEO_H264_PARSER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_capssetter, DEFAULT_NAME_OF_VIDEO_CAPSSETTER, WFD_SINK_INI_MAX_STRLEN - 1);
		strncpy(ini->name_of_video_h264_decoder, DEFAULT_NAME_OF_VIDEO_H264_DECODER, WFD_SINK_INI_MAX_STRLEN - 1);
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
		ini->wfd_content_protection.enable_hdcp = DEFAULT_ENABLE_HDCP;
		ini->wfd_content_protection.hdcp_content_protection = DEFAULT_WFD_HDCP_CONTENT_PROTECTION;
		ini->wfd_content_protection.hdcp_port_no = DEFAULT_WFD_HDCP_PORT_NO;
	}

	/* free dict as we got our own structure */
	iniparser_freedict(dict);


	/* dump structure */
	wfd_sink_debug("Wi-Fi Display Sink Initial Settings-----------------------------------");

	/* general */
	wfd_sink_debug("gst_param1 : %s", ini->gst_param[0]);
	wfd_sink_debug("gst_param2 : %s", ini->gst_param[1]);
	wfd_sink_debug("gst_param3 : %s", ini->gst_param[2]);
	wfd_sink_debug("gst_param4 : %s", ini->gst_param[3]);
	wfd_sink_debug("gst_param5 : %s", ini->gst_param[4]);
	wfd_sink_debug("enable_rm : %d", ini->enable_rm);

	/* debug */
	wfd_sink_debug("generate_dot : %d", ini->generate_dot);
	if (ini->generate_dot == TRUE) {
		wfd_sink_debug("generate_dot is TRUE, dot file will be stored into /tmp/");
		g_setenv("GST_DEBUG_DUMP_DOT_DIR", "/tmp/", FALSE);
	}
	wfd_sink_debug("dump_rtsp_message : %d", ini->dump_rtsp_message);
	wfd_sink_debug("trace_buffers : %d", ini->trace_buffers);
	wfd_sink_debug("trace_first_buffer : %d", ini->trace_first_buffer);
	wfd_sink_debug("state_change_timeout(sec) : %d\n", ini->state_change_timeout);
	wfd_sink_debug("jitter_buffer_latency(msec) : %d\n", ini->jitter_buffer_latency);
	wfd_sink_debug("enable_retransmission : %d\n", ini->enable_retransmission);
	wfd_sink_debug("enable_reset_basetime : %d\n", ini->enable_reset_basetime);
	wfd_sink_debug("video_sink_max_lateness(nsec) : %d\n", ini->video_sink_max_lateness);
	wfd_sink_debug("sink_ts_offset(nsec) : %d\n", ini->sink_ts_offset);
	wfd_sink_debug("audio_sink_async : %d\n", ini->audio_sink_async);
	wfd_sink_debug("video_sink_async : %d\n", ini->video_sink_async);
	wfd_sink_debug("trace_buffers_of_wfdsrc : %d", ini->trace_buffers_of_wfdsrc);
	wfd_sink_debug("dump_ts_data : %d", ini->dump_ts_data);
	wfd_sink_debug("dump_rtp_data : %d", ini->dump_rtp_data);


	/* pipeline */
	wfd_sink_debug("name_of_source : %s", ini->name_of_source);
	wfd_sink_debug("name_of_tsdemux : %s", ini->name_of_tsdemux);
	wfd_sink_debug("name_of_audio_hdcp : %s", ini->name_of_audio_hdcp);
	wfd_sink_debug("name_of_aac_parser : %s", ini->name_of_aac_parser);
	wfd_sink_debug("name_of_aac_decoder : %s", ini->name_of_aac_decoder);
	wfd_sink_debug("name_of_ac3_parser : %s", ini->name_of_ac3_parser);
	wfd_sink_debug("name_of_ac3_decoder : %s", ini->name_of_ac3_decoder);
	wfd_sink_debug("name_of_lpcm_converter : %s", ini->name_of_lpcm_converter);
	wfd_sink_debug("name_of_lpcm_filter : %s", ini->name_of_lpcm_filter);
	wfd_sink_debug("name_of_audio_resampler : %s", ini->name_of_audio_resampler);
	wfd_sink_debug("name_of_audio_volume : %s", ini->name_of_audio_volume);
	wfd_sink_debug("name_of_audio_sink : %s", ini->name_of_audio_sink);
	wfd_sink_debug("name_of_video_hdcp : %s", ini->name_of_video_hdcp);
	wfd_sink_debug("name_of_video_h264_parser : %s", ini->name_of_video_h264_parser);
	wfd_sink_debug("name_of_video_capssetter : %s\n", ini->name_of_video_capssetter);
	wfd_sink_debug("name_of_video_h264_decoder : %s", ini->name_of_video_h264_decoder);
	wfd_sink_debug("name_of_video_converter : %s", ini->name_of_video_converter);
	wfd_sink_debug("name_of_video_filter : %s", ini->name_of_video_filter);
	wfd_sink_debug("name_of_video_sink : %s", ini->name_of_video_sink);
	wfd_sink_debug("name_of_video_evas_sink : %s", ini->name_of_video_evas_sink);

	/* audio parameter*/
	wfd_sink_debug("wfd_audio_codecs.audio_codec : %x", ini->wfd_audio_codecs.audio_codec);
	wfd_sink_debug("wfd_audio_codecs.audio_latency : %d", ini->wfd_audio_codecs.audio_latency);
	wfd_sink_debug("wfd_audio_codecs.audio_channel : %x", ini->wfd_audio_codecs.audio_channel);
	wfd_sink_debug("wfd_audio_codecs.audio_sampling_frequency : %x", ini->wfd_audio_codecs.audio_sampling_frequency);

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
	wfd_sink_debug("wfd_content_protection.enable_hdcp : %d", ini->wfd_content_protection.enable_hdcp);
	wfd_sink_debug("wfd_content_protection.hdcp_content_protection : %x", ini->wfd_content_protection.hdcp_content_protection);
	wfd_sink_debug("wfd_content_protection.hdcp_port_no : %d", ini->wfd_content_protection.hdcp_port_no);

	wfd_sink_debug("---------------------------------------------------");

	loaded = TRUE;

	wfd_sink_debug_fleave();

	return MM_ERROR_NONE;
}


static
void __mm_wfd_sink_ini_check_status(const char *path)
{
	struct stat ini_buff;

	wfd_sink_return_if_fail(path);

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
