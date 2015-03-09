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

#ifndef __MM_WFD_ATTRS_H__
#define __MM_WFD_ATTRS_H__

#include <string.h>
#include <glib.h>
#include <mm_message.h>
#include <mm_error.h>
#include <mm_types.h>
#include <mm_attrs_private.h>
#include <mm_attrs.h>
#include <mm_debug.h>

/* general */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#endif
#define MMWFD_MAX_INT (2147483647)

/**
 * Enumeration for attribute values types.
 */
typedef enum {
 MM_WFD_ATTRS_TYPE_INVALID = -1,        /**< Type is invalid */
 MM_WFD_ATTRS_TYPE_INT,                 /**< Integer type */
 MM_WFD_ATTRS_TYPE_DOUBLE,              /**< Double type */
 MM_WFD_ATTRS_TYPE_STRING,              /**< UTF-8 String type */
 MM_WFD_ATTRS_TYPE_DATA,                /**< Pointer type */
 MM_WFD_ATTRS_TYPE_ARRAY,               /**< Array type */
 MM_WFD_ATTRS_TYPE_RANGE,               /**< Range type */
 MM_WFD_ATTRS_TYPE_NUM,                 /**< Number of attribute type */
} MMWfdAttrsType;

/**
 * Enumeration for attribute validation type.
 */
typedef enum {
 MM_WFD_ATTRS_VALID_TYPE_INVALID = -1,       /**< Invalid validation type */
 MM_WFD_ATTRS_VALID_TYPE_NONE,               /**< Do not check validity */
 MM_WFD_ATTRS_VALID_TYPE_INT_ARRAY,          /**< validity checking type of integer array */
 MM_WFD_ATTRS_VALID_TYPE_INT_RANGE,          /**< validity checking type of integer range */
 MM_WFD_ATTRS_VALID_TYPE_DOUBLE_ARRAY,       /**< validity checking type of double array */
 MM_WFD_ATTRS_VALID_TYPE_DOUBLE_RANGE,       /**< validity checking type of double range */
} MMWfdAttrsValidType;

/**
 * Enumeration for attribute access flag.
 */
typedef enum {
 MM_WFD_ATTRS_FLAG_NONE = 0,              /**< None flag is set */
 MM_WFD_ATTRS_FLAG_READABLE = 1 << 0,     /**< Readable */
 MM_WFD_ATTRS_FLAG_WRITABLE = 1 << 1,     /**< Writable */
 MM_WFD_ATTRS_FLAG_MODIFIED = 1 << 2,     /**< Modified */

 MM_WFD_ATTRS_FLAG_RW = MM_WFD_ATTRS_FLAG_READABLE | MM_WFD_ATTRS_FLAG_WRITABLE, /**< Readable and Writable */
} MMWfdAttrsFlag;

/**
 * Attribute validity structure
 */
typedef struct {
   MMWfdAttrsType type;
   MMWfdAttrsValidType validity_type;
   MMWfdAttrsFlag flag;
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
} MMWfdAttrsInfo;

MMHandleType _mmwfd_construct_attribute(MMHandleType hwfd);
void  _mmwfd_deconstruct_attribute( MMHandleType hwfd);
int _mmwfd_set_attribute(MMHandleType hwfd,  char **err_atr_name, const char *attribute_name, va_list args_list);
int _mmwfd_get_attributes_info(MMHandleType handle,  const char *attribute_name, MMWfdAttrsInfo *dst_info);
int _mmwfd_get_attribute(MMHandleType handle,  char **err_attr_name, const char *attribute_name, va_list args_list);
#endif /* __MM_WFD_ATTRS_H__ */