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
/* ===========================================================================================
EDIT HISTORY FOR MODULE

    This section contains comments describing changes made to the module.
    Notice that changes are listed in reverse chronological order.

when  who   what, where, why
---------   --------------------	----------------------------------------------------------
09/28/07  jhchoi.choi@samsung.com  Created

=========================================================================================== */

/*===========================================================================================
|                                                                                                                         |
|  INCLUDE FILES                                                                                          |
|                                                                                                                         |
========================================================================================== */
//#define MTRACE;
#include <glib.h>
#include <glib/gprintf.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_wfd_sink.h>
#include <iniparser.h>
#include <dlfcn.h>
#include <wifi-direct.h>

gboolean quit_pushing;

#define PACKAGE "mm_wfd_sink_testsuite"

/*===========================================================================================
|                                                                                                                         |
|  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE             |
|                                                                                                                         |
========================================================================================== */
/*---------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                                  |
---------------------------------------------------------------------------*/
#if defined(_USE_GMAINLOOP)
GMainLoop *g_loop;
#endif

gint g_server_port;
gchar g_server_uri[255];
gchar g_server_ip_address[255];

static int g_peer_cnt = 0;

enum {
  TIZEN_PORT = 2022,
  GALAXY_PORT = 7236,
};

/*---------------------------------------------------------------------------
|    GLOBAL CONSTANT DEFINITIONS:                                |
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED VARIABLE DECLARATIONS:                          |
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED FUNCTION DECLARATIONS:                         |
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL #defines:                                                                    |
---------------------------------------------------------------------------*/
#define MAX_STRING_LEN   2048

/*---------------------------------------------------------------------------
|    LOCAL CONSTANT DEFINITIONS:                                   |
---------------------------------------------------------------------------*/
enum
{
  CURRENT_STATUS_MAINMENU,
  CURRENT_STATUS_TIZEN_SELECTED,
  CURRENT_STATUS_GALAXY_SELECTED,
  CURRENT_STATUS_IP_ADDRESS_SELECTED
};
/*---------------------------------------------------------------------------
|    LOCAL DATA TYPE DEFINITIONS:                                    |
---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS:                                     |
---------------------------------------------------------------------------*/

int g_menu_state = CURRENT_STATUS_MAINMENU;
static MMHandleType g_wfd = 0;
void *g_event_capturer = 0;

/*---------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:                                                             |
---------------------------------------------------------------------------*/
void quit_program();
gboolean _wfd_start(gpointer data);

/*===========================================================================================
|                                                                                                                          |
|  FUNCTION DEFINITIONS                                                                       |
|                                                                                                                         |
========================================================================================== */


/*---------------------------------------------------------------------------
  |    LOCAL FUNCTION DEFINITIONS:                                  |
  ---------------------------------------------------------------------------*/

void quit_program()
{
  mm_wfd_sink_destroy(g_wfd);
  g_wfd = 0;
#ifdef _USE_EFLMAINLOOP
  elm_exit();
#endif

#ifdef _USE_GMAINLOOP
  g_main_loop_quit(g_loop);
#endif
}

void display_sub_basic()
{
  g_print("\n");
  g_print("=========================================================================================\n");
  g_print("                          MM-WFD Sink Testsuite with WIFI-DIRECT (press q to quit) \n");
  g_print("-----------------------------------------------------------------------------------------\n");
  g_print("a : TIZEN\t");
  g_print("b : GALAXY\t");
  g_print("i : custom ip address\t");
  g_print("p : pause \t");
  g_print("r : resume \t");
  g_print("s : start \t");
  g_print("e : stop\t");
  g_print("q : quit\t");
  g_print("\n");
  g_print("=========================================================================================\n");
}

void reset_menu_state(void)
{
  g_menu_state = CURRENT_STATUS_MAINMENU;
}


static void displaymenu()
{
  if (g_menu_state == CURRENT_STATUS_MAINMENU)
  {
    display_sub_basic();
  }
  else if (g_menu_state == CURRENT_STATUS_TIZEN_SELECTED)
  {
    g_print("*** TIZEN is selected. Use port number 2022\n");
    reset_menu_state();
    display_sub_basic();
  }
  else if (g_menu_state == CURRENT_STATUS_IP_ADDRESS_SELECTED)
  {
    g_print("Enter ip address:\n");
  }
  else if (g_menu_state == CURRENT_STATUS_GALAXY_SELECTED)
  {
    g_print("*** GALAXY is selected. Use port number 7236\n");
    reset_menu_state();
    display_sub_basic();
  }
  else
  {
    g_print("*** unknown status.\n");
    quit_program();
  }
  g_print(" >>> ");
}

