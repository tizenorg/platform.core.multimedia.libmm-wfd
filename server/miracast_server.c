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

 /*===========================================================================================
|                                             |
|  INCLUDE FILES                                      |
|                                               |
========================================================================================== */
//#define MTRACE;
#include <glib.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_wfd.h>
#include <mm_debug.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>

#include <iniparser.h>
#include <mm_ta.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>

#include <netdb.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <vconf.h>

#define PACKAGE "mm_wfd_testsuite"

/*===========================================================================================
|                                             |
|  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE                      |
|                                               |
========================================================================================== */

#if defined(_USE_GMAINLOOP)
GMainLoop *g_loop;
#endif

gboolean wfd_server_test_method(void *pObject, int pid, char *test_name,
         int *return_code)
{
  debug_log("Received test %s\n", test_name);
  return TRUE;
}

#include "wfd-structure.h"

#define WFD_PROXY_COMM_PORT 8888

typedef struct WfdServerObject WfdServerObject;
typedef struct WfdServerObjectClass WfdServerObjectClass;

GType Daemon_Object_get_type(void);
struct WfdServerObject {
  GObject parent;
};

struct WfdServerObjectClass {
  GObjectClass parent;
};

#define DBUS_NAME_FLAG_PROHIBIT_REPLACEMENT 0

#define WFD_SERVER_TYPE_OBJECT (Daemon_Object_get_type())

#define WFD_SERVER_OBJECT(object) (G_TYPE_CHECK_INSTANCE_CAST \
  ((object), WFD_SERVER_TYPE_OBJECT, WfdServerObject))

G_DEFINE_TYPE(WfdServerObject, Daemon_Object, G_TYPE_OBJECT)
static void Daemon_Object_init(WfdServerObject * obj)
{
  debug_log("Daemon_Object_init\n");
}

static void Daemon_Object_class_init(WfdServerObjectClass * klass)
{
  debug_log("Daemon_Object_class_init\n");
}

/*---------------------------------------------------------------------------
|    LOCAL #defines:                            |
---------------------------------------------------------------------------*/
#define MAX_STRING_LEN    2048

/*---------------------------------------------------------------------------
|    LOCAL CONSTANT DEFINITIONS:                      |
---------------------------------------------------------------------------*/
enum
{
  CURRENT_STATUS_MAINMENU,
  CURRENT_STATUS_SERVER_IP,
  CURRENT_STATUS_SERVER_PORT,
  CURRENT_STATUS_CONNECT,
};

typedef struct {
  int connfd;
  socklen_t clilen;
  struct sockaddr_in cli_addr;
  GThread *thread;
  char inbuff[512];
  gboolean inactive;
  GMutex *lock;
  GIOChannel *channel;
  void *parent;
} WFDClient;

typedef struct {
  int sockfd;
  int portno;
  struct sockaddr_in serv_addr;
  GThread *thread;
  GList *clients;
} WFDServer;

/*---------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS:                      |
---------------------------------------------------------------------------*/

int  g_current_state;
static MMHandleType g_wfd = 0;
char *g_err_name = NULL;
gboolean quit_pushing;
int socket_id = 0;
/*---------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:                       |
---------------------------------------------------------------------------*/
static void wfd_connect();
static void wfd_start();
static void wfd_stop();

void quit_program();

static void wfd_set_signal();

/*---------------------------------------------------------------------------
  |    LOCAL FUNCTION DEFINITIONS:                      |
  ---------------------------------------------------------------------------*/
