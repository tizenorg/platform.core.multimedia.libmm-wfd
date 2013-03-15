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

#include "mm_wfd.h"
#include <gst/gst.h>
#include "mm_wfd_priv.h"
#include "mm_wfd_attrs.h"
#include "mm_wfd_ini.h"

int mm_wfd_create(MMHandleType *wfd)
{
  int result = MM_ERROR_NONE;
  mm_wfd_t* new_wfd = NULL;

  debug_fenter();
  debug_log("mm_wfd_create \n");

  return_val_if_fail(wfd, MM_ERROR_WFD_INVALID_ARGUMENT);

  // TODO: need to check whether g_thread needed or not ....
  if (!g_thread_supported ())
    g_thread_init (NULL);

  MMTA_INIT();

  /* load options in ini file */
  __ta__("mm_wfd_ini_load",
    result = mm_wfd_ini_load();
  )

  if(result != MM_ERROR_NONE)
    goto ERROR;
  debug_log("mm_wfd_create mm_wfd_ini_load\n");

  /* alloc wfd structure */
  new_wfd = (MMHandleType)g_malloc(sizeof(mm_wfd_t));
  if ( !new_wfd )
  {
    debug_critical("Cannot allocate memory for wifi-display handle\n");
    result = MM_ERROR_WFD_NO_FREE_SPACE;
    goto ERROR;
  }

  memset(new_wfd, 0, sizeof(mm_wfd_t));

  /* create wfd lock */
  new_wfd->cmd_lock = g_mutex_new();

  if ( ! new_wfd->cmd_lock )
  {
    debug_critical("failed to create wifi-display lock\n");
    result = MM_ERROR_WFD_NO_FREE_SPACE;
    goto ERROR;
  }

  __ta__("[KPI] create wifi-display service",
    result = _mmwfd_create ((MMHandleType)new_wfd);
  )

  if(result != MM_ERROR_NONE)
    goto ERROR;

  *wfd = (MMHandleType)new_wfd;

  debug_fleave ();
  return result;

ERROR:

  debug_error ("ERROR");
  if ( new_wfd )
  {
    _mmwfd_destroy( (MMHandleType)new_wfd );
    MMWFD_FREEIF( new_wfd );
  }
  
  *wfd = NULL;
  return result;
}


int mm_wfd_destroy(MMHandleType wfd)
{
  int result = MM_ERROR_NONE;
  debug_fenter();
  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
  MMWFD_CMD_LOCK( wfd );
  __ta__("[KPI] destroy media wfd service",
  result = _mmwfd_destroy(wfd);
  )
  MMWFD_CMD_UNLOCK( wfd );
  /* free wfd */
  g_free( (void*)wfd );

  MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_FILE);
  MMTA_RELEASE();

  debug_leave();
  return result;
}


int mm_wfd_realize(MMHandleType wfd)
{
  int result = MM_ERROR_NONE;

  debug_fenter();

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  __ta__("[KPI] initialize wifi display service",
  result = _mmwfd_realize(wfd);
  )

  MMWFD_CMD_UNLOCK( wfd );

  debug_leave();

  return result;
}


int mm_wfd_unrealize(MMHandleType wfd)
{
  int result = MM_ERROR_NONE;

  debug_fenter();

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  __ta__("[KPI] cleanup wifi display service",
  result = _mmwfd_unrealize(wfd);
  )

  MMWFD_CMD_UNLOCK( wfd );

  debug_leave();

  return result;
}

int mm_wfd_connect (MMHandleType wfd)
{
  int result = MM_ERROR_NONE;

  debug_leave();

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  __ta__("[KPI] connecting wifi display service",
  result = _mmwfd_connect(wfd);
  )

  MMWFD_CMD_UNLOCK( wfd );

  debug_leave();

  return result;

}


int mm_wfd_start(MMHandleType wfd)
{
  int result = MM_ERROR_NONE;

  debug_log("\n");

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  MMTA_ACUM_ITEM_BEGIN("[KPI] start media wfd service", false);
  result = _mmwfd_start(wfd);

  MMWFD_CMD_UNLOCK( wfd );

  return result;
}

int mm_wfd_pause (MMHandleType wfd)
{
  int result = MM_ERROR_NONE;

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  __ta__("[KPI] pause media wfd service",
  result = _mmwfd_pause(wfd);
  )

  MMWFD_CMD_UNLOCK( wfd );

  return result;
}

int mm_wfd_resume (MMHandleType wfd)
{
  int result = MM_ERROR_NONE;

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK(wfd);

  __ta__("[KPI] resume media wfd service",
  result = _mmwfd_resume(wfd);
  )

  MMWFD_CMD_UNLOCK(wfd);

  return result;
}

int mm_wfd_standby (MMHandleType wfd)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  __ta__("[KPI] standby media wfd service",
  result = _mmwfd_standby(wfd);
  )

  MMWFD_CMD_UNLOCK(wfd);

  return result;
}

int mm_wfd_stop(MMHandleType wfd)
{
  int result = MM_ERROR_NONE;

  debug_log("\n");

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  __ta__("[KPI] stop media wfd service",
  result = _mmwfd_stop(wfd);
  )

  MMWFD_CMD_UNLOCK( wfd );

  return result;
}

int mm_wfd_get_state(MMHandleType wfd, MMWfdStateType *state)
{
  int result = MM_ERROR_NONE;

  debug_log("\n");

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
  return_val_if_fail(state, MM_ERROR_COMMON_INVALID_ARGUMENT);

  *state = MM_WFD_STATE_NULL;

  MMWFD_CMD_LOCK( wfd );

  result = _mmwfd_get_state(wfd, state); /* FIXIT : why int* ? */

  MMWFD_CMD_UNLOCK( wfd );

  return result;
}


int mm_wfd_set_message_callback(MMHandleType wfd, MMMessageCallback callback, void *user_param)
{
  int result = MM_ERROR_NONE;

  debug_log("\n");

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_CMD_LOCK( wfd );

  result = _mmwfd_set_message_callback(wfd, callback, user_param);

  MMWFD_CMD_UNLOCK( wfd );

  return result;
}

int mm_wfd_set_attribute(MMHandleType wfd,  char **err_attr_name, const char *first_attribute_name, ...)
{
  int result = MM_ERROR_NONE;
  va_list var_args;

  debug_log("\n");

  return_val_if_fail(wfd, MM_ERROR_WFD_NOT_INITIALIZED);
  return_val_if_fail(first_attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);

  MMWFD_CMD_LOCK( wfd );

  va_start (var_args, first_attribute_name);
  result = _mmwfd_set_attribute(wfd, err_attr_name, first_attribute_name, var_args);
  va_end (var_args);

  MMWFD_CMD_UNLOCK( wfd );

  return result;
}