gboolean timeout_menu_display(void* data)
{
  displaymenu();
  return FALSE;
}

void _interpret_main_menu(char *cmd)
{
  if ( strlen(cmd) == 1 )
  {
    if (strncmp(cmd, "a", 1) == 0)
    {
      g_menu_state = CURRENT_STATUS_TIZEN_SELECTED;
      g_server_port = TIZEN_PORT;
    }
    else if (strncmp(cmd, "b", 1) == 0)
    {
      g_menu_state = CURRENT_STATUS_GALAXY_SELECTED;
      g_server_port = GALAXY_PORT;
    }
    else if (strncmp(cmd, "p", 1) == 0)
    {
      g_print ("PAUSING..\n\n");
      //mm_wfd_sink_pause(g_wfd);
    }
    else if (strncmp(cmd, "r", 1) == 0)
    {
      g_print ("RESUMING..\n\n");
      //mm_wfd_sink_resume(g_wfd);
    }
    else if (strncmp(cmd, "e", 1) == 0)
    {
      g_print ("STOPPING..\n\n");
      mm_wfd_sink_stop(g_wfd);
      g_print ("STOPPED..\n\n");
    }
    else if (strncmp(cmd, "q", 1) == 0)
    {
      quit_pushing = TRUE;
      quit_program();
    }
    else if (strncmp(cmd, "i", 1) == 0)
    {
      g_menu_state = CURRENT_STATUS_IP_ADDRESS_SELECTED;
    }
    else if (strncmp(cmd, "s", 1) == 0)
    {
      snprintf(g_server_uri, sizeof(g_server_uri),
          "rtsp://%s:%d/wfd1.0/streamid=0", g_server_ip_address, g_server_port);
      _wfd_start(NULL);
    }
    else
    {
      g_print("unknown menu \n");
    }
  }
}

static void interpret (char *cmd)
{
  switch (g_menu_state)
  {
    case CURRENT_STATUS_MAINMENU:
    {
      _interpret_main_menu(cmd);
    }
    break;
    case CURRENT_STATUS_IP_ADDRESS_SELECTED:
    {
      strncpy (g_server_ip_address, cmd, sizeof(g_server_ip_address));
      reset_menu_state();
    }
    break;
  }
  g_timeout_add(100, timeout_menu_display, 0);
}

gboolean input (GIOChannel *channel, GIOCondition cond, gpointer data)
{
  char buf[MAX_STRING_LEN + 3];
  gsize read;
  GError *error = NULL;
  g_io_channel_read (channel, buf, 80, &read);
  buf[read] = '\0';
  g_strstrip(buf);
  interpret(buf);
  return TRUE;
}


bool _discovered_peer_cb(wifi_direct_discovered_peer_info_s *peer, void *user_data)
{
  if(peer == NULL) return FALSE;

  g_print("[%d] discovered device peer : %s, %s, %d\n", g_peer_cnt, peer->device_name, peer->mac_address, peer->is_connected);

  char *mac_addr = (char *)user_data;
  if (peer && !strcmp(peer->mac_address, mac_addr)) {
    g_print("PORT number of client = %d\n", g_server_port);
    return FALSE;
  }

  g_peer_cnt++;
  return TRUE;
}

bool _connected_peer_cb(wifi_direct_connected_peer_info_s *peer, void *user_data)
{
  g_print("Connected to IP [%s] port [%d]\n", peer->ip_address, g_server_port);

  memset(g_server_uri, 0x00, sizeof(g_server_uri));
  snprintf(g_server_uri, sizeof(g_server_uri), "rtsp://%s:%d/wfd1.0/streamid=0", peer->ip_address, g_server_port);

  g_print("RTSP address : %s\n", g_server_uri);

  g_timeout_add(5000, _wfd_start, NULL);

  return TRUE;
}

void _activation_cb(int error_code, wifi_direct_device_state_e device_state, void *user_data)
{
  g_print("Activated [ error : %d device state : %d ]\n", error_code, device_state);
}

void _discover_cb(int error_code, wifi_direct_discovery_state_e discovery_state, void *user_data)
{
  g_print("Discovered [ error : %d discovery state : %d ]\n", error_code, discovery_state);

  if (discovery_state == WIFI_DIRECT_ONLY_LISTEN_STARTED) {
    g_print("** WIFI_DIRECT_ONLY_LISTEN_STARTED **\n");
  } else if (discovery_state == WIFI_DIRECT_DISCOVERY_STARTED) {
    g_print("** WIFI_DIRECT_DISCOVERY_STARTED **\n");
  } else if (discovery_state == WIFI_DIRECT_DISCOVERY_FOUND) {
    g_print("** WIFI_DIRECT_DISCOVERY_FOUND **\n");

    g_peer_cnt = 0;
  } else {
    g_print("** discovery state error : %d **\n", discovery_state);
  } 
}