static bool msg_callback(int message, MMMessageParamType *param, void *user_param)
{
  /* TODO any error notification or state change should be forwarded to the active list of proxy apps */
  switch (message) {
    case MM_MESSAGE_ERROR:
      {
        WFDClient *client = (WFDClient *)user_param;
        if(!client) return FALSE;
        WFDServer *lserver = (WFDServer*) client->parent;
        if(!lserver) return FALSE;
        debug_log("msg_callback error : code = %x\n", param->code);
        debug_log ("DESTROY..\n\n");
        quit_pushing = TRUE;
        shutdown (lserver->sockfd, SHUT_RDWR);
	    debug_log("msg_callback error call quit_program()");
        quit_program();
      }
      break;
    case MM_MESSAGE_WARNING:
      // debug_log("warning : code = %d\n", param->code);
      break;
    case MM_MESSAGE_END_OF_STREAM:
      debug_log("msg_callback end of stream\n");
      mm_wfd_stop(g_wfd);
      break;
    case MM_MESSAGE_STATE_CHANGED:
      g_current_state = param->state.current;
      switch(g_current_state)
      {
        case MM_WFD_STATE_NULL:
          debug_log("\n                                                            ==> [WFDApp] Player is [NULL]\n");
          break;
        case MM_WFD_STATE_READY:
          debug_log("\n                                                            ==> [WFDApp] Player is [READY]\n");
          break;
        case MM_WFD_STATE_PLAYING:
          debug_log("\n                                                            ==> [WFDApp] Player is [PLAYING]\n");
          break;
        case MM_WFD_STATE_PAUSED:
          debug_log("\n                                                            ==> [WFDApp] Player is [PAUSED]\n");
          break;
      }
      break;
    default:
      return FALSE;
  }
  return TRUE;
}

static gboolean proxy_write (WFDClient *client, char *wbuf)
{
  write(client->connfd, wbuf, strlen(wbuf));
  return TRUE;
}

static void input_server_ip_and_port(char *server_ip, char* port, WFDClient *client)
{
  int len = strlen(server_ip);
  int ret = MM_ERROR_NONE;
  if ( len < 0 || len > MAX_STRING_LEN )
    return;

  if (!g_wfd)
  {
    if ( mm_wfd_create(&g_wfd) != MM_ERROR_NONE )
    {
      debug_log("input_server_ip wfd create is failed\n");
      return;
    }
    ret = mm_wfd_set_message_callback(g_wfd, msg_callback, (void*)client);
    if (ret != MM_ERROR_NONE)
    {
      debug_log ("input_server_ip Error in setting server_port...\n");
      return;
    }
  }
  ret = mm_wfd_set_attribute(g_wfd,
        &g_err_name,
        "server_ip", server_ip, strlen(server_ip),
        NULL);
  if (ret != MM_ERROR_NONE)
  {
    debug_log ("input_server_ip Error in setting server_port...\n");
    return;
  }
  ret = mm_wfd_set_attribute(g_wfd,
          &g_err_name,
          "server_port", port, strlen(port),
          NULL);
  if (ret != MM_ERROR_NONE)
  {
    debug_log ("input_server_port Error in setting server_port...\n");
    return;
  }
  /*set wfd source status*/
  gboolean status = VCONFKEY_MIRACAST_WFD_SOURCE_ON;
  if (!vconf_set_bool(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS, status)) {
    debug_log ("set VCONFKEY_MIRACAST_WFD_SOURCE_ON");
  }
}
static void wfd_connect()
{
  int ret = MM_ERROR_NONE;

  if (!g_wfd)
  {
    debug_log ("wfd_start Creating server with default values : ");
    if ( mm_wfd_create(&g_wfd) != MM_ERROR_NONE )
    {
      debug_log("wfd_start wfd create is failed\n");
      return;
    }

    ret = mm_wfd_set_message_callback(g_wfd, msg_callback, (void*)g_wfd);
    if (ret != MM_ERROR_NONE)
    {
      debug_log ("wfd_start Error in setting server_port...\n");
      return;
    }
  }
  if ( mm_wfd_realize(g_wfd) != MM_ERROR_NONE )
  {
    debug_log("wfd_start wfd realize is failed\n");
    return;
  }

  if ( mm_wfd_connect(g_wfd) != MM_ERROR_NONE )
  {
    debug_log("wfd_start wfd connect is failed\n");
    return;
  }

}

static void wfd_start()
{
  int ret = MM_ERROR_NONE;

  if (!g_wfd)
  {
    debug_log ("wfd_start Creating server with default values : ");
    if ( mm_wfd_create(&g_wfd) != MM_ERROR_NONE )
    {
      debug_log("wfd_start wfd create is failed\n");
      return;
    }

    ret = mm_wfd_set_message_callback(g_wfd, msg_callback, (void*)g_wfd);
    if (ret != MM_ERROR_NONE)
    {
      debug_log ("wfd_start Error in setting server_port...\n");
      return;
    }

   if ( mm_wfd_realize(g_wfd) != MM_ERROR_NONE )
   {
     debug_log("wfd_start wfd realize is failed\n");
     return;
   }

   if ( mm_wfd_connect(g_wfd) != MM_ERROR_NONE )
   {
     debug_log("wfd_start wfd connect is failed\n");
     return;
    }
  }

  if (mm_wfd_start(g_wfd) != MM_ERROR_NONE)
  {
    debug_log ("wfd_start Failed to start WFD");
    return;
  }
}

