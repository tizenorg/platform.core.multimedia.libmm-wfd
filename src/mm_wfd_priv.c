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

#include <gst/gst.h>

#include "mm_wfd_priv.h"

static const gchar * __get_state_name ( int state );
static int __mmwfd_check_state(mm_wfd_t* wfd, enum PlayerCommandState command);
static gboolean __mmwfd_set_state(mm_wfd_t* wfd, int state);
static gboolean __mmwfd_gstreamer_init(void);

void __mmwfd_server_cb(gboolean is_error, void *userdata)
{
  MMMessageParamType msg = {0, };
  mm_wfd_t* wfd = (mm_wfd_t*)userdata;

  debug_fenter();

  /* check wfd handle */
  return_if_fail ( wfd);
  debug_error("posting message to app...");

//  msg.state.previous = prev_state;
//  msg.state.current = new_state;
  if(is_error)
    MMWFD_POST_MSG( wfd, MM_MESSAGE_ERROR, &msg );
}

int _mmwfd_create (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*)hwfd;
  debug_fenter();

  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  MMTA_ACUM_ITEM_BEGIN("[KPI] media wifi-display service create->playing", FALSE);

  /* initialize wfd state */
  MMWFD_CURRENT_STATE(wfd)   = MM_WFD_STATE_NONE;
  MMWFD_PREV_STATE(wfd)     = MM_WFD_STATE_NONE;
  MMWFD_PENDING_STATE(wfd)   = MM_WFD_STATE_NONE;
  MMWFD_TARGET_STATE(wfd)    = MM_WFD_STATE_NONE;

    debug_log ("\n\n\ncur : %d, prev : %d, pend : %d, target : %d\n\n\n\n",
       MMWFD_CURRENT_STATE(wfd),
       MMWFD_PREV_STATE(wfd),
       MMWFD_PENDING_STATE(wfd),
       MMWFD_TARGET_STATE(wfd));

  MMWFD_PRINT_STATE(wfd);

  /* check current state */
  MMWFD_CHECK_STATE_RETURN_IF_FAIL ( wfd, MMWFD_COMMAND_CREATE );

  /* construct attributes */
  // TODO: need to decide on attributes

  wfd->attrs = _mmwfd_construct_attribute(wfd);
  if ( NULL == wfd->attrs)
  {
    debug_critical("Failed to construct attributes\n");
    goto ERROR;
  }

  /* initialize gstreamer with configured parameter */
  if ( ! __mmwfd_gstreamer_init() )
  {
    debug_critical("Initializing gstreamer failed\n");
    goto ERROR;
  }

  /* creates the server object */
  wfd->server = gst_rtsp_server_new(__mmwfd_server_cb, wfd);
  if (NULL == wfd->server)
  {
    debug_error("Failed to create server...");
    goto ERROR;
  }

  /* Gets the media mapping object for the server */
  wfd->mapping = gst_rtsp_server_get_media_mapping (wfd->server);
  if (wfd->mapping == NULL)
  {
    debug_error ("No media mapping...");
    goto ERROR;
  }

  wfd->factory = gst_rtsp_media_factory_new ();
  if (wfd->factory == NULL)
  {
    debug_error ("Failed to create factory...");
    goto ERROR;
  }

  /* attach the test factory to the /test url */
  // TODO: Check why test url name is required.... ???
    gst_rtsp_media_mapping_add_factory (wfd->mapping, "/wfd1.0", wfd->factory);

  /* set wfd state to ready */
  MMWFD_SET_STATE(wfd, MM_WFD_STATE_NULL);

  debug_fleave();

  return MM_ERROR_NONE;

ERROR:

  /* release attributes */
  _mmwfd_deconstruct_attribute((mm_wfd_t *)wfd);

  return MM_ERROR_WFD_INTERNAL;
}


int _mmwfd_destroy (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*)hwfd;

  debug_fenter();

  /* check wfd handle */
  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  /* destroy can called at anytime */
  MMWFD_CHECK_STATE_RETURN_IF_FAIL ( wfd, MMWFD_COMMAND_DESTROY );

  g_object_unref (wfd->client);
  g_object_unref (wfd->server);
  g_object_unref (wfd->mapping);
  g_object_unref (wfd->factory);

  /* release attributes */
  _mmwfd_deconstruct_attribute( wfd );

  debug_fleave();

  return MM_ERROR_NONE;
}

