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

#ifndef _WFDSOURCEPROXY_H_
#define _WFDSOURCEPROXY_H_

/*******************
  * Allow for C++ users
  */
#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include <glib.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>
#include <mm_debug.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/**
 * Enumerations of wifi-display states.
 */
typedef enum {
  WFDSRC_STATE_NULL = 0,                         /**< wifi-display is created, but not realized yet */
  WFDSRC_STATE_READY,                      /**< wifi-display is ready to play media */
  WFDSRC_STATE_CONNECTION_WAIT, /** < wifi-display is waiting for connection */
  WFDSRC_STATE_CONNECTED,             /** < wifi-display is connected */
  WFDSRC_STATE_PLAYING,                  /**< wifi-display is now playing media */
  WFDSRC_STATE_PAUSED,                   /**< wifi-display is paused while playing media */
  WFDSRC_STATE_NONE,                       /**< wifi-display is not created yet */
  WFDSRC_STATE_NUM                        /**< Number of wifi-display states */
} WfdSrcProxyState;

typedef enum {
  WFDSRC_COMMAND_NONE = 0,
  WFDSRC_COMMAND_CREATE,
  WFDSRC_COMMAND_DESTROY,
  WFDSRC_COMMAND_REALIZE,
  WFDSRC_COMMAND_UNREALIZE,
  WFDSRC_COMMAND_CONNECT,
  WFDSRC_COMMAND_START,
  WFDSRC_COMMAND_STOP,
  WFDSRC_COMMAND_PAUSE,
  WFDSRC_COMMAND_RESUME,
  WFDSRC_COMMAND_NUM,
}WfdSrcProxyCmd;

/**
 * Enumerations of wifi-display source module errors and proxy errors
 * Note: Ensure appending of proxy errors after WFD source module errors
 */
typedef enum {
  WFDSRC_ERROR_NONE = 0,
  WFDSRC_ERROR_UNKNOWN,
  WFDSRC_ERROR_WFD_INVALID_ARGUMENT,
  WFDSRC_ERROR_WFD_NO_FREE_SPACE,
  WFDSRC_ERROR_WFD_NOT_INITIALIZED,
  WFDSRC_ERROR_WFD_NO_OP,
  WFDSRC_ERROR_WFD_INVALID_STATE,
  WFDSRC_ERROR_WFD_INTERNAL
} WfdSrcProxyRet;

/**
  * Application callback,
  */
typedef void (*WfdSrcProxyStateError_cb) (MMHandleType pHandle,
                                   WfdSrcProxyRet error_code,
                                   WfdSrcProxyState state,
                                   void *user_data);

/*******************************************************
  * Function Name:  WfdSrcProxyInit
  * =========
  * Description:
  * =======
  * This API does the following
  *   - creates handle
  *   - creates socket and connect to server address ( IPC)
  *        - This ensures socket listen on server to be successful
  *   - call back registration
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *    appCb - pointer to application callback
  *
  * Return : return WfdSrcProxyRet
  * ====
  *******************************************************/
WfdSrcProxyRet WfdSrcProxyInit(
    MMHandleType *pHandle,
    WfdSrcProxyStateError_cb *appCb,
    void *user_data );


/*******************************************************
  * Function Name:  WfdSrcProxyDeInit
  * =========
  * Description:
  * =======
  *  - Free the handle
  *  - close the sockets
  *
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return WfdSrcProxyRet
  * ====
  *******************************************************/
WfdSrcProxyRet WfdSrcProxyDeInit(MMHandleType pHandle );


/*******************************************************
  * Function Name:  WfdSrcProxySetIPAddrAndPort
  * =========
  * Description:
  * =======
  * This API sets IP address and port number to be used by WFD source.
  * Application should call this API after Initialize is called.
  * IP address and port number is sent to WFD source Server (daemon)
  * on IPC
  *
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *    wfdsrcIP - IP address string
  *    wfdsrcPort : Port no string
  *
  * Return : return WfdSrcProxyRet
  * ====
  ******************************************************/
WfdSrcProxyRet WfdSrcProxySetIPAddrAndPort(
     MMHandleType pHandle,
     char *wfdsrcIP,
     char *wfdsrcPort);

/*******************************************************
  * Function Name:  WfdSrcProxyConnect
  * =========
  * Description:
  * =======
  * This API is used to connect WiFi display source to client.
  * After return, display mode should be set to X
  * refer to utilx_scrnconf_set_dispmode()
  *
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return WfdSrcProxyRet
  * ====
  ******************************************************/
WfdSrcProxyRet WfdSrcProxyConnect(MMHandleType pHandle);

/*******************************************************
  * Function Name:  WfdSrcProxyStart
  * =========
  * Description:
  * =======
  * This API is used to start WiFi display source to start sending data
  *  to client.
  *
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return WfdSrcProxyRet
  * ====
  ******************************************************/
WfdSrcProxyRet WfdSrcProxyStart(MMHandleType pHandle);

/*******************************************************
  * Function Name:  WfdSrcProxyPause
  * =========
  * Description:
  * =======
  * This API is used to pause WFD streaming between source and sink.
  * This pauses the streaming between WFD source and sink. So that
  * when user resumes back, streaming does not continue from the point
  * where it stopped, instead it will start from current content.
  *
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return WfdSrcProxyRet
  * ====
  ******************************************************/
WfdSrcProxyRet WfdSrcProxyPause(MMHandleType pHandle);

/*******************************************************
  * Function Name:  WfdSrcProxyResume
  * =========
  * Description:
  * =======
  * This API is used to resume WFD streaming between source and sink
  * Resume after pause starts from current content that is viewed on source
  * No caching of content from the time it is paused is done.
  *
  * Arguments:
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return WfdSrcProxyRet
  * ====
  ******************************************************/
WfdSrcProxyRet WfdSrcProxyResume(MMHandleType pHandle);

/*******************************************************
  * Function Name:  WfdSrcProxyStop
  * =========
  * Description:
  * =======
  * This API stops WFD streaming between source and sink.
  * The Server(daemon) will be still running even after stop.
  *
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return one of WfdSrcProxyRet
  * ====
  ******************************************************/
WfdSrcProxyRet WfdSrcProxyStop(MMHandleType pHandle);

/*******************************************************
  * Function Name:  WfdSrcProxyDestroyServer
  * =========
  * Description:
  * =======
  * This API destroy WFD server which is already in STOP state.
  * The Server(daemon) will be destroyed after this call.
  *
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return one of WfdSrcProxyRet
  * ====
  ******************************************************/
WfdSrcProxyRet WfdSrcProxyDestroyServer(MMHandleType pHandle);

/*******************************************************
  * Function Name:  WfdSrcProxyGetCurrentState
  * =========
  * Description:
  * =======
  * This API is a provision given to Application, if in any case
  * application needs to know the status of WiFi Source server state.
  * This is a synchronous call.
  * Arguments:
  * =======
  *    pHandle - pointer to WfdSrcProxyInfo
  *
  * Return : return one of the states from WfdSrcProxyState
  * ====
  ******************************************************/
WfdSrcProxyState WfdSrcProxyGetCurrentState(MMHandleType pHandle);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_WFDSOURCEPROXY_H_
