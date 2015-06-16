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

#ifndef _MM_WFD_SINK_UTIL_H_
#define _MM_WFD_SINK_UTIL_H_

#include <glib.h>
#include <gst/gst.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>
#include "mm_wfd_sink_dlog.h"

#define MMWFDSINK_FREEIF(x) \
	do	{\
		if ((x)) \
			g_free((gpointer)(x)); \
		(x) = NULL;\
	} while (0);

/* lock for commnad */
#define MMWFDSINK_CMD_LOCK(x_wfd) \
	if (x_wfd) \
		g_mutex_lock(&(((mm_wfd_sink_t *)x_wfd)->cmd_lock));

#define MMWFDSINK_CMD_UNLOCK(x_wfd) \
	if (x_wfd) \
		g_mutex_unlock(&(((mm_wfd_sink_t *)x_wfd)->cmd_lock));

/* create element  */
#define MMWFDSINK_CREATE_ELEMENT(x_bin, x_id, x_factory, x_name, x_add_bucket) \
	do	{ \
		if (x_name && (strlen(x_factory) > 1)) {\
			x_bin[x_id].id = x_id;\
			x_bin[x_id].gst = gst_element_factory_make(x_factory, x_name);\
			if (! x_bin[x_id].gst)	{\
				wfd_sink_error("failed to create %s \n", x_factory);\
				goto CREATE_ERROR;\
			}\
			wfd_sink_debug("%s is created \n", x_factory);\
			if (x_add_bucket)\
				element_bucket = g_list_append(element_bucket, &x_bin[x_id]);\
		}\
	} while (0);

/* generating dot */
#define MMWFDSINK_GENERATE_DOT_IF_ENABLED(x_wfd_sink, x_name) \
	if (x_wfd_sink->ini.generate_dot) { \
		wfd_sink_debug("create dot file : %s.dot", x_name);\
		GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(x_wfd_sink->pipeline->mainbin[WFD_SINK_M_PIPE].gst), \
		                           GST_DEBUG_GRAPH_SHOW_ALL, x_name); \
	}

/* postint message */
#define MMWFDSINK_POST_MESSAGE(x_wfd_sink, x_error_type, x_state_type) \
	if (x_wfd_sink->msg_cb) { \
		wfd_sink_debug("Message(error : %d, state : %d) will be posted using user callback\n", x_error_type, x_state_type); \
		x_wfd_sink->msg_cb(x_error_type, x_state_type, x_wfd_sink->msg_user_data); \
	}


/* state */
#define MMWFDSINK_CURRENT_STATE(x_wfd_sink) ((mm_wfd_sink_t *)x_wfd_sink)->state.state
#define MMWFDSINK_PREVIOUS_STATE(x_wfd_sink) ((mm_wfd_sink_t *)x_wfd_sink)->state.prev_state
#define MMWFDSINK_PENDING_STATE(x_wfd_sink) ((mm_wfd_sink_t *)x_wfd_sink)->state.pending_state
#define MMWFDSINK_STATE_GET_NAME(x_state) __mm_wfds_sink_get_state_name(x_state)

#define MMWFDSINK_PRINT_STATE(x_wfd_sink) \
	wfd_sink_debug("--prev %s, current %s, pending %s--\n", \
	               MMWFDSINK_STATE_GET_NAME(MMWFDSINK_PREVIOUS_STATE(x_wfd_sink)), \
	               MMWFDSINK_STATE_GET_NAME(MMWFDSINK_CURRENT_STATE(x_wfd_sink)), \
	               MMWFDSINK_STATE_GET_NAME(MMWFDSINK_PENDING_STATE(x_wfd_sink)));

#define MMWFDSINK_CHECK_STATE(x_wfd_sink, x_cmd) \
	switch (__mm_wfd_sink_check_state((mm_wfd_sink_t *)x_wfd_sink, x_cmd)) \
	{ \
	case MM_ERROR_NONE: \
		break;\
	case MM_ERROR_WFD_NO_OP: \
		return MM_ERROR_NONE; \
		break; \
	default: \
		return MM_ERROR_WFD_INVALID_STATE; \
		break; \
	}


/* pad probe */
void
mm_wfd_sink_util_add_pad_probe(GstPad *pad, GstElement *element, const gchar *pad_name);
void
mm_wfd_sink_util_add_pad_probe_for_checking_first_buffer(GstPad *pad, GstElement *element, const gchar *pad_name);

#define MMWFDSINK_PAD_PROBE(x_wfd_sink, x_pad, x_element, x_pad_name) \
	if (x_wfd_sink) {  \
		if (x_wfd_sink->ini.enable_pad_probe) { \
			mm_wfd_sink_util_add_pad_probe(x_pad, x_element, (const gchar*)x_pad_name); \
		} else {\
			mm_wfd_sink_util_add_pad_probe_for_checking_first_buffer (x_pad, x_element, (const gchar*)x_pad_name); \
		}\
	}

void
mm_wfd_sink_util_add_pad_probe_for_data_dump(GstElement *element, const gchar *pad_name);

#define MMWFDSINK_TS_DATA_DUMP(x_wfd_sink, x_element, x_pad_name) \
	if (x_wfd_sink && x_wfd_sink->ini.enable_ts_data_dump) { \
		mm_wfd_sink_util_add_pad_probe_for_data_dump (x_element, (const gchar*)x_pad_name); \
	}

#endif