static void wfd_stop()
{
  int ret = MM_ERROR_NONE;

  ret = mm_wfd_stop(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    debug_log ("wfd_stop Error to do stop...\n");
    return;
  }
}

static void wfd_pause()
{
  int ret = MM_ERROR_NONE;

  ret = mm_wfd_pause(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    debug_log ("wfd_pause Error to do pausep...\n");
    return;
  }
}

static void wfd_resume()
{
  int ret = MM_ERROR_NONE;

  ret = mm_wfd_resume(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    debug_log ("wfd_resume Error to do resume...\n");
    return;
  }
}

static void wfd_standby()
{
  int ret = MM_ERROR_NONE;
  ret = mm_wfd_standby(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    debug_log ("wfd_standby Error to do standby...\n");
    return;
  }
}

void quit_program()
{
  MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_STDOUT);
  MMTA_RELEASE();
  debug_log ("quit_program...\n");

  /*set wfd source status*/
  gboolean status = VCONFKEY_MIRACAST_WFD_SOURCE_OFF;
  if (!vconf_set_bool(VCONFKEY_MIRACAST_WFD_SOURCE_STATUS, status)) {
    debug_log ("set VCONFKEY_MIRACAST_WFD_SOURCE_OFF");
  }

  if (g_wfd) {
	mm_wfd_unrealize(g_wfd);
	mm_wfd_destroy(g_wfd);
	g_wfd = 0;
  }
  g_main_loop_quit(g_loop);
}

gchar * convert_state_to_string(MMWfdStateType aState)
{
  GString *cmd_replay;
  cmd_replay = g_string_new ("");
  switch(aState)
  {
    case MM_WFD_STATE_NULL:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_NULL");
      break;
    case MM_WFD_STATE_READY:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_READY");
      break;
    case MM_WFD_STATE_CONNECTION_WAIT:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_CONNECTION_WAIT");
      break;
    case MM_WFD_STATE_CONNECTED:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_CONNECTED");
      break;
    case MM_WFD_STATE_PLAYING:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_PLAYING");
      break;
    case MM_WFD_STATE_PAUSED:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_PAUSED");
      break;
    case MM_WFD_STATE_NONE:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_NONE");
      break;
    case MM_WFD_STATE_NUM:
      g_string_append_printf (cmd_replay, "MM_WFD_STATE_NUM");
      break;
  }
  return g_string_free (cmd_replay, FALSE);
}

static void interpret (WFDClient *client, char *cmd)
{
  GString *cmd_replay;
  if (strstr(cmd, "WFD_PROXY_SET_IP_PORT"))
  {
    gchar **IP_port;
    gchar **IP;
    gchar **port;
    debug_log ("setting attributes... WFD..\n\n");
    IP_port = g_strsplit(cmd,"\r\n",0);
    IP = g_strsplit(IP_port[1]," ",0);
    port = g_strsplit(IP_port[2]," ",0);
    debug_log ("received IP %s port %s\n", IP[1], port[1]);
    input_server_ip_and_port(IP[1], port[1], client);
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_SET_IP_PORT");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, "MM_ERROR_NONE");
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
  }
  else if (strstr(cmd, "WFD_PROXY_CONNECT"))
  {
    debug_log ("Starting... WFD..\n\n");
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_CONNECT");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, "MM_ERROR_NONE");
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
    wfd_connect();
  }
  else if (strstr(cmd, "WFD_PROXY_START"))
  {
    debug_log ("Starting... WFD..\n\n");
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_START");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, "MM_ERROR_NONE");
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
    wfd_start();
  }
  else if (strstr(cmd, "WFD_PROXY_PAUSE"))
  {
    debug_log ("PAUSING..\n\n");
    wfd_pause();
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_PAUSE");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, "MM_ERROR_NONE");
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
  }
  else if (strstr(cmd, "WFD_PROXY_RESUME"))
  {
    debug_log ("RESUMING..\n\n");
    wfd_resume();
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_RESUME");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, "MM_ERROR_NONE");
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
  }
  else if (strstr(cmd, "WFD_PROXY_STOP"))
  {
    debug_log ("STOPPING..\n\n");
    wfd_stop();
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_STOP");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, "MM_ERROR_NONE");
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
  }
  else if (strstr(cmd, "WFD_PROXY_DESTROY"))
  {
    WFDServer *lserver = (WFDServer*) client->parent;
    debug_log ("DESTROY..\n\n");
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_DESTROY");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, "MM_ERROR_NONE");
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
    quit_pushing = TRUE;
    shutdown (lserver->sockfd, SHUT_RDWR);
    debug_log("interpret calll quit_program()");
    quit_program();
  }
  else if (strstr(cmd, "WFD_PROXY_STATE_QUERY"))
  {
    debug_log ("STATE QUERY..\n\n");
    cmd_replay = g_string_new ("");
    g_string_append_printf (cmd_replay, "REPLAY WFD_PROXY_STATE_QUERY");
    g_string_append_printf (cmd_replay, "\r\n");
    g_string_append_printf (cmd_replay, convert_state_to_string(g_current_state));
    proxy_write(client, g_string_free (cmd_replay, FALSE));
    debug_log ("STATE QUERY..demon return\n\n");
  }
  else
  {
    debug_log("unknown menu \n");
  }
}

