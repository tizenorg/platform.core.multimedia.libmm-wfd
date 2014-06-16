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

#include "mm_wfd_proxy.h"
#include <gst/gst.h>
#include <fcntl.h>

#include <dbus/dbus.h>
#include <dbus/dbus-shared.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "wfd-stub.h"
#include <dlog.h>

#define DEST_HOST "127.0.0.1"
#define WFD_PROXY_COMM_PORT 8888
#define WFD_TIME_SPAN_100_MILLISECOND (100 * G_TIME_SPAN_MILLISECOND)
#define LIMIT_TIME 50 /* 5 sec */

/*WFD_PROXY_SET_IP_PORT
IP 192.168.16.1
PORT 2022
*/
/* REPLAY WFD_PROXY_SET_IP_PORT
MM_ERROR_NONE
*/
// WFD_PROXY_START
/* REPLAY WFD_PROXY_START
MM_ERROR_NONE
*/
// WFD_PROXY_STOP
/* REPLAY WFD_PROXY_STOP
MM_ERROR_NONE
*/
// WFD_PROXY_PAUSE
/* REPLAY WFD_PROXY_PAUSE
MM_ERROR_NONE
*/
// WFD_PROXY_RESUME
/* REPLAY WFD_PROXY_RESUME
MM_ERROR_NONE
*/
//WFD_PROXY_DESTROY
/* REPLAY WFD_PROXY_DESTROY
MM_ERROR_NONE
*/
// WFD_PROXY_STATE_QUERY
/* REPLAY WFD_PROXY_STATE_QUERY
MM_WFD_STATE_NULL
MM_WFD_STATE_READY
MM_WFD_STATE_PLAYING
MM_WFD_STATE_NONE
*/

/* NOTIFY
MM_WFD_STATE_READY
WFDSRC_ERROR_UNKNOWN
*/

/**
 * Data structure to hold call back info for proxy to send asynchronous
 * info received from WFD daemon
 */
typedef struct{
  WfdSrcProxyStateError_cb applicationCb;
  void *user_data;
  GCond *cond;
  GMutex *cond_lock;
  char outbuff[512];
  char inbuff[512];
  int sockfd;
  gboolean quitloop;
  GThread *thread;
  gboolean response;
  gboolean server_destroyed;
} WfdSrcProxy;

static int wfd_proxy_initialize(WfdSrcProxy *wfd);
static WfdSrcProxyState wfd_proxy_message_parse_state (const char * data, guint size);
static WfdSrcProxyRet wfd_proxy_message_parse_status (const char * data, guint size);
static gboolean is_cmd_valid_in_current_state(WfdSrcProxyState aState, WfdSrcProxyCmd acmd);
static gboolean proxy_write (WfdSrcProxy *wfd, char *wbuf);
WfdSrcProxyState convert_string_to_state(gchar *buffer);
WfdSrcProxyRet convert_string_to_status(gchar *buffer);

WfdSrcProxyRet WfdSrcProxyInit(
    MMHandleType *pHandle,
    WfdSrcProxyStateError_cb appCb,
    void *user_data )
{
  WfdSrcProxy *temp = NULL;

  debug_fenter();
  debug_log("mm_wfd_proxy_create \n");

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_INVALID_ARGUMENT);
  int ret = 0;
  temp = g_malloc(sizeof(WfdSrcProxy));
  if(!temp)
  {
    debug_log("WfdSrcProxy malloc failed. out of memory");
    return WFDSRC_ERROR_WFD_NO_FREE_SPACE;
  }
  temp->cond = g_cond_new ();
  temp->cond_lock = g_mutex_new ();
  temp->applicationCb = appCb;
  temp->user_data = user_data;
  temp->quitloop = FALSE;
  temp->response = FALSE;
  temp->server_destroyed = FALSE;
  ret = wfd_proxy_initialize(temp);
  if(ret < 0)
    return WFDSRC_ERROR_WFD_NOT_INITIALIZED;
  *pHandle = (MMHandleType)temp;
  debug_fleave ();
  return WFDSRC_ERROR_NONE;
}

