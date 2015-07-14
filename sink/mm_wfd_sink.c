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

#include <gst/gst.h>

#include "mm_wfd_sink_util.h"
#include "mm_wfd_sink.h"
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_dlog.h"

int mm_wfd_sink_create(MMHandleType *wfd_sink)
{
	mm_wfd_sink_t *new_wfd_sink = NULL;
	int result = MM_ERROR_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	result = _mm_wfd_sink_create(&new_wfd_sink);
	if (result != MM_ERROR_NONE) {
		wfd_sink_error("fail to create wi-fi display sink handle. ret[%d]", result);
		*wfd_sink = (MMHandleType)NULL;
		return result;
	}

	/* init wfd lock */
	g_mutex_init(&new_wfd_sink->cmd_lock);

	*wfd_sink = (MMHandleType)new_wfd_sink;

	wfd_sink_debug_fleave();

	return result;

}

int mm_wfd_sink_prepare(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_prepare((mm_wfd_sink_t *)wfd_sink);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_connect(MMHandleType wfd_sink, const char *uri)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(uri, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_connect((mm_wfd_sink_t *)wfd_sink, uri);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_start(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_start((mm_wfd_sink_t *)wfd_sink);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_pause(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_pause((mm_wfd_sink_t *)wfd_sink);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_resume(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_resume((mm_wfd_sink_t *)wfd_sink);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_disconnect(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_disconnect((mm_wfd_sink_t *)wfd_sink);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_unprepare(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_unprepare((mm_wfd_sink_t *)wfd_sink);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_destroy(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *sink_handle = NULL;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_destroy((mm_wfd_sink_t *)wfd_sink);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	g_mutex_clear(&(((mm_wfd_sink_t *)wfd_sink)->cmd_lock));

	sink_handle = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSINK_FREEIF(sink_handle);

	return result;
}

int mm_wfd_sink_set_message_callback(MMHandleType wfd_sink, MMWFDMessageCallback callback, void *user_data)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_set_message_callback((mm_wfd_sink_t *)wfd_sink, callback, user_data);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_set_attribute(MMHandleType wfd_sink,  char **err_attr_name, const char *first_attribute_name, ...)
{
	int result = MM_ERROR_NONE;
	va_list var_args;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(first_attribute_name, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd_sink);
	va_start(var_args, first_attribute_name);
	result = _mmwfd_set_attribute(MMWFDSINK_GET_ATTRS(wfd_sink), err_attr_name, first_attribute_name, var_args);
	va_end(var_args);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_get_video_resolution(MMHandleType wfd_sink, gint *width, gint *height)
{
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(width, MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail(height, MM_ERROR_WFD_INVALID_ARGUMENT);

	*width = wfd->stream_info.video_stream_info.width;
	*height = wfd->stream_info.video_stream_info.height;

	return MM_ERROR_NONE;
}

int mm_wfd_sink_get_video_framerate(MMHandleType wfd_sink, gint *frame_rate)
{
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(frame_rate, MM_ERROR_WFD_INVALID_ARGUMENT);

	*frame_rate = wfd->stream_info.video_stream_info.frame_rate;

	return MM_ERROR_NONE;
}

int mm_wfd_sink_set_resolution(MMHandleType wfd_sink,  gint resolution)
{
	int result = MM_ERROR_NONE;

	wfd_sink_return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	MMWFDSINK_CMD_LOCK(wfd_sink);
	result = _mm_wfd_sink_set_resolution((mm_wfd_sink_t *)wfd_sink, resolution);
	MMWFDSINK_CMD_UNLOCK(wfd_sink);

	return result;
}

int mm_wfd_sink_get_negotiated_video_codec(MMHandleType wfd_sink,  gint *codec)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(codec, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd);

	MMWFDSINK_PRINT_STATE(wfd);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd);
	if (cur_state != MM_WFD_SINK_STATE_CONNECTED &&
        cur_state != MM_WFD_SINK_STATE_PLAYING &&
        cur_state != MM_WFD_SINK_STATE_PAUSED) {

		wfd_sink_error("This function must be called after MM_WFD_SINK_STATE_CONNECTED");
		result = MM_ERROR_WFD_INVALID_STATE;
	} else {
		*codec = wfd->stream_info.video_stream_info.codec;
	}

	MMWFDSINK_CMD_UNLOCK(wfd);

	wfd_sink_debug_fleave();

	return result;
}

int mm_wfd_sink_get_negotiated_video_resolution(MMHandleType wfd_sink,  gint *width, gint *height)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(width, MM_ERROR_WFD_INVALID_ARGUMENT);
	wfd_sink_return_val_if_fail(height, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd);

	MMWFDSINK_PRINT_STATE(wfd);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd);
	if (cur_state != MM_WFD_SINK_STATE_CONNECTED &&
        cur_state != MM_WFD_SINK_STATE_PLAYING &&
        cur_state != MM_WFD_SINK_STATE_PAUSED) {

		wfd_sink_error("This function must be called after MM_WFD_SINK_STATE_CONNECTED");
		result = MM_ERROR_WFD_INVALID_STATE;
	} else {
		*width = wfd->stream_info.video_stream_info.width;
		*height = wfd->stream_info.video_stream_info.height;
	}

	MMWFDSINK_CMD_UNLOCK(wfd);

	wfd_sink_debug_fleave();

	return result;
}

int mm_wfd_sink_get_negotiated_video_frame_rate(MMHandleType wfd_sink,  gint *frame_rate)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(frame_rate, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd);

	MMWFDSINK_PRINT_STATE(wfd);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd);
	if (cur_state != MM_WFD_SINK_STATE_CONNECTED &&
        cur_state != MM_WFD_SINK_STATE_PLAYING &&
        cur_state != MM_WFD_SINK_STATE_PAUSED) {

		wfd_sink_error("This function must be called after MM_WFD_SINK_STATE_CONNECTED");
		result = MM_ERROR_WFD_INVALID_STATE;
	} else {
		*frame_rate = wfd->stream_info.video_stream_info.frame_rate;
	}

	MMWFDSINK_CMD_UNLOCK(wfd);

	wfd_sink_debug_fleave();

	return result;
}

int mm_wfd_sink_get_negotiated_audio_codec(MMHandleType wfd_sink,  gint *codec)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(codec, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd);

	MMWFDSINK_PRINT_STATE(wfd);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd);
	if (cur_state != MM_WFD_SINK_STATE_CONNECTED &&
        cur_state != MM_WFD_SINK_STATE_PLAYING &&
        cur_state != MM_WFD_SINK_STATE_PAUSED) {

		wfd_sink_error("This function must be called after MM_WFD_SINK_STATE_CONNECTED");
		result = MM_ERROR_WFD_INVALID_STATE;
	} else {
		*codec = wfd->stream_info.audio_stream_info.codec;
	}

	MMWFDSINK_CMD_UNLOCK(wfd);

	wfd_sink_debug_fleave();

	return result;
}

int mm_wfd_sink_get_negotiated_audio_channel(MMHandleType wfd_sink,  gint *channel)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(channel, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd);

	MMWFDSINK_PRINT_STATE(wfd);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd);
	if (cur_state != MM_WFD_SINK_STATE_CONNECTED &&
        cur_state != MM_WFD_SINK_STATE_PLAYING &&
        cur_state != MM_WFD_SINK_STATE_PAUSED) {

		wfd_sink_error("This function must be called after MM_WFD_SINK_STATE_CONNECTED");
		result = MM_ERROR_WFD_INVALID_STATE;
	} else {
		*channel = wfd->stream_info.audio_stream_info.channels;
	}

	MMWFDSINK_CMD_UNLOCK(wfd);

	wfd_sink_debug_fleave();

	return result;
}

