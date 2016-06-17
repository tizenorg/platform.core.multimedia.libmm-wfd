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

#ifndef _MM_WFD_SINK_H_
#define _MM_WFD_SINK_H_

#include <string.h>
#include <glib.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>

/**
 *  * Enumerations of wifi-display sink state.
 *   */
typedef enum {
	MM_WFD_SINK_STATE_NONE,				/**< wifi-display is not created */
	MM_WFD_SINK_STATE_NULL,				/**< wifi-display is created */
	MM_WFD_SINK_STATE_PREPARED,			/**< wifi-display is prepared */
	MM_WFD_SINK_STATE_CONNECTED,		 /**< wifi-display is connected */
	MM_WFD_SINK_STATE_PLAYING,			/**< wifi-display is now playing  */
	MM_WFD_SINK_STATE_PAUSED,			/**< wifi-display is now paused  */
	MM_WFD_SINK_STATE_DISCONNECTED,	/**< wifi-display is disconnected */
	MM_WFD_SINK_STATE_NUM,				/**< Number of wifi-display states */
} MMWFDSinkStateType;

/* audio codec : AAC, AC3, LPCM  */
typedef enum {
	MM_WFD_SINK_AUDIO_CODEC_NONE,
	MM_WFD_SINK_AUDIO_CODEC_AAC = 0x0F,
	MM_WFD_SINK_AUDIO_CODEC_AC3 = 0x81,
	MM_WFD_SINK_AUDIO_CODEC_LPCM = 0x83
} MMWFDSinkAudioCodec;

/* video codec : H264  */
typedef enum {
	MM_WFD_SINK_VIDEO_CODEC_NONE,
	MM_WFD_SINK_VIDEO_CODEC_H264 = 0x1b
} MMWFDSinkVideoCodec;

typedef void(*MMWFDMessageCallback)(int error_type, MMWFDSinkStateType state_type, void *user_data);


/**
 * This function creates a wi-fi display sink object. \n
 * The attributes of wi-fi display sink are created to get/set some values with application. \n
 * And, mutex, gstreamer and other resources are initialized at this time. \n
 * If wi-fi display sink is created, he state will become MM_WFD_SINK_STATE_NULL.. \n
 *
 * @param	wfd_sink		[out]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code. \n
 *			Please refer 'mm_error.h' to know it in detail.
 * @pre		None
 * @post 		MM_WFD_SINK_STATE_NULL
 * @see		mm_wfd_sink_destroy
 * @remark	You can create multiple handles on a context at the same time. \n
 *			However, wi-fi display sink cannot guarantee proper operation because of limitation of resources, \n
 * 			such as audio device or display device.
 *
 * @par Example
 * @code
if (mm_wfd_sink_create(&g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to create wi-fi display sink\n");
}

mm_wfd_sink_set_message_callback(g_wfd_sink_handle, msg_callback, (void*)g_wfd_sink_handle);
 * @endcode
 */
int mm_wfd_sink_create(MMHandleType *wfd_sink);

/**
 * This function trys to make gstreamer pipeline. \n
 * If wi-fi display sink is realized, the state will become MM_WFD_SINK_STATE_READY.. \n
 *
 * @param	wfd_sink		[out]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code. \n
 *			Please refer 'mm_error.h' to know it in detail.
 * @pre		MM_WFD_SINK_STATE_NULL
 * @post 		MM_WFD_SINK_STATE_PREPARED
 * @see		mm_wfd_sink_unprepare
 * @remark	None
 * @par Example
 * @code
if (mm_wfd_sink_prepare(&g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to realize wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_prepare(MMHandleType wfd_sink_handle);

/**
 * This function connect wi-fi display source using uri. \n
 * audio type(AC3 AAC, LPCM) is decided at this time. \n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	uri			[in]	URI of wi-fi displaysource to be connected
 *
 * @return	This function returns zero on success, or negative value with error code.
 *
 * @pre		wi-fi display sink state should be MM_WFD_SINK_STATE_PREPARED
 * @post		wi-fi display sink state will be MM_WFD_SINK_STATE_CONNECTED with no preroll.
 * @remark 	None
 * @par Example
 * @code
if (mm_wfd_sink_connect(g_wfd_sink_handle, g_uri) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to connect to wi-fi display source\n");
}
 * @endcode
 */