gboolean input (GIOChannel *channel, GIOCondition condition, gpointer data)
{
  WFDClient *client = (WFDClient*)data;
  gsize read;
  GError *error = NULL;
  if(condition & G_IO_IN) {
    memset(&(client->inbuff),0,sizeof(client->inbuff));
    g_io_channel_read(client->channel, client->inbuff, sizeof(client->inbuff), &read);
    if(read)
    {
      client->inbuff[read] = '\0';
      g_strstrip(client->inbuff);
      debug_log("got command %s\n", client->inbuff);
      if(!g_str_has_prefix(client->inbuff,"WFD")) goto client_cleanup;
      interpret (client, client->inbuff);
      debug_log("command return \n");
    }
    else goto client_cleanup;
  }
  if (condition & G_IO_HUP) goto client_cleanup;
  return TRUE;
client_cleanup:
  {
    WFDServer *lserver = (WFDServer *)client->parent;
    client->inactive = TRUE;
    debug_log("client connection closed \n");
    shutdown (client->connfd, SHUT_RDWR);
    g_io_channel_shutdown(client->channel, TRUE, &error);
    debug_log("IO channel closed \n");
    if(g_list_length(lserver->clients))
    {
      lserver->clients = g_list_remove(lserver->clients, client);
    }
    free(client);
    debug_log("client removed from list \n");
  }
  return TRUE;
}

static void* wfd_server_thread_function(void * asrc)
{
  int  newsockfd;
  int i=0;
  int nSockOpt = 1;
  WFDServer *pserver = (WFDServer *)asrc;
  pserver->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  debug_log("wfd_proxy_initialize socke is %d", pserver->sockfd);
  if (pserver->sockfd < 0)
  {
    debug_log("ERROR opening socket");
    goto cleanup;
  }
  debug_log("wfd_proxy_initialize get socket created\n");
  bzero((char *) &pserver->serv_addr, sizeof(pserver->serv_addr));
  pserver->portno = WFD_PROXY_COMM_PORT;
  pserver->serv_addr.sin_family = AF_INET;
  pserver->serv_addr.sin_addr.s_addr = INADDR_ANY;
  pserver->serv_addr.sin_port = htons(pserver->portno);
  debug_log("serv_addr = %p sizeof %d ", (struct sockaddr *) &pserver->serv_addr, sizeof(pserver->serv_addr));
  setsockopt(pserver->sockfd, SOL_SOCKET, SO_REUSEADDR, &nSockOpt, sizeof(nSockOpt));
  if (bind(pserver->sockfd, (struct sockaddr *) &pserver->serv_addr, sizeof(pserver->serv_addr)) < 0)
  {
    debug_log("ERROR on binding");
    goto cleanup;
  }
  debug_log("wfd_proxy_initialize get socket bind\n");
  if (listen(pserver->sockfd,5) <0)
  {
    debug_log("ERROR on socket listen");
    goto cleanup;
  }
  wfd_set_signal();
  MMTA_INIT();

  while(!quit_pushing)
  {
    socklen_t clilen = {0};
    struct sockaddr_in cli_addr = {0};
    WFDClient *client = NULL;
    clilen = sizeof(cli_addr);
    debug_log("wfd_proxy_initialize waiting for accept \n");
    newsockfd = accept(pserver->sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
      debug_log("ERROR on accept");
      continue;
    }
    debug_log("wfd_proxy_initialize get socket accept \n");
    client = g_malloc(sizeof(WFDClient));
    if(!client)
    {
      debug_log("client malloc failed. out of memory");
      continue;
    }
    client->connfd = newsockfd;
    socket_id = newsockfd;
    client->lock = g_mutex_new ();
    client->inactive = 0;
    client->parent = pserver;
    client->channel = g_io_channel_unix_new(client->connfd);
    g_io_add_watch(client->channel, G_IO_IN|G_IO_HUP|G_IO_ERR, (GIOFunc)input, client);
    pserver->clients = g_list_prepend(pserver->clients, client);
  }
cleanup:
  debug_log("wfd_server_thread_function cleanup \n");
  for(i=0;i<g_list_length(pserver->clients); i++)
  {
    GError *error = NULL;
    WFDClient *tempclient = (WFDClient *)g_list_nth_data(pserver->clients,i);
    if(tempclient) {
      tempclient->inactive = TRUE;
      shutdown (tempclient->connfd, SHUT_RDWR);
      g_io_channel_shutdown(tempclient->channel, TRUE, &error);
      pserver->clients = g_list_remove(pserver->clients, tempclient);
      free(tempclient);
    }
  }
  g_list_free(pserver->clients);
  shutdown (pserver->sockfd, SHUT_RDWR);
  debug_log("wfd_server_thread_function calll quit_program()");
  quit_program();

  debug_log("wfd_server_thread_function THREAD EXIT \n");

  return NULL;
}