int _mmwfd_realize (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd =  (mm_wfd_t*)hwfd;
  char *server_ip =NULL;
  int backlog_cnt = 1;
  char *port = 0;
  MMHandleType attrs = 0;
  int ret = MM_ERROR_NONE;

  debug_fenter();

  /* check wfd handle */
  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED )

  /* check current state */
  MMWFD_CHECK_STATE_RETURN_IF_FAIL( wfd, MMWFD_COMMAND_REALIZE );

  attrs = MMWFD_GET_ATTRS(wfd);
  if ( !attrs )
  {
    debug_error("fail to get attributes.\n");
    return MM_ERROR_WFD_INTERNAL;
  }

  /* set the server_ip address on WFD-server */
  mm_attrs_get_string_by_name ( attrs, "server_ip", &server_ip);
  gst_rtsp_server_set_address(wfd->server, server_ip);

  debug_log ("server_ip : %s", server_ip);

  /* set service address on WFD-server */
  mm_attrs_get_string_by_name(attrs, "server_port", &port);
  gst_rtsp_server_set_service (wfd->server, port);

  debug_log ("server_port : %s", port);

  /* set max clients allowed on WFD-server */
  mm_attrs_get_int_by_name(attrs, "max_client_count", &backlog_cnt);
  gst_rtsp_server_set_backlog (wfd->server, backlog_cnt);

  debug_log ("max_client_count : %d", backlog_cnt);

  /* attach the server to the default maincontext */
  if (gst_rtsp_server_attach (wfd->server, NULL) == 0)
  {
    debug_error ("Failed to attach server to context");
    return MM_ERROR_WFD_INTERNAL;
  }

  MMWFD_SET_STATE ( wfd, MM_WFD_STATE_READY );

  debug_fleave();

  return ret;
}

int _mmwfd_unrealize (MMHandleType hwfd)
{
  mm_wfd_t* wfd =  (mm_wfd_t*)hwfd;
  void *pool;

  int ret = MM_ERROR_NONE;

  debug_fenter();

  pool = gst_rtsp_server_get_session_pool (wfd->server);
  gst_rtsp_session_pool_cleanup (pool);
  g_object_unref (pool);

  debug_fleave();

  return ret;
}

int _mmwfd_connect (MMHandleType hwfd)
{
  mm_wfd_t* wfd =  (mm_wfd_t*)hwfd;

  int ret = MM_ERROR_NONE;

  debug_fenter();

  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  /* check current state */
  MMWFD_CHECK_STATE_RETURN_IF_FAIL( wfd, MMFWD_COMMAND_CONNECT);

  MMWFD_SET_STATE ( wfd, MM_WFD_STATE_CONNECTION_WAIT );

  /* start accepting the client.. this call is a blocking call & keep on waiting on accept () sys API */
  wfd->client = gst_rtsp_server_accept_client (wfd->server);
  if (wfd->client == NULL)
  {
    debug_error ("Error in client accept");
    return MM_ERROR_WFD_INTERNAL;
  }

  if (!gst_rtsp_server_negotiate_client (wfd->server, wfd->client))
  {
    debug_error ("Error in starting client");
    return MM_ERROR_WFD_INTERNAL;
  }

  MMWFD_SET_STATE ( wfd, MM_WFD_STATE_CONNECTED );

  debug_fleave();

  return ret;
}

int _mmwfd_start (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*) hwfd;
  gint ret = MM_ERROR_NONE;

  debug_fenter();

  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  /* check current state */
  MMWFD_CHECK_STATE_RETURN_IF_FAIL( wfd, MMWFD_COMMAND_START );

  /* set client params */
  gst_rtsp_server_set_client_params (wfd->server, wfd->client, WFD_INI()->videosrc_element, WFD_INI()->session_mode, WFD_INI()->videobitrate, WFD_INI()->mtu_size,
      WFD_INI()->infile);

  if (!gst_rtsp_server_start_client (wfd->server, wfd->client))
  {
    debug_error ("Error in starting client");
    return MM_ERROR_WFD_INTERNAL;
  }

  MMWFD_SET_STATE ( wfd, MM_WFD_STATE_PLAYING );

  debug_fleave();

  return ret;
}