int mm_wfd_sink_connect(MMHandleType wfd_sink_handle, const char *uri);

/**
 * This function is to start playing. \n
 * Data from wi-fi display source will be received. \n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @remark
 *
 * @pre		wi-fi display sink state may be MM_WFD_SINK_STATE_CONNECTED.
 * @post		wi-fi display sink state will be MM_WFD_SINK_STATE_PLAYING.
 * @see		mm_wfd_sink_disconnect
 * @remark 	None
 * @par Example
 * @code
if (mm_wfd_sink_start(g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to start wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_start(MMHandleType wfd_sink_handle);

/**
 * This function is to pause playing. \n
 * The wi-fi display sink pause the current stream being received form wi-fi display source. \n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @remark
 *
 * @pre		wi-fi display sink state should be MM_WFD_SINK_STATE_PLAYING.
 * @post		wi-fi display sink state will be MM_WFD_SINK_STATE_PAUSED.
 * @see		mm_wfd_sink_pause
 * @remark 	None
 * @par Example
 * @code
if (mm_wfd_sink_pause(g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to pause wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_pause(MMHandleType wfd_sink_handle);

/**
 * This function is to resume playing. \n
 * Data from wi-fi display source will be received. \n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 *
 * @return	This function returns zero  on success, or negative value with error code.
 * @remark
 *
 * @pre		wi-fi display sink state may be MM_WFD_SINK_STATE_PAUSED.
 * @post		wi-fi display sink state will be MM_WFD_SINK_STATE_PLAYING.
 * @see		mm_wfd_sink_disconnect
 * @remark 	None
 * @par Example
 * @code
if (mm_wfd_sink_start(g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to resume wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_resume(MMHandleType wfd_sink_handle);

/**
 * This function is to stop playing wi-fi display. \n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 *
 * @pre		wi-fi display sink state may be MM_WFD_SINK_STATE_PLAYING.
 * @post		wi-fi display sink state will be MM_WFD_SINK_STATE_DISCONNECTED.
 * @see		mm_wfd_sink_start
 * @remark 	None
 * @par Example
 * @code
if (mm_wfd_sink_disconnect(g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to stop wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_disconnect(MMHandleType wfd_sink_handle);

/**
 * This function trys to destroy gstreamer pipeline. \n
 *
 * @param	wfd_sink_handle		[out]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code. \n
 *			Please refer 'mm_error.h' to know it in detail.
 * @pre		wi-fi display sink state may be MM_WFD_SINK_STATE_PREPARED or MM_WFD_SINK_STATE_DISCONNECTED.
 * 			But, it can be called in any state.
 * @post 		MM_WFD_SINK_STATE_NULL
 * @see		mm_wfd_sink_prepare
 * @remark	None
 * @par Example
 * @code
if (mm_wfd_sink_unprepare(&g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to unprepare wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_unprepare(MMHandleType wfd_sink_handle);

/**
 * This function releases wi-fi display sink object and all resources which were created by mm_wfd_sink_create(). \n
 * And, wi-fi display sink handle will also be destroyed. \n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state may be MM_WFD_SINK_STATE_NULL. \n
 * 			But, it can be called in any state.
 * @post		Because handle is released, there is no any state.
 * @see		mm_wfd_sink_create
 * @remark	This method can be called with a valid wi-fi display sink handle from any state to \n
 *			completely shutdown the wi-fi display sink operation.
 *
 * @par Example
 * @code
if (mm_wfd_sink_destroy(g_wfd_sink_handle) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to destroy wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_destroy(MMHandleType wfd_sink_handle);

/**
 * This function sets callback function for receiving messages from wi-fi display sink. \n
 * So, player can notify warning, error and normal cases to application. \n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	callback	[in]	Message callback function.
 * @param	user_param	[in]	User parameter which is passed to callback function.
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @see		MMWFDMessageCallback
 * @remark	None
 * @par Example
 * @code

int msg_callback(int error_type, MMWFDSinkStateType state_type, void *user_data)
{
	switch (state_type)
	{
		case MM_WFD_SINK_STATE_NULL:
			//do something
			break;

		case MM_WFD_SINK_STATE_PREPARED:
			//do something
			break;

		case MM_WFD_SINK_STATE_CONNECTED:
			//do something
			break;

		case MM_WFD_SINK_STATE_PLAYING:
			//do something
			break;

		case MM_WFD_SINK_STATE_PAUSED:
			//do something
			break;

		case MM_WFD_SINK_DISCONNECTED:
			//do something
			break;

		default:
			break;
	}
	return TRUE;
}

mm_wfd_sink_set_message_callback(g_wfd_sink_handle, msg_callback, (void*)g_wfd_sink_handle);
 * @endcode
 */
