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
#include <glib.h>
#include <glib/gprintf.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_wfd_proxy.h>
#include <dlfcn.h>
#include <utilX.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

#define PACKAGE "mm_wfd_proxy_test"

/*===========================================================================================
|                                             |
|  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE                      |
|                                               |
========================================================================================== */
/*---------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:                     |
---------------------------------------------------------------------------*/
#if defined(_USE_GMAINLOOP)
GMainLoop *g_loop;
#endif

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

/*---------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS:                      |
---------------------------------------------------------------------------*/
static MMHandleType g_wfd = 0;
gboolean quit_pushing;
static int g_current_state = WFDSRC_STATE_NULL;

/*---------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:                       |
---------------------------------------------------------------------------*/
static void wfd_start();
static void wfd_stop();
static void wfd_destroy ();
static void set_display_mode(char *mode);

gboolean timeout_quit_program(void* data);
void quit_program();

static void wfd_proxy_callback(MMHandleType pHandle,
                                   WfdSrcProxyRet error_code,
                                   WfdSrcProxyState state,
                                   void *user_data)
{
  if(error_code == WFDSRC_ERROR_NONE)
  {
    if(g_current_state != state)
    {
      g_print("miracast server STATE changed from %d --> %d \n",g_current_state, state);
      g_current_state = state;
    }
    return;
  }
  g_print("Miracast server notified error %d", error_code);
  switch (state) {
    case WFDSRC_STATE_NULL:
    case WFDSRC_STATE_READY:
    case WFDSRC_STATE_CONNECTION_WAIT:
    case WFDSRC_STATE_CONNECTED:
    {
      wfd_destroy();
      quit_program();
    }break;
    case WFDSRC_STATE_PLAYING:
    case WFDSRC_STATE_PAUSED:
    {
      wfd_stop();
      wfd_destroy();
      quit_program();
    }break;
    case WFDSRC_STATE_NONE:
    default:
    {
      wfd_destroy();
      quit_program();
    }break;
  }
  return;
}

static void wfd_input_server_ip_and_port(char* serverIP, char* port)
{
  if (WfdSrcProxySetIPAddrAndPort(g_wfd, serverIP, port) != MM_ERROR_NONE)
  {
    g_print ("wfd_input_server_ip_and_port Failed to set attributes");
    quit_program();
    return;
  }
}

static void wfd_connect()
{
  if (WfdSrcProxyConnect(g_wfd) != MM_ERROR_NONE)
  {
    g_print ("wfd_start Failed to start WFD");
    wfd_destroy();
    quit_program();
    return;
  }
}

static void wfd_start()
{
  if (WfdSrcProxyStart(g_wfd) != MM_ERROR_NONE)
  {
    g_print ("wfd_start Failed to start WFD");
    wfd_destroy();
    quit_program();
    return;
  }
}

static void wfd_stop()
{
  int ret = MM_ERROR_NONE;

  ret = WfdSrcProxyStop(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("wfd_stop Error to do stop...\n");
    wfd_destroy();
    quit_program();
    return;
  }
}

static void wfd_pause()
{
  int ret = MM_ERROR_NONE;

  ret = WfdSrcProxyPause(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("wfd_pause Error to do pause...\n");
    quit_program();
    return;
  }
}

static void wfd_resume()
{
  int ret = MM_ERROR_NONE;

  ret = WfdSrcProxyResume(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("wfd_resume Error to do resume...\n");
    quit_program();
    return;
  }
}

void quit_program()
{
  int ret;
  ret = WfdSrcProxyDeInit(g_wfd);
  if (ret != WFDSRC_ERROR_NONE) {
    g_print ("error : WfdSrcProxyDeInit [%d]", ret);
  }
  g_wfd = 0;
  g_main_loop_quit(g_loop);
}

void wfd_destroy()
{
  WfdSrcProxyDestroyServer(g_wfd);
  g_print("wfd_destroy wfd server is destroyed.\n");
}