void _connection_cb(int error_code, wifi_direct_connection_state_e connection_state, const char *mac_address, void *user_data)
{
  g_print("Connected [ error : %d connection state : %d mac_addr:%s ]\n", error_code, connection_state, mac_address);

  int err = -1;
  if (connection_state == WIFI_DIRECT_CONNECTION_REQ) {
    err = wifi_direct_foreach_discovered_peers(_discovered_peer_cb, (char *)mac_address);
    if (err != WIFI_DIRECT_ERROR_NONE) {
      g_print("wifi_direct_foreach_discovered_peers is failed : %d\n", err);
    }

    err = wifi_direct_accept_connection((char *)mac_address);
    if (err != WIFI_DIRECT_ERROR_NONE) {
      g_print("wifi_direct_accept_connection is failed : %d\n", err);
    }
  } else if (connection_state == WIFI_DIRECT_CONNECTION_RSP) {
    bool is_go = FALSE;
    err = wifi_direct_is_group_owner(&is_go);
    if (err != WIFI_DIRECT_ERROR_NONE) {
      g_print("wifi_direct_is_group_owner is failed : %d\n", err);
    }

    if (is_go) {
      g_print("Connected as Group Owner\n");
    } else {
      err = wifi_direct_foreach_connected_peers(_connected_peer_cb, (void *)NULL);
      if (err < WIFI_DIRECT_ERROR_NONE) {
        g_print("Error : wifi_direct_foreach_connected_peers failed : %d\n", err);
        return;
      }
    }
  }
}

void _ip_assigned_cb(const char *mac_address, const char *ip_address, const char *interface_address, void *user_data)
{
  g_print("IP assigned [ ip addr : %s if addr : %s mac_addr:%s ]\n", ip_address, interface_address, mac_address);

  memset(g_server_uri, 0x00, sizeof(g_server_uri));
  snprintf(g_server_uri, sizeof(g_server_uri), "rtsp://%s:%d/wfd1.0/streamid=0", ip_address, g_server_port);
  
  g_print("RTSP address : %s\n", g_server_uri);

  g_timeout_add(5000, _wfd_start, NULL);
}

void _wfd_msg_handle(MMWfdSinkStateType type, void *user_data)
{
  if (type == MM_WFD_SINK_STATE_TEARDOWN) {
    g_print("Got message from pipeline! : MM_WFD_SINK_STATE_TEARDOWN\n");
    quit_program();
  }
}

gboolean _wfd_start(gpointer data)
{
  int err = -1;

  err = mm_wfd_sink_create(&g_wfd);
  if (err < 0) {
    g_print("Error : mm_wfd_sink_create failed : %d\n", err);
    return FALSE;
  }
  err = mm_wfd_sink_realize(g_wfd);
  if (err < 0) {
    g_print("Error : mm_wfd_sink_realize failed : %d\n", err);
    return FALSE;
  }
  mm_wfd_sink_set_attribute(g_wfd, NULL, "display_surface_type", MM_DISPLAY_SURFACE_X, NULL);
#ifdef _USE_EFLMAINLOOP
  /* display surface type and overlay setting */
  mm_wfd_sink_set_attribute(g_wfd, NULL, "display_surface_type", MM_DISPLAY_SURFACE_EVAS, NULL);
  mm_wfd_sink_set_attribute(g_wfd, NULL, "display_overlay", g_eo, sizeof(g_eo), NULL);
  /*     0:LETTER BOX, 1:ORIGIN SIZE, 2:FULL SCREEN, 3:CROPPED FULL, 4:ORIGIN_OR_LETTER    */
  mm_wfd_sink_set_display_method(g_wfd, 1);
  mm_wfd_sink_set_attribute(g_wfd, NULL, "display_method", 1, NULL);
  /*     0,1      */
  mm_wfd_sink_set_attribute(g_wfd, NULL, "display_visible", 1, NULL);
#endif
  err = mm_wfd_sink_connect(g_wfd, g_server_uri);
  if (err < 0) {
    g_print("Error : mm_wfd_sink_connect failed : %d\n", err);
    return FALSE;
  }
  err = mm_wfd_sink_set_message_callback(g_wfd, _wfd_msg_handle, (void *)g_wfd);
  if (err < 0) {
    g_print("Error : mm_wfd_sink_set_message_callback failed : %d\n", err);
  }
  err = mm_wfd_sink_start(g_wfd);
  if (err < 0) {
    g_print("Error : mm_wfd_sink_start failed : %d\n", err);
  }

  return FALSE;
}

