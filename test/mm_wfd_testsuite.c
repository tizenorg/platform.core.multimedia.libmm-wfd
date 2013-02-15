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

#include <glib.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_wfd.h>
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

#define PACKAGE "mm_wfd_testsuite"
#if defined(_USE_GMAINLOOP)
GMainLoop *g_loop;
#endif

char g_file_list[9][256];
#define MAX_STRING_LEN		2048
#define INI_SAMPLE_LIST_MAX 9
enum
{
  CURRENT_STATUS_MAINMENU,
  CURRENT_STATUS_SERVER_IP,
  CURRENT_STATUS_SERVER_PORT,
  CURRENT_STATUS_DISPLAY_MODE,
  CURRENT_STATUS_CONNECT,
};
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

#if 0
  /* add system event callback */
  appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE, lang_changed, ad);
#endif
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

//int r;
struct appdata ad;
#endif

void * overlay = NULL;

int			g_current_state;
int			g_menu_state = CURRENT_STATUS_MAINMENU;
bool			g_bArgPlay = FALSE;
static MMHandleType g_wfd = 0;
unsigned int		g_video_xid = 0;

int g_audio_dsp = FALSE;
int g_video_dsp = FALSE;

char g_subtitle_uri[MAX_STRING_LEN];
int g_subtitle_width = 800;
int g_subtitle_height = 480;
bool g_subtitle_silent = FALSE;
unsigned int g_subtitle_xid = 0;
char *g_err_name = NULL;

static void wfd_start();
static void wfd_stop();
static void wfd_destroy ();

gboolean timeout_quit_program(void* data);
void quit_program();
#ifndef PROTECTOR_VODA_3RD
void TestFileInfo (char* filename);
#endif

bool testsuite_sample_cb(void *stream, int stream_size, void *user_param);

static bool msg_callback(int message, MMMessageParamType *param, void *user_param)
{
  switch (message) {
    case MM_MESSAGE_ERROR:
      quit_pushing = TRUE;
      g_print("error : code = %x\n", param->code);
      //g_print("Got MM_MESSAGE_ERROR, testsuite will be exit\n");
      //quit_program ();							// 090519
      break;

    case MM_MESSAGE_WARNING:
      // g_print("warning : code = %d\n", param->code);
      break;

    case MM_MESSAGE_END_OF_STREAM:
      g_print("end of stream\n");
      mm_wfd_stop(g_wfd);		//bw.jang :+:
      //bw.jang :-: MMPlayerUnrealize(g_wfd);
      //bw.jang :-: MMPlayerDestroy(g_wfd);
      //bw.jang :-: g_wfd = 0;

      if (g_bArgPlay == TRUE ) {

        g_timeout_add(100, timeout_quit_program, 0);

//          quit_program();
    }
    break;

    case MM_MESSAGE_STATE_CHANGED:
      g_current_state = param->state.current;
      //bw.jang :=:
      //g_print("current state : %d\n", g_current_state);
      //-->
      switch(g_current_state)
      {
        case MM_WFD_STATE_NULL:
          g_print("\n                                                            ==> [WFDApp] Player is [NULL]\n");
        break;
        case MM_WFD_STATE_READY:
          g_print("\n                                                            ==> [WFDApp] Player is [READY]\n");
        break;
        case MM_WFD_STATE_PLAYING:
          g_print("\n                                                            ==> [WFDApp] Player is [PLAYING]\n");
        break;
        case MM_WFD_STATE_PAUSED:
          g_print("\n                                                            ==> [WFDApp] Player is [PAUSED]\n");
        break;
      }
      //::
      break;
    default:
      return FALSE;
  }

  return TRUE;
}
static void input_display_mode(char *mode)
{
  int len = strlen(mode);
  int err;
  int ret = MM_ERROR_NONE;
  static Atom g_sc_status_atom = 0;
  static Display *dpy = NULL;
  int dispmode = UTILX_SCRNCONF_DISPMODE_NULL;

  if ( len < 0 || len > MAX_STRING_LEN )
    return;
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    g_printf ("Error : fail to open display\n");
    return 0;
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
  if (!utilx_scrnconf_set_dispmode (dpy, dispmode)) {
    g_printf ("fail to set dispmode\n");
  }
  g_printf("set dispmode : %d", dispmode);
}

static void input_server_ip(char *server_ip)
{
  int len = strlen(server_ip);
  int err;
  int ret = MM_ERROR_NONE;

  if ( len < 0 || len > MAX_STRING_LEN )
    return;

  if (!g_wfd)
  {
    if ( mm_wfd_create(&g_wfd) != MM_ERROR_NONE )
    {
      g_print("wfd create is failed\n");
      return;
    }

    ret = mm_wfd_set_message_callback(g_wfd, msg_callback, (void*)g_wfd);
    if (ret != MM_ERROR_NONE)
    {
      g_print ("Error in setting server_port...\n");
      return;
    }
  }

  ret = mm_wfd_set_attribute(g_wfd,
        &g_err_name,
        "server_ip", server_ip, strlen(server_ip),
        NULL);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("Error in setting server_port...\n");
    return;
  }

}

