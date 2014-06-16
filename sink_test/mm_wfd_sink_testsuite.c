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

when		who						what, where, why
---------	--------------------	----------------------------------------------------------
09/28/07	jhchoi.choi@samsung.com	Created

=========================================================================================== */

/*===========================================================================================
|																							|
|  INCLUDE FILES																			|
|  																							|
========================================================================================== */
//#define MTRACE;
#include <glib.h>
#include <glib/gprintf.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_wfd_sink.h>
#include <iniparser.h>
#include <mm_ta.h>
#include <dlfcn.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <utilX.h>

#if defined(_USE_EFLMAINLOOP)
#include <appcore-efl.h>
//#include <Ecore_X.h>
#include <Elementary.h>

#elif defined(_USE_GMAINLOOP)
#endif

//#include <mm_session.h>

gboolean quit_pushing;

#define PACKAGE "mm_wfd_sink_testsuite"

/*===========================================================================================
|																							|
|  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE											|
|  																							|
========================================================================================== */
/*---------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:											|
---------------------------------------------------------------------------*/
#if defined(_USE_GMAINLOOP)
GMainLoop *g_loop;
#endif

gchar *g_server_ip;
gchar *g_server_port;
gchar g_server_uri[255];

/*---------------------------------------------------------------------------
|    GLOBAL CONSTANT DEFINITIONS:											|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED VARIABLE DECLARATIONS:										|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED FUNCTION DECLARATIONS:										|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL #defines:														|
---------------------------------------------------------------------------*/
#define MAX_STRING_LEN		2048

/*---------------------------------------------------------------------------
|    LOCAL CONSTANT DEFINITIONS:											|
---------------------------------------------------------------------------*/
enum
{
  CURRENT_STATUS_MAINMENU,
  CURRENT_STATUS_SERVER_IP,
  CURRENT_STATUS_SERVER_PORT,
  CURRENT_STATUS_DISPLAY_MODE,
  CURRENT_STATUS_CONNECT,
};
/*---------------------------------------------------------------------------
|    LOCAL DATA TYPE DEFINITIONS:											|
---------------------------------------------------------------------------*/

#ifdef _USE_EFLMAINLOOP
struct appdata
{
  Evas *evas;
  Ecore_Evas *ee;
  Evas_Object *win;

  Evas_Object *layout_main; /* layout widget based on EDJ */
  Ecore_X_Window xid;

  /* add more variables here */
};

static void win_del(void *data, Evas_Object *obj, void *event)
{
  elm_exit();
}

static Evas_Object* create_win(const char *name)
{
  Evas_Object *eo;
  int w, h;

  printf ("[%s][%d] name=%s\n", __func__, __LINE__, name);

  eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
  if (eo) {
    elm_win_title_set(eo, name);
    elm_win_borderless_set(eo, EINA_TRUE);
    evas_object_smart_callback_add(eo, "delete,request",win_del, NULL);
    ecore_x_window_size_get(ecore_x_window_root_first_get(),&w, &h);
    evas_object_resize(eo, w, h);
  }

  return eo;
}

static int app_create(void *data)
{
  struct appdata *ad = data;
  Evas_Object *win;
  //Evas_Object *ly;
  //int r;

  /* create window */
  win = create_win(PACKAGE);
  if (win == NULL)
    return -1;
  ad->win = win;

#if 0
  /* load edje */
  ly = load_edj(win, EDJ_FILE, GRP_MAIN);
  if (ly == NULL)
    return -1;
  elm_win_resize_object_add(win, ly);
  edje_object_signal_callback_add(elm_layout_edje_get(ly),
    "EXIT", "*", main_quit_cb, NULL);
  ad->ly_main = ly;
  evas_object_show(ly);

  /* init internationalization */
  r = appcore_set_i18n(PACKAGE, LOCALEDIR);
  if (r)
    return -1;
  lang_changed(ad);
#endif
  evas_object_show(win);

  return 0;
}

static int app_terminate(void *data)
{
  struct appdata *ad = data;

  if (ad->win)
    evas_object_del(ad->win);

  return 0;
}

struct appcore_ops ops = {
 .create = app_create,
 .terminate = app_terminate,
};

struct appdata ad;
#endif


/*---------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS:											|
---------------------------------------------------------------------------*/

int	g_menu_state = CURRENT_STATUS_MAINMENU;
static MMHandleType g_wfd = 0;

/*---------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:												|
---------------------------------------------------------------------------*/
void quit_program();

/*===========================================================================================
|																							|
|  FUNCTION DEFINITIONS																		|
|  																							|
========================================================================================== */


/*---------------------------------------------------------------------------
  |    LOCAL FUNCTION DEFINITIONS:											|
  ---------------------------------------------------------------------------*/

static void input_display_mode(char *mode)
{
  int len = strlen(mode);
  static Atom g_sc_status_atom = 0;
  static Display *dpy = NULL;
  int dispmode = UTILX_SCRNCONF_DISPMODE_NULL;

  if ( len < 0 || len > MAX_STRING_LEN )
    return;
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    g_printf ("Error : fail to open display\n");
    return;
  }
  g_sc_status_atom = XInternAtom (dpy, STR_ATOM_SCRNCONF_STATUS, False);
       XSelectInput (dpy, RootWindow(dpy, 0), StructureNotifyMask);

  g_printf("%s", mode);
  if (strncmp(mode, "a", 1) == 0) {
    dispmode = UTILX_SCRNCONF_DISPMODE_CLONE;
  }
  else if (strncmp(mode, "b", 1) == 0) {
    dispmode = UTILX_SCRNCONF_DISPMODE_EXTENDED;
  }
