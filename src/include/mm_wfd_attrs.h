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
#define	__MM_WFD_ATTRS_H__


#include <mm_attrs_private.h>
#include <mm_attrs.h>
#include <mm_wfd_priv.h>
#include <mm_wfd.h>

/* general */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
#endif
#define MMWFD_MAX_INT	(2147483647)

MMHandleType _mmwfd_construct_attribute(MMHandleType hwfd);
void  _mmwfd_deconstruct_attribute( MMHandleType hwfd);
int _mmwfd_set_attribute(MMHandleType hwfd,  char **err_atr_name, const char *attribute_name, va_list args_list);
int _mmwfd_get_attributes_info(MMHandleType handle,  const char *attribute_name, MMWfdAttrsInfo *dst_info);
int _mmwfd_get_attribute(MMHandleType handle,  char **err_attr_name, const char *attribute_name, va_list args_list);
#endif /* __MM_WFD_ATTRS_H__ */