int _mmwfd_pause (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*) hwfd;
  gint ret = MM_ERROR_NONE;

  debug_fenter();

  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  /* check current state */
  //MMWFD_CHECK_STATE_RETURN_IF_FAIL( wfd, MMWFD_COMMAND_PLAY );

  if (!gst_rtsp_server_pause_client (wfd->server, wfd->client))
  {
    debug_error ("Error in starting client");
    return MM_ERROR_WFD_INTERNAL;
  }

  MMWFD_SET_STATE ( wfd, MM_WFD_STATE_PAUSED);

  debug_fleave();

  return ret;
}

int _mmwfd_resume (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*) hwfd;
  gint ret = MM_ERROR_NONE;

  debug_fenter();

  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  /* check current state */
  //MMWFD_CHECK_STATE_RETURN_IF_FAIL( wfd, MMWFD_COMMAND_PAUSE );

  if (!gst_rtsp_server_resume_client (wfd->server, wfd->client))
  {
    debug_error ("Error in starting client");
    return MM_ERROR_WFD_INTERNAL;
  }

  MMWFD_SET_STATE ( wfd, MM_WFD_STATE_PLAYING );

  debug_fleave();

  return ret;
}

int _mmwfd_standby (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*) hwfd;
  gint ret = MM_ERROR_NONE;

  debug_fenter();
  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  /* check current state */
  //MMWFD_CHECK_STATE_RETURN_IF_FAIL( wfd, MMWFD_COMMAND_PLAY );

  if (!gst_rtsp_server_standby_client (wfd->server, wfd->client))
  {
    debug_error ("Error in client standby");
    return MM_ERROR_WFD_INTERNAL;
  }

  MMWFD_SET_STATE ( wfd, MM_WFD_STATE_PAUSED);

  debug_fleave();

  return ret;
}

int _mmwfd_stop (MMHandleType hwfd) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*) hwfd;
  gint ret = MM_ERROR_NONE;

  debug_fenter();

  return_val_if_fail ( wfd, MM_ERROR_WFD_NOT_INITIALIZED );

  /* check current state */
  //MMWFD_CHECK_STATE_RETURN_IF_FAIL( wfd, MMWFD_COMMAND_STOP );
  if (!gst_rtsp_server_stop_client (wfd->server, wfd->client))
  {
    debug_error ("Error in starting client");
    return MM_ERROR_WFD_INTERNAL;
  }
  debug_fleave();

  return ret;
}

int _mmwfd_get_state(MMHandleType hwfd, int *state) // @
{
  mm_wfd_t *wfd = (mm_wfd_t*)hwfd;

  return_val_if_fail(state, MM_ERROR_INVALID_ARGUMENT);

  *state = MMWFD_CURRENT_STATE(wfd);

  return MM_ERROR_NONE;
}

static const gchar *
__get_state_name ( int state )
{
  switch ( state )
  {
    case MM_WFD_STATE_NULL:
      return "NULL";
    case MM_WFD_STATE_READY:
      return "READY";
    case MM_WFD_STATE_PAUSED:
      return "PAUSED";
    case MM_WFD_STATE_CONNECTION_WAIT:
      return "WAIT_FOR_CONNECTION";
    case MM_WFD_STATE_CONNECTED:
      return "CONNECTED";
    case MM_WFD_STATE_PLAYING:
      return "PLAYING";
    case MM_WFD_STATE_NONE:
      return "NONE";
    default:
      debug_log ("\n\n\nstate value : %d\n\n\n", state);
      return "INVALID";
  }
}

