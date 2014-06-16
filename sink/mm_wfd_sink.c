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

int mm_wfd_sink_create(MMHandleType *wfd_sink, const char *uri)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);
	return_val_if_fail(uri, MM_ERROR_WFD_NOT_INITIALIZED);

	mm_wfd_sink_t *new_wfd_sink = NULL;

	result = _mm_wfd_sink_create(&new_wfd_sink, uri);
	*wfd_sink = new_wfd_sink;
	return result;
}

int mm_wfd_sink_start(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	result = _mm_wfd_sink_start((mm_wfd_sink_t *)wfd_sink);
	return result;
}

int mm_wfd_sink_stop(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	result = _mm_wfd_sink_stop((mm_wfd_sink_t *)wfd_sink);
	return result;
}

int mm_wfd_sink_destroy(MMHandleType wfd_sink)
{
	int result = MM_ERROR_NONE;
	return_val_if_fail(wfd_sink, MM_ERROR_WFD_NOT_INITIALIZED);

	result = _mm_wfd_sink_destroy((mm_wfd_sink_t *)wfd_sink);
	return result;
}