gboolean _wfd_start_jobs(gpointer data)
{
  g_print("\n ------ Starting to get p2p connection established ------\n");

  int err = -1;
  wifi_direct_state_e wifi_status;

  err = wifi_direct_initialize();
  if (err < WIFI_DIRECT_ERROR_NONE) {
    g_print("Error : wifi_direct_initialize failed : %d\n", err);
    return FALSE;
  }

  struct ug_data *ugd = (struct ug_data *)data;

  /* Activation / Deactivation state Callback */
  err = wifi_direct_set_device_state_changed_cb(_activation_cb, (void *)ugd);
    if (err != WIFI_DIRECT_ERROR_NONE) {
    g_print("Failed to set callback fucntion. [%d]\n", err);
    return FALSE;
  }

  /* Discovery state Callback */
  err = wifi_direct_set_discovery_state_changed_cb(_discover_cb, (void *)ugd);
  if (err != WIFI_DIRECT_ERROR_NONE) {
    g_print("Failed to set callback fucntion. [%d]\n", err);
    return FALSE;
  }

  /* Connection state Callback */
  err = wifi_direct_set_connection_state_changed_cb(_connection_cb, (void *)ugd);
  if (err != WIFI_DIRECT_ERROR_NONE) {
    g_print("Failed to set callback fucntion. [%d]\n", err);
    return FALSE;
  }

  /* IP address assigning state callback */ 
  err = wifi_direct_set_client_ip_address_assigned_cb(_ip_assigned_cb, (void *)ugd);
  if (err != WIFI_DIRECT_ERROR_NONE) {
    g_print("Failed to set callback fucntion. [%d]\n", err);
    return FALSE;
  }

  err = wifi_direct_get_state(&wifi_status);

  if (wifi_status < WIFI_DIRECT_STATE_ACTIVATED) {
    g_print("wifi direct status < WIFI_DIRECT_STATE_ACTIVATED\n");
    g_print("\n ------ Starting to activate wfd ------\n");
    err = wifi_direct_activate();
    if (err < WIFI_DIRECT_ERROR_NONE) {
      g_print("Error : wifi_direct_activate failed : %d\n", err);
      return FALSE;
    }
  } else {
    g_print("wifi direct status >= WIFI_DIRECT_STATE_ACTIVATED.. Disconnect all first\n");
    err = wifi_direct_disconnect_all();
    if(!err) {
      g_print("wifi_direct_disconnect_all success\n");
    } else {
      g_print("wifi_direct_disconnect_all fail\n");
    }
  }

#if 0
  g_print("\n ------ Starting wifi display ------\n");
  err = wifi_direct_display_init();
  if (err < WIFI_DIRECT_ERROR_NONE) {
    g_print("wifi_direct_display_init fail\n");
    return false;
  } else {
    g_print("wifi_direct_display_init_success\n");
  }

  err = wifi_direct_display_set_device(WIFI_DISPLAY_TYPE_SINK,g_server_port,0);

  if (err < WIFI_DIRECT_ERROR_NONE) {
    g_print("wifi_direct_display_set_device fail\n");
    return false ;
  } else {
    g_print("wifi_direct_display_set_device success\n");
  }
#endif
  err = wifi_direct_set_group_owner_intent(2);
  if (err < WIFI_DIRECT_ERROR_NONE) {
    g_print("wifi_direct_set_group_owner_intent Fail\n");
    return false;
  } else {
    g_print("wifi_direct_set_group_owner_intent Success\n");
  }

  err = wifi_direct_set_max_clients(1);
  if (err < WIFI_DIRECT_ERROR_NONE) {
    g_print("wifi_direct_set_max_clients Fail\n");
    return false;
  } else {
    g_print("wifi_direct_set_max_clients Success\n");
  }

  return FALSE;
}

gboolean timeout_start_job(void* data)
{
  _wfd_start_jobs(NULL);
  return FALSE;
}

int main(int argc, char *argv[])
{
  GIOChannel *stdin_channel;
  GMainContext *context = NULL;
  GSource *start_job_src = NULL;

  g_server_ip_address[0] = '\0';

  stdin_channel = g_io_channel_unix_new(0);
  g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc)input, NULL);

  displaymenu();

  g_server_port = TIZEN_PORT;

  g_print (" <<<< GTK Loop >>>>");
  g_loop = g_main_loop_new(NULL, TRUE);
  //context = g_main_loop_get_context(g_loop);

  //start_job_src = g_idle_source_new ();
  //g_source_set_callback (start_job_src, _wfd_start_jobs, NULL, NULL);
  //g_source_attach (start_job_src, context);

  g_main_loop_run(g_loop);
    g_print("exit main\n");
  return 0;
}