static void set_display_mode(char *mode)
{
	int len = strlen(mode);
	//static Atom g_sc_status_atom = 0;
	static Display *dpy = NULL;
	int dispmode = UTILX_SCRNCONF_DISPMODE_NULL;

	if ( len < 0 || len > MAX_STRING_LEN )
		return;
	dpy = XOpenDisplay(NULL);

	if (!dpy) {
        g_printf ("Error : fail to open display\n");
        return;
	}
	//g_sc_status_atom = XInternAtom (dpy, STR_ATOM_SCRNCONF_STATUS, False);
	XSelectInput (dpy, RootWindow(dpy, 0), StructureNotifyMask);

	g_printf("%s", mode);
	if (strncmp(mode, "a", 1) == 0) {
		dispmode = UTILX_SCRNCONF_DISPMODE_CLONE;
	}
	else if (strncmp(mode, "b", 1) == 0) {
		dispmode = UTILX_SCRNCONF_DISPMODE_EXTENDED;
	}
	if (!utilx_scrnconf_set_dispmode (dpy, dispmode)) {
		g_printf ("fail to set dispmode\n");
	}
	g_printf("set dispmode : %d", dispmode);
}

static void displaymenu()
{
  g_print("\n");
  g_print("=========================================================================================\n");
  g_print("                          MM-WFD Testsuite (press q to quit) \n");
  g_print("-----------------------------------------------------------------------------------------\n");
  g_print("-----------------------------------------------------------------------------------------\n");
  g_print("a : a ip port\t");
  g_print("c : Connect\t");
  g_print("d : displaymode (a:clone b:desktop) \t");
  g_print("s : Start  \t");
  g_print("p : pause \t");
  g_print("w : standby \t");
  g_print("r : resume \t");
  g_print("e : stop\t");
  g_print("u : destroy server\t");
  g_print("q : quit\t");
  g_print("\n");
  g_print("=========================================================================================\n");
}

gboolean timeout_menu_display(void* data)
{
  displaymenu();
  return FALSE;
}

gboolean timeout_quit_program(void* data)
{
  quit_program();
  return FALSE;
}

static void interpret (char *cmd)
{
  //if ( strlen(cmd) == 1 )
  {
    if (strncmp(cmd, "a", 1) == 0)
    {
      gchar **IP_port;
      g_print ("Input server IP and port number..\n\n");
      IP_port = g_strsplit(cmd," ",0);
      wfd_input_server_ip_and_port(IP_port[1], IP_port[2]);
    }
    else if (strncmp(cmd, "c", 1) == 0)
    {
      g_print ("Connecting... WFD..\n\n");
      wfd_connect();
    }
    else if (strncmp(cmd, "d", 1) == 0)
    {
      gchar **Display_mode;
      Display_mode = g_strsplit(cmd," ",0);
      g_print ("set display mode\n\n");
      set_display_mode(Display_mode[1]);
    }
    else if (strncmp(cmd, "s", 1) == 0)
    {
      g_print ("Starting... WFD..\n\n");
      wfd_start();
    }
    else if (strncmp(cmd, "p", 1) == 0)
    {
      g_print ("PAUSING..\n\n");
      wfd_pause();
    }
    else if (strncmp(cmd, "r", 1) == 0)
    {
      g_print ("RESUMING..\n\n");
      wfd_resume();
    }
    else if (strncmp(cmd, "e", 1) == 0)
    {
      wfd_stop();
    }
    else if (strncmp(cmd, "u", 1) == 0)
    {
      wfd_destroy();
    }
    else if (strncmp(cmd, "q", 1) == 0)
    {
      quit_pushing = TRUE;
      quit_program();
    }
    else
    {
      g_print("unknown menu \n");
    }
  }
  g_timeout_add(100, timeout_menu_display, 0);
}

gboolean input (GIOChannel *channel)
{
    char buf[MAX_STRING_LEN + 3];
    gsize read;

    g_io_channel_read(channel, buf, MAX_STRING_LEN, &read);
    buf[read] = '\0';
    g_strstrip(buf);
    interpret (buf);
    return TRUE;
}

int main(int argc, char *argv[])
{
  GIOChannel *stdin_channel;

  WfdSrcProxyInit( &g_wfd, wfd_proxy_callback, NULL);
  stdin_channel = g_io_channel_unix_new(0);
  g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc)input, NULL);

  displaymenu();

#if defined(_USE_GMAINLOOP)
  g_print (" <<<< GTK Loop >>>>");
  g_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(g_loop);
  return 0;
#endif

}
