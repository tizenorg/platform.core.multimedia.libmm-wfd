/*
 * libmm-wfd
 *
 * Copyright (c) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, ByungWook Jang <bw.jang@samsung.com>,
 * Maksym Ukhanov <m.ukhanov@samsung.com>, Hyunjun Ko <zzoon.ko@samsung.com>
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
#include <mm_debug.h>

#include "mm_wfd_sink.h"
#include "mm_wfd_sink_priv.h"

int mm_wfd_sink_create(MMHandleType *wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  mm_wfd_sink_t *new_wfd_sink = NULL;

  mm_wfd_ini_load(FALSE);

  result = _mm_wfd_sink_create(&new_wfd_sink);
  if(result != MM_ERROR_NONE)
    goto ERROR;
  /* create wfd lock */
  new_wfd_sink->cmd_lock = g_mutex_new();

  if (!new_wfd_sink->cmd_lock)
  {
    debug_critical("failed to create wifi-display lock\n");
    result = MM_ERROR_WFD_NO_FREE_SPACE;
    goto ERROR;
  }
  *wfd_sink = (MMHandleType)new_wfd_sink;
  return result;
ERROR:
  debug_error ("ERROR");
  if ( new_wfd_sink )
  {
    MMWFDSINK_FREEIF(new_wfd_sink->cmd_lock);
    _mm_wfd_sink_destroy(new_wfd_sink );
  }

  *wfd_sink = (MMHandleType)NULL;
  return result;
}

int mm_wfd_sink_connect(MMHandleType wfd_sink, const char *uri)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
  return_val_if_fail(uri, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_SINK_CMD_LOCK(wfd_sink);
  result = _mm_wfd_sink_connect((mm_wfd_sink_t *)wfd_sink, uri);
  MMWFD_SINK_CMD_UNLOCK(wfd_sink);

  return result;
}

int mm_wfd_sink_start(MMHandleType wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_SINK_CMD_LOCK(wfd_sink);
  result = _mm_wfd_sink_start((mm_wfd_sink_t *)wfd_sink);
  MMWFD_SINK_CMD_UNLOCK(wfd_sink);

  return result;
}

int mm_wfd_sink_stop(MMHandleType wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_SINK_CMD_LOCK(wfd_sink);
  result = _mm_wfd_sink_stop((mm_wfd_sink_t *)wfd_sink);
  MMWFD_SINK_CMD_UNLOCK(wfd_sink);

  return result;
}

int mm_wfd_sink_destroy(MMHandleType wfd_sink)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_SINK_CMD_LOCK(wfd_sink);
  result = _mm_wfd_sink_destroy((mm_wfd_sink_t *)wfd_sink);
  MMWFD_SINK_CMD_UNLOCK(wfd_sink);

  return result;
}

int mm_wfd_sink_set_message_callback(MMHandleType wfd_sink, MMWFDMessageCallback callback, void *user_data)
{
  int result = MM_ERROR_NONE;
  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

  MMWFD_SINK_CMD_LOCK(wfd_sink);
  result = _mm_wfd_set_message_callback((mm_wfd_sink_t *)wfd_sink, callback, user_data);
  MMWFD_SINK_CMD_UNLOCK(wfd_sink);

  return result;
}

static int __mm_wfd_set_attribute(MMHandleType wfd_sink,  char **err_attr_name, const char *first_attribute_name, ...)
{
  int result = MM_ERROR_NONE;
  va_list var_args;

  debug_log("\n");

  return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
  return_val_if_fail(first_attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);

  MMWFD_SINK_CMD_LOCK(wfd_sink);
  va_start (var_args, first_attribute_name);
  result = _mmwfd_set_attribute(MMWFDSINK_GET_ATTRS(wfd_sink), err_attr_name, first_attribute_name, var_args);
  va_end (var_args);

  MMWFD_SINK_CMD_UNLOCK(wfd_sink);

  return result;
}

int mm_wfd_sink_set_display_surface_type(MMHandleType wfd,  gint display_surface_type)
{
  char *err_name = NULL;
  return __mm_wfd_set_attribute(wfd, &err_name, "display_surface_type", display_surface_type, NULL);
}

int mm_wfd_sink_set_display_overlay(MMHandleType wfd,  void *display_overlay)
{
  char *err_name = NULL;
  return __mm_wfd_set_attribute(wfd, &err_name, "display_overlay", display_overlay, sizeof(display_overlay), NULL);
}

int mm_wfd_sink_set_display_method(MMHandleType wfd,  gint display_method)
{
  char *err_name = NULL;
  return __mm_wfd_set_attribute(wfd, &err_name, "display_method", display_method, NULL);
}

int mm_wfd_sink_set_display_visible(MMHandleType wfd,  gint display_visible)
{
  char *err_name = NULL;
  return __mm_wfd_set_attribute(wfd, &err_name, "display_visible", display_visible, NULL);
}