static int
__mmwfd_check_state(mm_wfd_t* wfd, enum PlayerCommandState command)
{
  MMWfdStateType current_state = MM_WFD_STATE_NUM;
  MMWfdStateType pending_state = MM_WFD_STATE_NUM;
  MMWfdStateType target_state = MM_WFD_STATE_NUM;
  MMWfdStateType prev_state = MM_WFD_STATE_NUM;

  debug_fenter();

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  current_state = MMWFD_CURRENT_STATE(wfd);
  pending_state = MMWFD_PENDING_STATE(wfd);
  target_state = MMWFD_TARGET_STATE(wfd);
  prev_state = MMWFD_PREV_STATE(wfd);

  MMWFD_PRINT_STATE(wfd);

  debug_log("incomming command : %d \n", command );

  switch( command )
  {
    case MMWFD_COMMAND_CREATE:
    {
      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_NULL;

      if ( current_state == MM_WFD_STATE_NULL ||
        current_state == MM_WFD_STATE_READY ||
        current_state == MM_WFD_STATE_PAUSED ||
        current_state == MM_WFD_STATE_CONNECTION_WAIT ||
        current_state == MM_WFD_STATE_CONNECTED ||
        current_state == MM_WFD_STATE_PLAYING )
        goto NO_OP;
    }
    break;

    case MMWFD_COMMAND_DESTROY:
    {
      /* destroy can called anytime */

      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_NONE;
    }
    break;

    case MMWFD_COMMAND_REALIZE:
    {
      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_READY;

      if ( pending_state != MM_WFD_STATE_NONE )
      {
        goto INVALID_STATE;
      }
      else
      {
        /* need ready state to realize */
        if ( current_state == MM_WFD_STATE_READY )
          goto NO_OP;

        if ( current_state != MM_WFD_STATE_NULL )
          goto INVALID_STATE;
      }
    }
    break;

    case MMWFD_COMMAND_UNREALIZE:
    {
      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_NULL;

      if ( current_state == MM_WFD_STATE_NULL )
        goto NO_OP;
    }
    break;

    case MMFWD_COMMAND_CONNECT:
    {
      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_CONNECTED;

      // TODO: check any extra error handing is needed

      MMWFD_PRINT_STATE(wfd);

      debug_log ("target state is CONNECTED...");
    }
    break;

    case MMWFD_COMMAND_START:
    {
      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_PLAYING;

      if ( pending_state == MM_WFD_STATE_NONE )
      {
        if ( current_state == MM_WFD_STATE_PLAYING )
          goto NO_OP;
        else if ( current_state  != MM_WFD_STATE_READY &&
          current_state != MM_WFD_STATE_PAUSED  && current_state != MM_WFD_STATE_CONNECTED)
          goto INVALID_STATE;
      }
      else if ( pending_state == MM_WFD_STATE_PLAYING )
      {
        goto ALREADY_GOING;
      }
      else if ( pending_state == MM_WFD_STATE_PAUSED )
      {
        debug_log("wfd is going to paused state, just change the pending state as playing.\n");
      }
      else
      {
        goto INVALID_STATE;
      }
    }
    break;

    case MMWFD_COMMAND_STOP:
    {
      {
        MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_READY;

        if ( current_state == MM_WFD_STATE_READY )
          goto NO_OP;
      }

      /* need playing/paused state to stop */
      if ( current_state != MM_WFD_STATE_PLAYING &&
         current_state != MM_WFD_STATE_PAUSED )
        goto INVALID_STATE;
    }
    break;

    case MMWFD_COMMAND_PAUSE:
    {

      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_PAUSED;

      if ( pending_state == MM_WFD_STATE_NONE )
      {
        if ( current_state == MM_WFD_STATE_PAUSED )
          goto NO_OP;
        else if ( current_state != MM_WFD_STATE_PLAYING && current_state != MM_WFD_STATE_READY ) // support loading state of broswer
          goto INVALID_STATE;
      }
      else if ( pending_state == MM_WFD_STATE_PAUSED )
      {
        goto ALREADY_GOING;
      }
      else if ( pending_state == MM_WFD_STATE_PLAYING )
      {
        if ( current_state == MM_WFD_STATE_PAUSED ) {
          debug_log("wfd is PAUSED going to PLAYING, just change the pending state as PAUSED.\n");
        } else {
          goto INVALID_STATE;
        }
      }
    }
    break;

    case MMWFD_COMMAND_RESUME:
    {
      MMWFD_TARGET_STATE(wfd) = MM_WFD_STATE_PLAYING;

      if ( pending_state == MM_WFD_STATE_NONE )
      {
        if ( current_state == MM_WFD_STATE_PLAYING )
          goto NO_OP;
        else if (  current_state != MM_WFD_STATE_PAUSED )
          goto INVALID_STATE;
      }
      else if ( pending_state == MM_WFD_STATE_PLAYING )
      {
        goto ALREADY_GOING;
      }
      else if ( pending_state == MM_WFD_STATE_PAUSED )
      {
        debug_log("wfd is going to paused state, just change the pending state as playing.\n");
      }
      else
      {
        goto INVALID_STATE;
      }
    }
    break;

    default:
    break;
  }

  debug_log("status OK\n");

  wfd->cmd = command;

  debug_fleave();

  return MM_ERROR_NONE;


INVALID_STATE:
  debug_warning("since wfd is in wrong state(%s). it's not able to apply the command(%d)\n",
    MMWFD_STATE_GET_NAME(current_state), command);
  return MM_ERROR_WFD_INVALID_STATE;

NO_OP:
  debug_warning("wfd is in the desired state(%s). doing nothing\n", MMWFD_STATE_GET_NAME(current_state));
  return MM_ERROR_WFD_NO_OP;

ALREADY_GOING:
  debug_warning("wfd is already going to %s, doing nothing.\n", MMWFD_STATE_GET_NAME(pending_state));
  return MM_ERROR_WFD_NO_OP;
}