int mm_wfd_sink_set_message_callback(MMHandleType wfd_sink_handle, MMWFDMessageCallback callback, void *user_param);

int mm_wfd_sink_set_attribute(MMHandleType wfd_sink_handle,  char **err_attr_name, const char *first_attribute_name, ...);

/**
 * This function get resources \n
 *
 * @param	wfd_sink		[in]	Handle of wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_READY or MM_WFD_SINK_STATE_PREPARED. \n
 * @post		N/A
 * @remark	resources are released when mm_wfd_sink_destory is called
 *
 * @par Example
 * @code
if (mm_wfd_sink_get_resource(g_wfd_sink) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to get resources for wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_get_resource(MMHandleType wfd_sink);

/**
 * This function sets the display surface type for wi-fi display sink\n
 *
 * @param	wfd_sink		[in]	Handle of wi-fi display sink
 * @param	display_surface_type		[in]	Display surface type
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_NULL. \n
 *
 * @par Example
 * @code
if (mm_wfd_sink_set_display_surface_type(g_wfd_sink, g_display_surface_type) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to set display surface type for wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_set_display_surface_type(MMHandleType wfd_sink, gint display_surface_type);

/**
 * This function sets the display overlay for wi-fi display sink\n
 *
 * @param	wfd_sink		[in]	Handle of wi-fi display sink
 * @param	display_overlay		[in]	Display overlay
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_NULL. \n
 *
 * @par Example
 * @code
if (mm_wfd_sink_set_display_overlay(g_wfd_sink, g_display_overlay) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to set display overlay for wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_set_display_overlay(MMHandleType wfd_sink, void *display_overlay);

/**
 * This function sets the display method for wi-fi display sink\n
 *
 * @param	wfd_sink		[in]	Handle of wi-fi display sink
 * @param	display_method		[in]	Display method
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_NULL. \n
 *
 * @par Example
 * @code
if (mm_wfd_sink_set_display_method(g_wfd_sink, g_display_method) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to set display method for wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_set_display_method(MMHandleType wfd_sink, gint display_method);

/**
 * This function sets the display visible for wi-fi display sink\n
 *
 * @param	wfd_sink		[in]	Handle of wi-fi display sink
 * @param	display_visible		[in]	Display visible
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_NULL. \n
 *
 * @par Example
 * @code
if (mm_wfd_sink_set_display_visible(g_wfd_sink, g_display_visible) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to set display visible for wi-fi display sink\n");
}
 * @endcode
 */
int mm_wfd_sink_set_display_visible(MMHandleType wfd_sink, gint display_visible);

/**
 * This function gets the width and height of video which is played by wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	width		[in]	Width of video
 * @param	height		[in]	Height of video
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED or MM_WFD_SINK_STATE_PLAYING. \n
 *
 * @par Example
 * @code
gint g_width=0, g_height=0;

if (mm_wfd_sink_get_video_resolution(g_wfd_sink_handle, &g_width, &g_height) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to get video resolution.\n");
}
 * @endcode
 */