static gboolean __func1(void *data)
{
  GError *error = NULL;
  WFDServer *server = (WFDServer *)data;
  debug_log("__func1 enter \n");
  server->thread = g_thread_create ((GThreadFunc) wfd_server_thread_function, server, TRUE, &error);
  server->clients = g_list_alloc ();
  debug_log("__func1 exit \n");
  return FALSE;
}

static bool __wfd_server_setup(void* pserver)
{
  WFDServer *server = (WFDServer *)pserver;
  DBusGConnection *conn = NULL;
  GObject *object = NULL;
  DBusGObjectInfo dbus_glib_wfd_server_object_info;
  GError *err = NULL;
  DBusError derr;
  int ret = 0;
  debug_log("__wfd_server_setup start\n");
  dbus_g_object_type_install_info(WFD_SERVER_TYPE_OBJECT,
    &dbus_glib_wfd_server_object_info);

  conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &err);
  if (NULL == conn) {
    debug_log("Unable to get dbus!");
    return false;
  }
  dbus_error_init(&derr);

  ret = dbus_bus_request_name(dbus_g_connection_get_connection(conn),
    "com.samsung.wfd.server",
    DBUS_NAME_FLAG_PROHIBIT_REPLACEMENT, &derr);

  if (dbus_error_is_set(&derr)) {
    debug_log("dbus_bus_request_name failed with error:%s", derr.message);
    dbus_error_free(&derr);
    return false;
  }
  object = g_object_new(WFD_SERVER_TYPE_OBJECT, NULL);
  if (NULL == object) {
    debug_log("Unable to create new object");
    return false;
  }
  dbus_g_connection_register_g_object(conn, "/com/samsung/wfd/server", G_OBJECT(object));
  g_idle_add(__func1, server);
  debug_log("__wfd_server_setup end\n");
  return true;
}
void wfd_signal_handler(int signo) {
  quit_pushing = TRUE;
  debug_log("wfd_signal_handler calll quit_program()");
  shutdown (socket_id, SHUT_RDWR);
  quit_program();
  exit(1);
}