static gboolean
__mmwfd_gstreamer_init(void) // @
{
  static gboolean initialized = FALSE;
  static const int max_argc = 50;
  gint* argc = NULL;
  gchar** argv = NULL;
  GError *err = NULL;
  int i = 0;

  debug_fenter();

  if ( initialized )
  {
    debug_log("gstreamer already initialized.\n");
    return TRUE;
  }

  /* alloc */
  argc = malloc( sizeof(int) );
  argv = malloc( sizeof(gchar*) * max_argc );

  if ( !argc || !argv )
    goto ERROR;

  memset( argv, 0, sizeof(gchar*) * max_argc );

  /* add initial */
  *argc = 1;
  argv[0] = g_strdup( "mmwfd" );

  /* add gst_param */
  for ( i = 0; i < 5; i++ ) /* FIXIT : num of param is now fixed to 5. make it dynamic */
  {
    if ( strlen( WFD_INI()->gst_param[i] ) > 0 )
    {
      argv[*argc] = g_strdup( WFD_INI()->gst_param[i] );
      (*argc)++;
    }
  }

  /* we would not do fork for scanning plugins */
  argv[*argc] = g_strdup("--gst-disable-registry-fork");
  (*argc)++;

  /* check disable registry scan */
  if ( WFD_INI()->skip_rescan )
  {
    argv[*argc] = g_strdup("--gst-disable-registry-update");
    (*argc)++;
  }

  /* check disable segtrap */
  if ( WFD_INI()->disable_segtrap )
  {
    argv[*argc] = g_strdup("--gst-disable-segtrap");
    (*argc)++;
  }

  debug_log("initializing gstreamer with following parameter\n");
  debug_log("argc : %d\n", *argc);

  for ( i = 0; i < *argc; i++ )
  {
    debug_log("argv[%d] : %s\n", i, argv[i]);
  }


  /* initializing gstreamer */
  __ta__("gst_init time",

    if ( ! gst_init_check (argc, &argv, &err))
    {
      debug_error("Could not initialize GStreamer: %s\n", err ? err->message : "unknown error occurred");
      if (err)
      {
        g_error_free (err);
      }

      goto ERROR;
    }
  );

  /* release */
  for ( i = 0; i < *argc; i++ )
  {
    MMWFD_FREEIF( argv[i] );
  }

  MMWFD_FREEIF( argv );
  MMWFD_FREEIF( argc );

  /* done */
  initialized = TRUE;

  debug_fleave();

  return TRUE;

ERROR:

  MMWFD_FREEIF( argv );
  MMWFD_FREEIF( argc );

  return FALSE;
}


