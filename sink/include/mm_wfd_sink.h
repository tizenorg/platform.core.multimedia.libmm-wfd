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

#ifndef _MM_WFD_SINK_H_
#define _MM_WFD_SINK_H_

#include <string.h>
#include <glib.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>

/**
 *  * Enumerations of wifi-display state.
 *   */
typedef enum {
  MM_WFD_SINK_STATE_NULL,       /**< wifi-display is created, but not realized yet */
  MM_WFD_SINK_STATE_READY,      /**< wifi-display is ready to play media */
  MM_WFD_SINK_STATE_PLAYING,    /**< wifi-display is now playing media */
  MM_WFD_SINK_STATE_PAUSED,     /**< wifi-display is paused while playing media */
  MM_WFD_SINK_STATE_TEARDOWN,   /**< wifi-display is paused while playing media */
  MM_WFD_SINK_STATE_NONE,       /**< wifi-display is not created yet */
  MM_WFD_SINK_STATE_NUM,        /**< Number of wifi-display states */
} MMWfdSinkStateType;

/**
 * Enumeration for attribute values types.
 */
typedef enum {
 MM_WFD_SINK_ATTRS_TYPE_INVALID = -1,        /**< Type is invalid */
 MM_WFD_SINK_ATTRS_TYPE_INT,                 /**< Integer type */
 MM_WFD_SINK_ATTRS_TYPE_DOUBLE,              /**< Double type */
 MM_WFD_SINK_ATTRS_TYPE_STRING,              /**< UTF-8 String type */
 MM_WFD_SINK_ATTRS_TYPE_DATA,                /**< Pointer type */
 MM_WFD_SINK_ATTRS_TYPE_ARRAY,               /**< Array type */
 MM_WFD_SINK_ATTRS_TYPE_RANGE,               /**< Range type */
 MM_WFD_SINK_ATTRS_TYPE_NUM,                 /**< Number of attribute type */
} MMWfdSinkAttrsType;

/**
 * Enumeration for attribute validation type.
 */
typedef enum {
 MM_WFD_SINK_ATTRS_VALID_TYPE_INVALID = -1,    /**< Invalid validation type */
 MM_WFD_SINK_ATTRS_VALID_TYPE_NONE,            /**< Do not check validity */
 MM_WFD_SINK_ATTRS_VALID_TYPE_INT_ARRAY,       /**< validity checking type of integer array */
 MM_WFD_SINK_ATTRS_VALID_TYPE_INT_RANGE,       /**< validity checking type of integer range */
 MM_WFD_SINK_ATTRS_VALID_TYPE_DOUBLE_ARRAY,    /**< validity checking type of double array */
 MM_WFD_SINK_ATTRS_VALID_TYPE_DOUBLE_RANGE,    /**< validity checking type of double range */
} MMWfdSinkAttrsValidType;

/**
 * Enumeration for attribute access flag.
 */
typedef enum {
 MM_WFD_SINK_ATTRS_FLAG_NONE = 0,               /**< None flag is set */
 MM_WFD_SINK_ATTRS_FLAG_READABLE = 1 << 0,      /**< Readable */
 MM_WFD_SINK_ATTRS_FLAG_WRITABLE = 1 << 1,      /**< Writable */
 MM_WFD_SINK_ATTRS_FLAG_MODIFIED = 1 << 2,      /**< Modified */

 MM_WFD_SINK_ATTRS_FLAG_RW = MM_WFD_SINK_ATTRS_FLAG_READABLE | MM_WFD_SINK_ATTRS_FLAG_WRITABLE, /**< Readable and Writable */
} MMWfdSinkAttrsFlag;

/**
 * Attribute validity structure
 */
typedef struct {
   MMWfdSinkAttrsType type;
   MMWfdSinkAttrsValidType validity_type;
   MMWfdSinkAttrsFlag flag;
  /**
    * a union that describes validity of the attribute.
    * Only when type is 'MM_ATTRS_TYPE_INT' or 'MM_ATTRS_TYPE_DOUBLE',
    * the attribute can have validity.
   */
   union {
    /**
       * Validity structure for integer array.
     */
    struct {
      int *array;  /**< a pointer of array */
      int count;   /**< size of array */
      int d_val;
    } int_array;
    /**
       * Validity structure for integer range.
     */
    struct {
      int min;   /**< minimum range */
      int max;   /**< maximum range */
      int d_val;
    } int_range;
    /**
    * Validity structure for double array.
    */
    struct {
      double   * array;  /**< a pointer of array */
      int    count;   /**< size of array */
      double d_val;
    } double_array;
    /**
    * Validity structure for double range.
    */
    struct {
      double   min;   /**< minimum range */
      double   max;   /**< maximum range */
      double d_val;
    } double_range;
  };
} MMWfdSinkAttrsInfo;

typedef void (*MMWFDMessageCallback)(MMWfdSinkStateType type, void *user_data);

int mm_wfd_sink_create(MMHandleType *wfd_sink);
int mm_wfd_sink_connect(MMHandleType wfd_sink, const char *uri);
int mm_wfd_sink_start(MMHandleType wfd_sink);
int mm_wfd_sink_stop(MMHandleType wfd_sink);
int mm_wfd_sink_unrealize(MMHandleType wfd_sink);
int mm_wfd_sink_destroy(MMHandleType wfd_sink);
int mm_wfd_sink_set_message_callback(MMHandleType wfd_sink, MMWFDMessageCallback callback, void *user_data);
int mm_wfd_sink_set_display_surface_type(MMHandleType wfd,  gint display_surface_type);
int mm_wfd_sink_set_display_overlay(MMHandleType wfd,  void *display_overlay);
int mm_wfd_sink_set_display_method(MMHandleType wfd,  gint display_method);
int mm_wfd_sink_set_display_visible(MMHandleType wfd,  gint display_visible);

#endif