static void wfd_set_signal()
{

  /*refer to signal.h*/
  /*SIGABRT  A  Process abort signal.
     SIGALRM   T  Alarm clock.
     SIGBUS    A  Access to an undefined portion of a memory object.
     SIGCHLD  I  Child process terminated, stopped,
     SIGCONT  C  Continue executing, if stopped.
     SIGFPE  A  Erroneous arithmetic operation.
     SIGHUP  T  Hangup.
     SIGILL  A  Illegal instruction.
     SIGINT  T  Terminal interrupt signal.
     SIGKILL  T  Kill (cannot be caught or ignored).
     SIGPIPE  T  Write on a pipe with no one to read it.
     SIGQUIT  A  Terminal quit signal.
     SIGSEGV  A  Invalid memory reference.
     SIGSTOP  S  Stop executing (cannot be caught or ignored).
     SIGTERM  T  Termination signal.
     SIGTSTP  S  Terminal stop signal.
     SIGTTIN  S  Background process attempting read.
     SIGTTOU  S  Background process attempting write.
     SIGUSR1  T  User-defined signal 1.
     SIGUSR2  T  User-defined signal 2.
     SIGPOLL  T  Pollable event.
     SIGPROF  T  Profiling timer expired.
     SIGSYS  A  Bad system call.
     SIGTRAP  A  Trace/breakpoint trap.
     SIGURG  I  High bandwidth data is available at a socket.
     SIGVTALRM  T  Virtual timer expired.
     SIGXCPU  A  CPU time limit exceeded.
     SIGXFSZ  A  File size limit exceeded.

The default actions are as follows:
T : Abnormal termination of the process. The process is terminated with all the consequences of _exit()
A : Abnormal termination of the process.
I  : Ignore the signal.
S : Stop the process
*/
  struct sigaction act_new;
  memset (&act_new, 0, sizeof (struct sigaction));

  act_new.sa_handler = wfd_signal_handler;

  sigaction(SIGABRT, &act_new, NULL);
  sigaction(SIGBUS, &act_new, NULL);
  sigaction(SIGINT, &act_new, NULL); //
  sigaction(SIGKILL, &act_new, NULL);
  sigaction(SIGPIPE, &act_new, NULL);
  sigaction(SIGQUIT, &act_new, NULL); //
  sigaction(SIGSEGV, &act_new, NULL);
  sigaction(SIGTERM, &act_new, NULL); //
  sigaction(SIGSYS, &act_new, NULL);

}



static void wifi_direct_state_change_cb(keynode_t *key, void *data)
{
  WFDServer *server = (WFDServer *)data;
  if(!server) return;
  debug_log("wifi direct state changed");
  int state = -1;
  state = vconf_keynode_get_int(key);
  if (state < VCONFKEY_WIFI_DIRECT_GROUP_OWNER) {
    debug_log("wifi disconnected");
    quit_pushing = TRUE;
    shutdown (server->sockfd, SHUT_RDWR);
    debug_log("wifi_direct_state_change_cb calll quit_program()");
    quit_program();
  }
}

static void lcd_state_change_cb(keynode_t *key, void *data)
{
  WFDServer *server = (WFDServer *)data;
  if(!server)
    return;
  int state = -1;
  state = vconf_keynode_get_int(key);
  if (state == VCONFKEY_PM_STATE_NORMAL) {
    debug_log("source has woke up time to wake-up-sink");
    wfd_resume();
  }
  else if(state == VCONFKEY_PM_STATE_LCDOFF || VCONFKEY_PM_STATE_SLEEP) {
    debug_log("source is sleeping time to go to sleep");
    wfd_standby();
  }
}

int main(int argc, char *argv[])
{
  WFDServer server;
  server.thread = NULL;
  server.clients = NULL;

  g_loop = g_main_loop_new(NULL, FALSE);
  if(NULL == g_loop) {
    debug_log("Unable to create gmain loop! Aborting wfd server\n");
    exit(-1);
  }
  g_type_init();
  debug_log("wfd_proxy_initialize\n");
  if (!__wfd_server_setup(&server)) {
    debug_log("Unable to initialize test server\n");
    exit(-1);
  }
  debug_log("set vconf_notify_key_changed about wifi direct");
  if (0 != vconf_notify_key_changed(VCONFKEY_WIFI_DIRECT_STATE, wifi_direct_state_change_cb, &server)) {
    debug_log("vconf_notify_key_changed() failed");
  }
  debug_log("set vconf_notify_key_changed about LCD state");
  if (0 != vconf_notify_key_changed(VCONFKEY_PM_STATE, lcd_state_change_cb, &server)) {
    debug_log("vconf_notify_key_changed() failed");
  }
  debug_log("wfd_proxy_initialize run loop \n");
  g_main_loop_run(g_loop);
  g_thread_join(server.thread);
  shutdown (server.sockfd, SHUT_RDWR);
  debug_log("WFD SERVER EXIT \n");
  exit(0);
}
