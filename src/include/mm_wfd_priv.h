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

#ifndef __MM_WFD_PRIV_H__
#define	__MM_WFD_PRIV_H__

/*===========================================================================================
|																							|
|  INCLUDE FILES																			|
|  																							|
========================================================================================== */
#include <glib.h>
#include <mm_types.h>
#include <mm_attrs.h>
#include <mm_wfd_attrs.h>
#include <mm_ta.h>
#include <mm_debug.h>
#include <mm_error.h>
#include "mm_message.h"
#include <string.h>
#include "mm_wfd_ini.h"
#include "mm_wfd.h"
#include <gst/gst.h>

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
#define _mmwfd_dbg_verb(fmt, args...)	mmf_debug(MMF_DEBUG_VERBOSE,"[%05d][%s]: " fmt "\n", __LINE__, __func__, ##args);
#define _mmwfd_dbg_log(fmt, args...)	mmf_debug(MMF_DEBUG_LOG,"[%05d][%s]: " fmt "\n", __LINE__, __func__, ##args);
#define _mmwfd_dbg_warn(fmt, args...)	mmf_debug(MMF_DEBUG_WARNING,"[%05d][%s]: " fmt "\n", __LINE__, __func__, ##args);
#define _mmwfd_dbg_err(fmt, args...)	mmf_debug(MMF_DEBUG_ERROR,"[%05d][%s]: " fmt "\n", __LINE__, __func__, ##args);
#define _mmwfd_dbg_crit(fmt, args...)	mmf_debug(MMF_DEBUG_CRITICAL,"[%05d][%s]: " fmt "\n", __LINE__, __func__, ##args);

#define MMWFD_FREEIF(x) \
if ( x ) \
	g_free( x ); \
x = NULL;


#define MMWFD_CMD_LOCK(x_wfd)	g_mutex_lock( ((mm_wfd_t*)x_wfd)->cmd_lock )
#define MMWFD_CMD_UNLOCK(x_wfd)	g_mutex_unlock( ((mm_wfd_t*)x_wfd)->cmd_lock )

#define MMWFD_GET_ATTRS(x_wfd)	((mm_wfd_t*)x_wfd)->attrs

/* state */
#define MMWFD_PREV_STATE(x_wfd)		((mm_wfd_t*)x_wfd)->prev_state
#define MMWFD_CURRENT_STATE(x_wfd)	((mm_wfd_t*)x_wfd)->state
#define MMWFD_PENDING_STATE(x_wfd)	((mm_wfd_t*)x_wfd)->pending_state
#define MMWFD_TARGET_STATE(x_wfd)	((mm_wfd_t*)x_wfd)->target_state
#define MMWFD_STATE_GET_NAME(state) 	__get_state_name(state)

#define 	MMWFD_PRINT_STATE(x_wfd) \
debug_log("-----------------------WFD STATE-------------------------\n"); \
debug_log(" prev %s, current %s, pending %s, target %s \n", \
	MMWFD_STATE_GET_NAME(MMWFD_PREV_STATE(x_wfd)), \
 	MMWFD_STATE_GET_NAME(MMWFD_CURRENT_STATE(x_wfd)), \
	MMWFD_STATE_GET_NAME(MMWFD_PENDING_STATE(x_wfd)), \
	MMWFD_STATE_GET_NAME(MMWFD_TARGET_STATE(x_wfd))); \
debug_log("------------------------------------------------------------\n");


#define MMWFD_CHECK_STATE_RETURN_IF_FAIL( x_wfd, x_command ) \
debug_log("checking wfd state before doing %s\n", #x_command); \
switch ( __mmwfd_check_state(x_wfd, x_command) ) \
{ \
	case MM_ERROR_WFD_INVALID_STATE: \
		return MM_ERROR_WFD_INVALID_STATE; \
	break; \
	/* NOTE : for robustness of wfd. we won't treat it as an error */ \
	case MM_ERROR_WFD_NO_OP: \
		return MM_ERROR_NONE; \
	break; \
	default: \
	break; \
}

/* setting wfd state */
#define MMWFD_SET_STATE( x_wfd, x_state ) \
debug_log("setting wfd state to %d\n", x_state); \
__mmwfd_set_state(x_wfd, x_state);

/* message posting */
#define MMWFD_POST_MSG( x_wfd, x_msgtype, x_msg_param ) \
debug_log("posting %s to application\n", #x_msgtype); \
__mmwfd_post_message(x_wfd, x_msgtype, x_msg_param);

enum PlayerCommandState
{
	MMWFD_COMMAND_NONE,
	MMWFD_COMMAND_CREATE,
	MMWFD_COMMAND_DESTROY,
	MMWFD_COMMAND_REALIZE,
	MMWFD_COMMAND_UNREALIZE,
	MMFWD_COMMAND_CONNECT,
	MMWFD_COMMAND_START,
	MMWFD_COMMAND_STOP,
	MMWFD_COMMAND_PAUSE,
	MMWFD_COMMAND_RESUME,
	MMWFD_COMMAND_NUM,
};

/* main pipeline's element id */
enum MainElementID
{
	MMWFD_M_PIPE = 0,	/* NOTE : MMWFD_M_PIPE should be zero */

	/* Video source bin elements */
	MMWFD_M_VSRC,		/* video source */
	MMWFD_M_VQ,		/* video queue */
	MMWFD_M_VENC,		/* video encoder */
	/* Audio source bin elements */
	MMWFD_M_ASRC,
	MMWFD_M_AQ,			/* aideo queue */
	MMWFD_M_AENC,		/* aideo encoder */
	MMWFD_M_MUX,
	MMWFD_M_PAY,
	MMWFD_M_RTPBIN,
	MMWFD_M_SINK,

};


typedef struct
{
	int id;
	GstElement *gst;
} MMWfdGstElement;

typedef struct
{
	GstTagList		*tag_list;
	MMWfdGstElement 	*mainbin; /* Main bin */
	MMWfdGstElement 	*asrcbin; /* audio source bin */
	MMWfdGstElement 	*vsrcbin; /* video source bin */
} MMWfdGstPipelineInfo;

typedef struct {
	/* command lock */
	GMutex* cmd_lock;
	int cmd;

	/* STATE */
	MMWfdStateType state;			// wfd current state
	MMWfdStateType prev_state;		// wfd  previous state
	// TODO: is it required to maintain pending & target states
	MMWfdStateType pending_state;		// wfd  state which is going to now
	MMWfdStateType target_state;		// wfd  state which user want to go to

	MMWfdGstPipelineInfo	*pipeline;
	void *server;
	void *client;

	/* player attributes */
	MMHandleType attrs;

	/* message callback */
	MMMessageCallback msg_cb;
	void* msg_cb_param;

	void *mapping;
	void *factory;

}mm_wfd_t;

int _mmwfd_create (MMHandleType hwfd);
int _mmwfd_destroy (MMHandleType hwfd);
int _mmwfd_realize (MMHandleType hwfd);
int _mmwfd_unrealize (MMHandleType hwfd);
int _mmwfd_connect (MMHandleType hwfd);
int _mmwfd_start (MMHandleType hwfd);
int _mmwfd_stop (MMHandleType hwfd);
int _mmwfd_pause (MMHandleType hwfd);
int _mmwfd_resume (MMHandleType hwfd);
int _mmwfd_get_state(MMHandleType hwfd, int* state);
gboolean __mmwfd_post_message(mm_wfd_t* wfd, enum MMMessageType msgtype, MMMessageParamType* param);
int _mmwfd_set_message_callback(MMHandleType hwfd, MMMessageCallback callback, gpointer user_param);
#endif