int mm_wfd_sink_get_negotiated_audio_sample_rate(MMHandleType wfd_sink,  gint *sample_rate)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(sample_rate, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd);

	MMWFDSINK_PRINT_STATE(wfd);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd);
	if (cur_state != MM_WFD_SINK_STATE_CONNECTED &&
        cur_state != MM_WFD_SINK_STATE_PLAYING &&
        cur_state != MM_WFD_SINK_STATE_PAUSED) {

		wfd_sink_error("This function must be called after MM_WFD_SINK_STATE_CONNECTED");
		result = MM_ERROR_WFD_INVALID_STATE;
	} else {
		*sample_rate = wfd->stream_info.audio_stream_info.sample_rate;
	}

	MMWFDSINK_CMD_UNLOCK(wfd);

	wfd_sink_debug_fleave();

	return result;
}

int mm_wfd_sink_get_negotiated_audio_bitwidth(MMHandleType wfd_sink,  gint *bitwidth)
{
	int result = MM_ERROR_NONE;
	mm_wfd_sink_t *wfd = (mm_wfd_sink_t *)wfd_sink;
	MMWFDSinkStateType cur_state = MM_WFD_SINK_STATE_NONE;

	wfd_sink_debug_fenter();

	wfd_sink_return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
	wfd_sink_return_val_if_fail(bitwidth, MM_ERROR_WFD_INVALID_ARGUMENT);

	MMWFDSINK_CMD_LOCK(wfd);

	MMWFDSINK_PRINT_STATE(wfd);
	cur_state = MMWFDSINK_CURRENT_STATE(wfd);
	if (cur_state != MM_WFD_SINK_STATE_CONNECTED &&
        cur_state != MM_WFD_SINK_STATE_PLAYING &&
        cur_state != MM_WFD_SINK_STATE_PAUSED) {

		wfd_sink_error("This function must be called after MM_WFD_SINK_STATE_CONNECTED");
		result = MM_ERROR_WFD_INVALID_STATE;
	} else {
		*bitwidth = wfd->stream_info.audio_stream_info.bitwidth;
	}

	MMWFDSINK_CMD_UNLOCK(wfd);

	wfd_sink_debug_fleave();

	return result;
}