WfdSrcProxyRet WfdSrcProxyDeInit(MMHandleType pHandle )
{
  WfdSrcProxy *lwfd;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  lwfd->quitloop = TRUE;
  debug_log("client close socket\n");
  shutdown (lwfd->sockfd, SHUT_RDWR);
  if(!lwfd->server_destroyed)
    g_thread_join(lwfd->thread);
  if(lwfd->cond_lock) g_mutex_free(lwfd->cond_lock);
  if(lwfd->cond) g_cond_free(lwfd->cond);
  debug_log("client after thread join\n");
  g_free(lwfd);
  debug_leave();
  return WFDSRC_ERROR_NONE;
}

WfdSrcProxyRet WfdSrcProxySetIPAddrAndPort(
     MMHandleType pHandle,
     char *wfdsrcIP,
     char *wfdsrcPort)
{
  WfdSrcProxyState lstate;
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  return_val_if_fail(wfdsrcIP, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  return_val_if_fail(wfdsrcPort, WFDSRC_ERROR_WFD_NOT_INITIALIZED);

  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lstate = WfdSrcProxyGetCurrentState(pHandle);
  if(!is_cmd_valid_in_current_state(lstate, WFDSRC_COMMAND_STOP))
    return WFDSRC_ERROR_WFD_INVALID_STATE;
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_SET_IP_PORT");
  g_string_append_printf (proxy_cmd, "\r\nIP ");
  g_string_append_printf (proxy_cmd, "%s", wfdsrcIP);
  g_string_append_printf (proxy_cmd, "\r\nPORT ");
  g_string_append_printf (proxy_cmd, "%s", wfdsrcPort);
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxySetIPAddrAndPort command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }

  debug_log("WfdSrcProxySetIPAddrAndPort replay for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_status(lwfd->inbuff, strlen(lwfd->inbuff));
}

WfdSrcProxyRet WfdSrcProxyConnect(MMHandleType pHandle)
{
  WfdSrcProxyState lstate;
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lstate = WfdSrcProxyGetCurrentState(pHandle);
  if(!is_cmd_valid_in_current_state(lstate, WFDSRC_COMMAND_CONNECT))
    return WFDSRC_ERROR_WFD_INVALID_STATE;
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_CONNECT");
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxyConnect command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }

  debug_log("WfdSrcProxyConnect inbuff for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_status(lwfd->inbuff, strlen(lwfd->inbuff));
}

WfdSrcProxyRet WfdSrcProxyStart(MMHandleType pHandle)
{
  WfdSrcProxyState lstate;
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lstate = WfdSrcProxyGetCurrentState(pHandle);
  if(!is_cmd_valid_in_current_state(lstate, WFDSRC_COMMAND_START))
    return WFDSRC_ERROR_WFD_INVALID_STATE;
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_START");
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxyStart command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }

  debug_log("WfdSrcProxyStart inbuff for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_status(lwfd->inbuff, strlen(lwfd->inbuff));
}

WfdSrcProxyRet WfdSrcProxyPause(MMHandleType pHandle)
{
  WfdSrcProxyState lstate;
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lstate = WfdSrcProxyGetCurrentState(pHandle);
  if(!is_cmd_valid_in_current_state(lstate, WFDSRC_COMMAND_PAUSE))
    return WFDSRC_ERROR_WFD_INVALID_STATE;
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_PAUSE");
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxyPause command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }

  debug_log("WfdSrcProxyPause inbuff for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_status(lwfd->inbuff, strlen(lwfd->inbuff));
}

WfdSrcProxyRet WfdSrcProxyResume(MMHandleType pHandle)
{
  WfdSrcProxyState lstate;
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lstate = WfdSrcProxyGetCurrentState(pHandle);
  if(!is_cmd_valid_in_current_state(lstate, WFDSRC_COMMAND_RESUME))
    return WFDSRC_ERROR_WFD_INVALID_STATE;
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_RESUME");
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxyResume command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }

  debug_log("WfdSrcProxyResume inbuff for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_status(lwfd->inbuff, strlen(lwfd->inbuff));
}

WfdSrcProxyRet WfdSrcProxyStop(MMHandleType pHandle)
{
  WfdSrcProxyState lstate;
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lstate = WfdSrcProxyGetCurrentState(pHandle);
  if(!is_cmd_valid_in_current_state(lstate, WFDSRC_COMMAND_STOP))
    return WFDSRC_ERROR_WFD_INVALID_STATE;
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_STOP");
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxyStop command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }

  debug_log("WfdSrcProxyStop inbuff for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_status(lwfd->inbuff, strlen(lwfd->inbuff));
}

WfdSrcProxyRet WfdSrcProxyDestroyServer(MMHandleType pHandle)
{
  WfdSrcProxyState lstate;
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lstate = WfdSrcProxyGetCurrentState(pHandle);
  if(!is_cmd_valid_in_current_state(lstate, WFDSRC_COMMAND_STOP))
    return WFDSRC_ERROR_WFD_INVALID_STATE;
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_DESTROY");
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxyDestroyServer command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }

  debug_log("WfdSrcProxyDestroyServer inbuff for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_status(lwfd->inbuff, strlen(lwfd->inbuff));
}

WfdSrcProxyState WfdSrcProxyGetCurrentState(MMHandleType pHandle)
{
  WfdSrcProxy *lwfd;
  GString *proxy_cmd;
  gchar *cmd_string;
  gint64 end_time;
  int limit = 0;
  debug_fenter();

  return_val_if_fail(pHandle, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  lwfd = (WfdSrcProxy *)pHandle;
  return_val_if_fail(!lwfd->server_destroyed, WFDSRC_ERROR_WFD_NOT_INITIALIZED);
  proxy_cmd = g_string_new ("");
  g_string_append_printf (proxy_cmd, "WFD_PROXY_STATE_QUERY");
  g_string_append_printf (proxy_cmd, "\r\n");
  cmd_string = g_string_free (proxy_cmd, FALSE);
  debug_log("WfdSrcProxyGetCurrentState command sent: %s \n", cmd_string);
  lwfd->response = FALSE;
  proxy_write(lwfd, cmd_string);
retry:
  debug_log("try timed wait...\n");
  end_time = g_get_monotonic_time () + WFD_TIME_SPAN_100_MILLISECOND;
  if (!g_cond_wait_until(lwfd->cond, lwfd->cond_lock, end_time))
  {
    debug_log("Out of timed wait but due to timeout...\n");
    limit ++;
    if(limit > LIMIT_TIME) return WFDSRC_ERROR_WFD_INTERNAL;
    if(!lwfd->response) goto retry;
  }
  debug_log("WfdSrcProxyGetCurrentState inbuff for command: %s \n", lwfd->inbuff);
  debug_leave();
  return wfd_proxy_message_parse_state(lwfd->inbuff, strlen(lwfd->inbuff));
}

static gboolean proxy_write (WfdSrcProxy *wfd, char *wbuf)
{
  write(wfd->sockfd, wbuf, strlen(wbuf));
  return TRUE;
}

static void* wfd_proxy_thread_function(void * asrc)
{
  fd_set read_flags,write_flags;
  WfdSrcProxy *wfd = (WfdSrcProxy *)asrc;
  int rc;
  while(!wfd->quitloop)
  {
    FD_ZERO(&read_flags);
    FD_ZERO(&write_flags);
    FD_SET(wfd->sockfd, &read_flags);
    debug_log("Waiting on select()...\n");
    rc = select(wfd->sockfd+1, &read_flags, NULL, NULL, NULL);
    if(rc < 0)
      continue;
    if(FD_ISSET(wfd->sockfd, &read_flags))
    { //Socket ready for reading
      FD_CLR(wfd->sockfd, &read_flags);
      memset(&(wfd->inbuff),0,sizeof(wfd->inbuff));
      debug_log("socket ready for reading...\n");
      rc = read(wfd->sockfd, wfd->inbuff, sizeof(wfd->inbuff)-1);
      if (rc <= 0)
      {
        debug_log("socket connection closed\n");
        wfd->server_destroyed = TRUE;
        goto cleanup;
      }
      debug_log("wfd_proxy replay for command: %s \n", wfd->inbuff);
      if(g_strrstr(wfd->inbuff, "NOTIFY"))
      {
        if(wfd->applicationCb)
        {
          gchar **notify;
          notify = g_strsplit(wfd->inbuff,"\r\n",0);
          wfd->applicationCb(asrc, convert_string_to_status(notify[2]), convert_string_to_state(notify[1]), wfd->user_data);
        }
      }
      else 
      {
        debug_log("wfd_proxy signalling \n");
        wfd->response = TRUE;
        g_cond_signal(wfd->cond);
        debug_log("wfd_proxy signal emitted \n");
      }
    }
  }
cleanup:
  close (wfd->sockfd);
  FD_CLR (wfd->sockfd, &read_flags);
  debug_log("thread function signal\n");
  g_cond_signal(wfd->cond);
  debug_log("thread function quit\n");
  if((!wfd->quitloop) && wfd->applicationCb)
  {
    debug_log("sending notification to proxytest");
    wfd->applicationCb(asrc, WFDSRC_ERROR_WFD_INVALID_STATE, WFDSRC_STATE_NULL, wfd->user_data);
  }
  return NULL;
}

static int wfd_proxy_initialize(WfdSrcProxy *wfd)
{
  DBusGConnection *bus = NULL;
  DBusGProxy *proxy = NULL;
  g_type_init();
  debug_log("wfd_proxy_initialize init \n");
  int portno;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  GError *error = NULL;
  debug_log("wfd_proxy_initialize get before socket\n");
  wfd->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (wfd->sockfd < 0)
  {
    debug_log("ERROR opening socket\n");
    return -1;
  }

  debug_log("wfd_proxy_initialize get socket created\n");
  server = gethostbyname(DEST_HOST);
  if (server == NULL) {
    debug_log("ERROR, no such host\n");
    return -1;
  }
  portno = WFD_PROXY_COMM_PORT;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  debug_log("wfd_proxy_initialize bcopy\n");
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);
  debug_log("wfd_proxy_initialize get socket before connect\n");
  if (connect(wfd->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
  {
    int return_code = 0;
    int pid = getpid();
    debug_log("-----------  wfd_proxy_initialize socket connect failed it means server is not yet started ------------\n");
    debug_log("going to start WFD server \n");
    bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
    if (bus == NULL) {
      debug_log("dbus bus get failed\n");
      return -1;
    }
    proxy = dbus_g_proxy_new_for_name(bus,
              "com.samsung.wfd.server",
              "/com/samsung/wfd/server",
              "com.samsung.wfd.server");
    if (proxy == NULL) {
      debug_log("dbus bus proxy get failed\n");
      return -1;
    }
    if (!com_samsung_wfd_server_test_method(proxy, pid, "miracast_server", &return_code, &error)) {
      debug_log(
      "com_samsung_wfd_server_test_method()failed.test_name[%s], "
      "return_code[%d]\n", "miracast_server", return_code);
      debug_log("error->message is %s\n", error->message);
    }
    sleep(1);
    debug_log("wfd_proxy_initialize trying for connect\n");
    if (connect(wfd->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
      debug_log("wfd_proxy_initialize trying to connect failed\n");
      return -1;
    }
  }
  debug_log("WFD server initiated and connected\n");
  wfd->thread = g_thread_create ((GThreadFunc) wfd_proxy_thread_function, wfd, TRUE, &error);
  return 0;
}

static gboolean is_cmd_valid_in_current_state(WfdSrcProxyState aState, WfdSrcProxyCmd acmd)
{
  //TODO state checking needs to be implemented
  return TRUE;
}

WfdSrcProxyState convert_string_to_state(gchar *buffer)
{
  g_return_val_if_fail (buffer != NULL, WFDSRC_STATE_NUM);
  if(!strcmp(buffer, "MM_WFD_STATE_NULL")) return WFDSRC_STATE_NULL;
  if(!strcmp(buffer, "MM_WFD_STATE_READY")) return WFDSRC_STATE_READY;
  if(!strcmp(buffer, "MM_WFD_STATE_CONNECTION_WAIT")) return WFDSRC_STATE_CONNECTION_WAIT;
  if(!strcmp(buffer, "MM_WFD_STATE_CONNECTED")) return WFDSRC_STATE_CONNECTED;
  if(!strcmp(buffer, "MM_WFD_STATE_PLAYING")) return WFDSRC_STATE_PLAYING;
  if(!strcmp(buffer, "MM_WFD_STATE_PAUSED")) return WFDSRC_STATE_PAUSED;
  if(!strcmp(buffer, "MM_WFD_STATE_NONE")) return WFDSRC_STATE_NONE;
  return WFDSRC_STATE_NULL;
}

WfdSrcProxyRet convert_string_to_status(gchar *buffer)
{
  g_return_val_if_fail (buffer != NULL, WFDSRC_ERROR_UNKNOWN);
  if(!strcmp(buffer, "MM_ERROR_NONE")) return WFDSRC_ERROR_NONE;
  if(!strcmp(buffer, "MM_ERROR_UNKNOWN")) return WFDSRC_ERROR_UNKNOWN;
  if(!strcmp(buffer, "MM_ERROR_WFD_INVALID_ARGUMENT")) return WFDSRC_ERROR_WFD_INVALID_ARGUMENT;
  if(!strcmp(buffer, "MM_ERROR_WFD_NO_FREE_SPACE")) return WFDSRC_ERROR_WFD_NO_FREE_SPACE;
  if(!strcmp(buffer, "MM_ERROR_WFD_NOT_INITIALIZED")) return WFDSRC_ERROR_WFD_NOT_INITIALIZED;
  if(!strcmp(buffer, "MM_ERROR_WFD_NO_OP")) return WFDSRC_ERROR_WFD_NO_OP;
  if(!strcmp(buffer, "MM_ERROR_WFD_INVALID_STATE")) return WFDSRC_ERROR_WFD_INVALID_STATE;
  if(!strcmp(buffer, "MM_ERROR_WFD_INTERNAL")) return WFDSRC_ERROR_WFD_INTERNAL;
  return WFDSRC_ERROR_UNKNOWN;
}

static WfdSrcProxyState wfd_proxy_message_parse_state (const char * data, guint size)
{
  gchar *p;
  gchar buffer[128];
  guint idx = 0;
  gboolean statequery = false;
  g_return_val_if_fail (data != NULL, WFDSRC_STATE_NUM);
  g_return_val_if_fail (size != 0, WFDSRC_STATE_NUM);

  p = (gchar *) data;
  while (TRUE) {
    if (*p == '\0')
    break;
    idx = 0;
    while (*p != '\n' && *p != '\r' && *p != '\0') {
      if (idx < sizeof (buffer) - 1)
        buffer[idx++] = *p;
      p++;
    }
    buffer[idx] = '\0';
    if(g_strrstr(buffer, "REPLAY WFD_PROXY_STATE_QUERY")) statequery = TRUE;
    else if(statequery) return convert_string_to_state(buffer);
    if (*p == '\0')
      break;
    p+=2;
  }
  return WFDSRC_STATE_NUM;
}

WfdSrcProxyRet wfd_proxy_message_parse_status (const char * data, guint size)
{
  gchar *p;
  gchar buffer[128];
  guint idx = 0;
  gboolean replay = false;
  g_return_val_if_fail (data != NULL, WFDSRC_ERROR_UNKNOWN);
  g_return_val_if_fail (size != 0, WFDSRC_ERROR_UNKNOWN);

  p = (gchar *) data;
  while (TRUE) {
    if (*p == '\0')
    break;
    idx = 0;
    while (*p != '\n' && *p != '\r' && *p != '\0') {
      if (idx < sizeof (buffer) - 1)
        buffer[idx++] = *p;
      p++;
    }
    buffer[idx] = '\0';
    if(g_strrstr(buffer, "REPLAY")) replay = TRUE;
    else if(replay) return convert_string_to_status(buffer);
    if (*p == '\0')
      break;
    p+=2;
  }
  return WFDSRC_ERROR_UNKNOWN;
}