static void input_server_port(char *port)
{
  int len = strlen(port);
  int err;
  int ret = MM_ERROR_NONE;

  if ( len < 0 || len > MAX_STRING_LEN )
    return;

  if (!g_wfd)
  {
    if ( mm_wfd_create(&g_wfd) != MM_ERROR_NONE )
    {
      g_print("wfd create is failed\n");
      return;
    }

    ret = mm_wfd_set_message_callback(g_wfd, msg_callback, (void*)g_wfd);
    if (ret != MM_ERROR_NONE)
    {
      g_print ("Error in setting server_port...\n");
      return;
    }
  }

  ret = mm_wfd_set_attribute(g_wfd,
         &g_err_name,
         "server_port", port, strlen(port),
         NULL);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("Error in setting server_port...\n");
    return;
  }
}

static void wfd_start()
{
  int bRet = FALSE;
  int ret = MM_ERROR_NONE;

  if (!g_wfd)
  {
    g_print ("Creating server with default values : ");
    if ( mm_wfd_create(&g_wfd) != MM_ERROR_NONE )
    {
      g_print("wfd create is failed\n");
      return;
    }

    ret = mm_wfd_set_message_callback(g_wfd, msg_callback, (void*)g_wfd);
    if (ret != MM_ERROR_NONE)
    {
      g_print ("Error in setting server_port...\n");
      return;
    }

    //TODO: add get_attribute to get default values
    if ( mm_wfd_realize(g_wfd) != MM_ERROR_NONE )
    {
      g_print("wfd realize is failed\n");
      return;
    }

    if ( mm_wfd_connect(g_wfd) != MM_ERROR_NONE )
    {
      g_print("wfd connect is failed\n");
      return;
    }
  }
  if (mm_wfd_start(g_wfd) != MM_ERROR_NONE)
  {
    g_print ("Failed to start WFD");
    return;
  }
}

static void wfd_connect()
{
  int bRet = FALSE;
  int ret = MM_ERROR_NONE;

  if (!g_wfd)
  {
    g_print ("Creating server with default values : ");
    if ( mm_wfd_create(&g_wfd) != MM_ERROR_NONE )
    {
      g_print("wfd create is failed\n");
      return;
    }

    ret = mm_wfd_set_message_callback(g_wfd, msg_callback, (void*)g_wfd);
    if (ret != MM_ERROR_NONE)
    {
      g_print ("Error in setting server_port...\n");
      return;
    }
  }
  // TODO: add get_attribute to get default values
  if ( mm_wfd_realize(g_wfd) != MM_ERROR_NONE )
  {
    g_print("wfd realize is failed\n");
    return;
  }

  if ( mm_wfd_connect(g_wfd) != MM_ERROR_NONE )
  {
    g_print("wfd connect is failed\n");
    return;
  }
}

static void wfd_stop()
{
  int ret = MM_ERROR_NONE;

  ret = mm_wfd_stop(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("Error to do stop...\n");
    return;
  }
  g_print("stop completed \n");
}

static void wfd_pause()
{
  int ret = MM_ERROR_NONE;

  ret = mm_wfd_pause(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("wfd_pause Error to do pausep...\n");
    return;
  }
}

static void wfd_resume()
{
  int ret = MM_ERROR_NONE;

  ret = mm_wfd_resume(g_wfd);
  if (ret != MM_ERROR_NONE)
  {
    g_print ("wfd_resume Error to do resume...\n");
    return;
  }
}

void quit_program()
{
	MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_STDOUT);
	MMTA_RELEASE();
  g_print ("quit_program...\n");

	mm_wfd_unrealize(g_wfd);
	mm_wfd_destroy(g_wfd);
	g_wfd = 0;

#ifdef _USE_GMAINLOOP_TEMP
	g_main_loop_quit(g_loop);
#endif

#ifdef _USE_EFLMAINLOOP
	elm_exit();
#endif
}

void wfd_destroy()
{
        mm_wfd_unrealize(g_wfd);
        mm_wfd_destroy(g_wfd);
        g_wfd = 0;
	g_print("wfd is destroyed.\n");
}

void display_sub_additional()
{

}

void display_sub_basic()
{
	int idx;

	g_print("\n");
	g_print("=========================================================================================\n");
	g_print("                          MM-WFD Testsuite (press q to quit) \n");
	g_print("-----------------------------------------------------------------------------------------\n");

	for( idx = 1; idx <= INI_SAMPLE_LIST_MAX ; idx++ )
	{
		if (strlen (g_file_list[idx-1]) > 0)
			g_print("%d. Play [%s]\n", idx, g_file_list[idx-1]);
	}

	g_print("-----------------------------------------------------------------------------------------\n");
	g_print("a : server IP\t");
	g_print("b : server port  \t");
	g_print("c : connect  \t");
       g_print("d : display mode\t");
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

gboolean timeout_quit_program(void* data)
{
	quit_program();
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
      g_print ("Connecting..\n\n");
      wfd_connect();
    }
    else if (strncmp(cmd, "d", 1) == 0)
    {
      g_menu_state = CURRENT_STATUS_DISPLAY_MODE;
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
          g_print ("STOPPING..\n\n");
          wfd_stop();
          g_print ("STOPPED..\n\n");
    }
    else if (strncmp(cmd, "q", 1) == 0)
    {
          quit_pushing = TRUE;
          g_print ("QUIT program..\n\n");
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
	int ret = 0;
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
