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

#include "mm_wfd_sink_util.h"
#include <stdio.h>

#define DUMP_TS_DATA_PATH "/var/tmp/"

static GstPadProbeReturn
_mm_wfd_sink_util_dump(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	gint8 *data = NULL;
	gint size = 0;
	FILE *f = NULL;
	char buf[256] = {0, };
	char path[256] = {0, };
	GstElement * parent = NULL;

	parent = gst_pad_get_parent_element(pad);
	if (parent == NULL) {
		wfd_sink_error("The parent of pad is NULL.");
		return GST_PAD_PROBE_OK;
	}

	snprintf(path, sizeof(path), "%s%s_%s.ts", DUMP_TS_DATA_PATH, gst_element_get_name(parent), gst_pad_get_name(pad));
	gst_object_unref(parent);

	if (info && info->type & GST_PAD_PROBE_TYPE_BUFFER) {
		GstMapInfo buf_info;
		GstBuffer *buffer = gst_pad_probe_info_get_buffer(info);

		gst_buffer_map(buffer, &buf_info, GST_MAP_READ);

		wfd_sink_debug("got buffer %p with size %d", buffer, buf_info.size);
		data = (gint8 *)(buf_info.data);
		size = buf_info.size;
		f = fopen(path, "a");
		if (f == NULL) {
			strerror_r(errno, buf, sizeof(buf));
			wfd_sink_error("failed to fopen! : %s", buf);
			return GST_PAD_PROBE_OK;
		}
		fwrite(data, size, 1, f);
		fclose(f);
		gst_buffer_unmap(buffer, &buf_info);
	}

	return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
_mm_wfd_sink_util_pad_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	GstElement *parent = NULL;

	wfd_sink_return_val_if_fail(info &&
						info->type != GST_PAD_PROBE_TYPE_INVALID,
						GST_PAD_PROBE_DROP);
	wfd_sink_return_val_if_fail(pad, GST_PAD_PROBE_DROP);

	parent = (GstElement *)gst_object_get_parent(GST_OBJECT(pad));
	if (!parent) {
		wfd_sink_error("failed to get parent of pad");
		return GST_PAD_PROBE_DROP;
	}

	if (info->type & GST_PAD_PROBE_TYPE_BUFFER) {
		GstBuffer *buffer = gst_pad_probe_info_get_buffer(info);
		/* show name and timestamp */
		wfd_sink_debug("BUFFER PROBE : %s:%s :  %u:%02u:%02u.%09u  (%"G_GSSIZE_FORMAT" bytes)\n",
					GST_STR_NULL(GST_ELEMENT_NAME(parent)),
					GST_STR_NULL(GST_PAD_NAME(pad)),
					GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buffer)),
					gst_buffer_get_size(buffer));
	} else if (info->type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM ||
			info->type & GST_PAD_PROBE_TYPE_EVENT_UPSTREAM ||
			info->type & GST_PAD_PROBE_TYPE_EVENT_FLUSH ||
			info->type & GST_PAD_PROBE_TYPE_EVENT_BOTH) {
		GstEvent *event = gst_pad_probe_info_get_event(info);

		/* show name and event type */
		wfd_sink_debug("EVENT PROBE : %s:%s :  %s\n",
					GST_STR_NULL(GST_ELEMENT_NAME(parent)),
					GST_STR_NULL(GST_PAD_NAME(pad)),
					GST_EVENT_TYPE_NAME(event));

		if (GST_EVENT_TYPE(event) == GST_EVENT_SEGMENT) {
			const GstSegment *segment = NULL;
			gst_event_parse_segment(event, &segment);
			if (segment)
				wfd_sink_debug("NEWSEGMENT : %" GST_TIME_FORMAT
							" -- %"  GST_TIME_FORMAT ", time %" GST_TIME_FORMAT " \n",
							GST_TIME_ARGS(segment->start), GST_TIME_ARGS(segment->stop),
							GST_TIME_ARGS(segment->time));
		}
	}

	if (parent)
		gst_object_unref(parent);

	return GST_PAD_PROBE_OK;
}