int mm_wfd_sink_get_video_resolution(MMHandleType wfd_sink_handle, gint *width, gint *height);

/**
 * This function gets the width and height of video which is played by wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	framerate		[in]	Framerate of video
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED or MM_WFD_SINK_STATE_PLAYING. \n
 *
 * @par Example
 * @code
gint g_framerate=0;

if (mm_wfd_sink_get_video_framerate(g_wfd_sink_handle, &g_framerate) != MM_ERROR_NONE)
{
	wfd_sink_error("failed to get video framerate.\n");
}
 * @endcode
 */
int mm_wfd_sink_get_video_framerate(MMHandleType wfd_sink_handle,  gint *framerate);

/**
 * This function sets the resolutions for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	resolution		[in]	Resolutions for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_NULL. \n
 *
 */
int mm_wfd_sink_set_resolution(MMHandleType wfd_sink_handle,  gint resolution);

/**
 * This function gets the negotiated video codec for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	codec		[in]	video codec for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED, MM_WFD_SINK_STATE_PLAYING or MM_WFD_SINK_STATE_PAUSED. \n
 *
 */
int mm_wfd_sink_get_negotiated_video_codec(MMHandleType wfd_sink_handle,  gint *codec);

/**
 * This function gets the negotiated video resolution for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	width		[in]	video width for wi-fi display sink
 * @param	height		[in]	video height for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED, MM_WFD_SINK_STATE_PLAYING or MM_WFD_SINK_STATE_PAUSED. \n
 *
 */
int mm_wfd_sink_get_negotiated_video_resolution(MMHandleType wfd_sink_handle,  gint *width, gint *height);

/**
 * This function gets the negotiated video framerate for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	framerate		[in]	video framerate for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED, MM_WFD_SINK_STATE_PLAYING or MM_WFD_SINK_STATE_PAUSED. \n
 *
 */
int mm_wfd_sink_get_negotiated_video_frame_rate(MMHandleType wfd_sink_handle,  gint *frame_rate);

/**
 * This function gets the negotiated audio codec for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	codec		[in]	audio codec for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED, MM_WFD_SINK_STATE_PLAYING or MM_WFD_SINK_STATE_PAUSED. \n
 *
 */
int mm_wfd_sink_get_negotiated_audio_codec(MMHandleType wfd_sink_handle,  gint *codec);

/**
 * This function gets the negotiated audio channel for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	channel		[in]	audio channel for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED, MM_WFD_SINK_STATE_PLAYING or MM_WFD_SINK_STATE_PAUSED. \n
 *
 */
int mm_wfd_sink_get_negotiated_audio_channel(MMHandleType wfd_sink_handle,  gint *channel);

/**
 * This function gets the negotiated audio sample rate for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	sample_rate		[in]	audio sample rate for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED, MM_WFD_SINK_STATE_PLAYING or MM_WFD_SINK_STATE_PAUSED. \n
 *
 */
int mm_wfd_sink_get_negotiated_audio_sample_rate(MMHandleType wfd_sink_handle,  gint *sample_rate);

/**
 * This function gets the negotiated audio bitwidth for wi-fi display sink\n
 *
 * @param	wfd_sink_handle		[in]	Handle of wi-fi display sink
 * @param	bitwidth		[in]	audio bitwidth for wi-fi display sink
 *
 * @return	This function returns zero on success, or negative value with error code.
 * @pre		wi-fi display state should be MM_WFD_SINK_STATE_CONNECTED, MM_WFD_SINK_STATE_PLAYING or MM_WFD_SINK_STATE_PAUSED. \n
 *
 */
int mm_wfd_sink_get_negotiated_audio_bitwidth(MMHandleType wfd_sink_handle,  gint *bitwidth);
#endif