/*
  if (!utilx_scrnconf_set_dispmode (dpy, dispmode)) {
    g_printf ("fail to set dispmode\n");
  }
  g_printf("set dispmode : %d", dispmode);
*/
}

static void input_server_ip(char *server_ip)
{
  int len = strlen(server_ip);

  if ( len < 0 || len > MAX_STRING_LEN )
    return;

  g_server_ip = strdup(server_ip);
}

static void input_server_port(char *port)
{
  int len = strlen(port);

  if ( len < 0 || len > MAX_STRING_LEN )
    return;

  g_server_port = strdup(port);
}


void quit_program()
{
	MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_STDOUT);
	MMTA_RELEASE();

	mm_wfd_sink_destroy(g_wfd);
	g_wfd = 0;

#ifdef _USE_GMAINLOOP
	g_main_loop_quit(g_loop);
#endif

#ifdef _USE_EFLMAINLOOP
	elm_exit();
#endif
}

void display_sub_basic()
{
	g_print("\n");
	g_print("=========================================================================================\n");
	g_print("                          MM-WFD Sink Testsuite (press q to quit) \n");
	g_print("-----------------------------------------------------------------------------------------\n");
	g_print("a : server IP\t");
	g_print("b : server port  \t");
	g_print("c : create  \t");
	g_print("s : start  \t");
	g_print("p : pause \t");
	g_print("r : resume \t");
	g_print("e : stop\t");
	g_print("q : quit\t");
	g_print("\n");
	g_print("=========================================================================================\n");
}


static void displaymenu()
{
	if (g_menu_state == CURRENT_STATUS_MAINMENU)
	{
		display_sub_basic();
	}
	else if (g_menu_state == CURRENT_STATUS_SERVER_IP)
	{
		g_print("*** input server ip address.\n");
	}
	else if (g_menu_state == CURRENT_STATUS_SERVER_PORT)
	{
		g_print("*** input server port.\n");
	}
	else if (g_menu_state == CURRENT_STATUS_DISPLAY_MODE)
	{
		g_print("*** input display mode. (a:clone mode, b:desktop mode)\n");
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

void reset_menu_state(void)
{
	g_menu_state = CURRENT_STATUS_MAINMENU;
}


gboolean start_pushing_buffers;
guint64 offset;
guint size_to_be_pushed;

gboolean seek_flag;

char filename_push[256];

void _interpret_main_menu(char *cmd)
{
  if ( strlen(cmd) == 1 )
  {
    if (strncmp(cmd, "a", 1) == 0)
    {
      g_menu_state = CURRENT_STATUS_SERVER_IP;
    }
    else if (strncmp(cmd, "b", 1) == 0)
    {
      g_menu_state = CURRENT_STATUS_SERVER_PORT;
    }
    else if (strncmp(cmd, "c", 1) == 0)
    {
      g_print ("Create..\n\n");

      memset(g_server_uri, 0x00, sizeof(g_server_uri));
      if (!g_server_ip || !g_server_port) {
        g_print("ip or port is NOT set!\n");
        return;
      }
      snprintf(g_server_uri, sizeof(g_server_uri), "rtsp://%s:%s/wfd1.0/streamid=0", g_server_ip, g_server_port);

      // snprintf(g_server_uri, sizeof(g_server_uri), "rtsp://192.168.49.1:2022/wfd1.0/streamid=0");
      g_print("URI : %s\n", g_server_uri);

      if (g_wfd) mm_wfd_sink_destroy(g_wfd);
      g_wfd = 0;
      mm_wfd_sink_create(&g_wfd, g_server_uri);
    }
    else if (strncmp(cmd, "d", 1) == 0)
    {
      g_menu_state = CURRENT_STATUS_DISPLAY_MODE;
    }
    else if (strncmp(cmd, "s", 1) == 0)
    {
      g_print ("Starting... WFD..\n\n");
      mm_wfd_sink_start(g_wfd);
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

		case CURRENT_STATUS_SERVER_IP:
		{
			input_server_ip(cmd);
			//g_menu_state = CURRENT_STATUS_SERVER_PORT;
			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_SERVER_PORT:
		{
			input_server_port(cmd);

			reset_menu_state();
		}
		break;
		case CURRENT_STATUS_DISPLAY_MODE:
		{
			input_display_mode(cmd);
			reset_menu_state();
		}
		break;
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
	MMTA_INIT();

	stdin_channel = g_io_channel_unix_new(0);
	g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc)input, NULL);

	displaymenu();

#if defined(_USE_GMAINLOOP)
	g_print (" <<<< GTK Loop >>>>");
	g_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(g_loop);
    g_print("exit main\n");
	return 0;
#elif defined(_USE_EFLMAINLOOP)
	g_print (" <<<< EFL Loop >>>>");
	memset(&ad, 0x0, sizeof(struct appdata));
	ops.data = &ad;
	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
#endif

}