void
mm_wfd_sink_util_add_pad_probe(GstPad *pad, GstElement *element, const gchar *pad_name)
{
	GstPad *probe_pad = NULL;

	if (!pad) {
		if (element && pad_name)
			probe_pad = gst_element_get_static_pad(element, pad_name);
	} else {
		probe_pad = pad;
		gst_object_ref(probe_pad);
	}

	if (probe_pad) {
		wfd_sink_debug("add pad(%s) probe", GST_STR_NULL(GST_PAD_NAME(probe_pad)));
		gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_DATA_BOTH, _mm_wfd_sink_util_pad_probe_cb, (gpointer)NULL, NULL);
		gst_object_unref(probe_pad);
	}
}

void
mm_wfd_sink_util_add_pad_probe_for_data_dump(GstElement *element, const gchar *pad_name)
{
	GstPad *probe_pad = NULL;

	if (element && pad_name)
		probe_pad = gst_element_get_static_pad(element, pad_name);

	if (probe_pad) {
		wfd_sink_debug("add pad(%s) probe", GST_STR_NULL(GST_PAD_NAME(probe_pad)));
		gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, _mm_wfd_sink_util_dump, (gpointer)NULL, NULL);
		gst_object_unref(probe_pad);
	}
}

static GstPadProbeReturn
_mm_wfd_sink_util_check_first_buffer_cb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
	GstElement *parent = NULL;
	GstBuffer *buffer = NULL;
	guint *probe_id = (guint *)user_data;

	wfd_sink_return_val_if_fail(pad, GST_PAD_PROBE_DROP);
	wfd_sink_return_val_if_fail(info, GST_PAD_PROBE_DROP);

	parent = GST_ELEMENT_CAST(gst_object_get_parent(GST_OBJECT(pad)));
	if (parent == NULL) {
		wfd_sink_error("The parent of pad is NULL.");
		return GST_PAD_PROBE_DROP;
	}

	buffer = gst_pad_probe_info_get_buffer(info);

	wfd_sink_debug("FIRST BUFFER PROBE : %s:%s :  %u:%02u:%02u.%09u (%"G_GSSIZE_FORMAT" bytes)\n",
				GST_STR_NULL(GST_ELEMENT_NAME(parent)), GST_STR_NULL(GST_PAD_NAME(pad)),
				GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buffer)), gst_buffer_get_size(buffer));

	if (probe_id && *probe_id > 0) {
		wfd_sink_debug("remove buffer probe[%d]\n", *probe_id);
		gst_pad_remove_probe(pad, *probe_id);

		MMWFDSINK_FREEIF(probe_id);
	}

	if (parent)
		gst_object_unref(parent);

	return GST_PAD_PROBE_REMOVE;
}

void
mm_wfd_sink_util_add_pad_probe_for_checking_first_buffer(GstPad *pad, GstElement *element, const gchar *pad_name)
{
	GstPad *probe_pad = NULL;
	guint *probe_id = NULL;

	if (!pad) {
		if (element && pad_name)
			probe_pad = gst_element_get_static_pad(element, pad_name);
	} else {
		probe_pad = pad;
		gst_object_ref(probe_pad);
	}

	if (probe_pad) {
		probe_id  = g_malloc0(sizeof(guint));
		if (!probe_id) {
			wfd_sink_error("failed to allocate memory for probe id\n");
			gst_object_unref(probe_pad);
			return;
		}

		*probe_id = gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER, _mm_wfd_sink_util_check_first_buffer_cb, (gpointer)probe_id, NULL);
		wfd_sink_debug("add pad(%s) probe, %d", GST_STR_NULL(GST_PAD_NAME(probe_pad)), *probe_id);

		gst_object_unref(probe_pad);
	}

	return;
}