static gboolean
__mmwfd_set_state(mm_wfd_t* wfd, int state) // @
{
  MMWfdStateType current_state = MM_WFD_STATE_NONE;
  MMWfdStateType prev_state = MM_WFD_STATE_NONE;
  MMWfdStateType target_state = MM_WFD_STATE_NONE;
  MMWfdStateType pending_state = MM_WFD_STATE_NONE;
  MMMessageParamType msg = {0, };
  int asm_result = MM_ERROR_NONE;
  int new_state = state;

  debug_fenter();

  return_val_if_fail ( wfd, FALSE );

  prev_state = MMWFD_PREV_STATE(wfd);
  current_state = MMWFD_CURRENT_STATE(wfd);
  pending_state = MMWFD_PENDING_STATE(wfd);
  target_state = MMWFD_TARGET_STATE(wfd);

  debug_log ("added");
  MMWFD_PRINT_STATE(wfd);

  MMWFD_PREV_STATE(wfd) = current_state;
  MMWFD_CURRENT_STATE(wfd) = new_state;

  if ( pending_state == new_state )
    MMWFD_PENDING_STATE(wfd) = MM_WFD_STATE_NONE;

  if ( current_state == new_state )
  {
    debug_warning("already same state(%s)\n", MMWFD_STATE_GET_NAME(state));
    return TRUE;
  }

  MMWFD_PRINT_STATE(wfd);

  if (target_state == new_state)
  {
    msg.state.previous = prev_state;
    msg.state.current = new_state;

    MMWFD_POST_MSG( wfd, MM_MESSAGE_STATE_CHANGED, &msg );

    debug_log ("wfd reach the target state, then do something in each state(%s).\n", MMWFD_STATE_GET_NAME(target_state));
  }
  else
  {
    debug_log ("intermediate state, do nothing.\n");
    return TRUE;
  }

  switch ( target_state )
  {
    case MM_WFD_STATE_NULL:
    case MM_WFD_STATE_READY:
    case MM_WFD_STATE_CONNECTION_WAIT:
    case MM_WFD_STATE_CONNECTED:
    break;

    case MM_WFD_STATE_PAUSED:
    {
      /* special care for local playback. normaly we can get some content attribute
       * when the demuxer is changed to PAUSED. so we are trying it. it will be tried again
       * when PLAYING state has signalled if failed.
       * note that this is only happening pause command has come before the state of pipeline
       * reach to the PLAYING.
       */
      //_mmwfd_update_content_attrs( wfd );

    }
    break;

    case MM_WFD_STATE_PLAYING:
    {
      /* update attributes which are only available on playing status */
      //_mmwfd_update_content_attrs ( wfd );

      if ( wfd->cmd == MMWFD_COMMAND_START )
      {
      }

    }
    break;

    case MM_WFD_STATE_NONE:
    default:
      debug_warning("invalid target state, there is nothing to do.\n");
      break;
  }

  debug_fleave();

  return TRUE;
}

static int
__gst_set_message_callback(mm_wfd_t* wfd, MMMessageCallback callback, gpointer user_param) // @
{
  debug_fenter();

  if ( !wfd )
  {
    debug_warning("set_message_callback is called with invalid wfd handle\n");
    return MM_ERROR_WFD_NOT_INITIALIZED;
  }

  wfd->msg_cb = callback;
  wfd->msg_cb_param = user_param;

  debug_log("msg_cb : 0x%x     msg_cb_param : 0x%x\n", (guint)callback, (guint)user_param);

  debug_fleave();

  return MM_ERROR_NONE;
}

int
_mmwfd_set_message_callback(MMHandleType hwfd, MMMessageCallback callback, gpointer user_param) // @
{
  mm_wfd_t* wfd = (mm_wfd_t*)hwfd;

      return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  return __gst_set_message_callback(wfd, callback, user_param);
}

gboolean
__mmwfd_post_message(mm_wfd_t* wfd, enum MMMessageType msgtype, MMMessageParamType* param) // @
{
  return_val_if_fail( wfd, FALSE );

  if ( !wfd->msg_cb )
  {
    debug_warning("no msg callback. can't post\n");
    return FALSE;
  }

  //debug_log("Message (type : %d)  will be posted using msg-cb(%p). \n", msgtype, wfd->msg_cb);

  wfd->msg_cb(msgtype, param, wfd->msg_cb_param);

  return TRUE;
}


