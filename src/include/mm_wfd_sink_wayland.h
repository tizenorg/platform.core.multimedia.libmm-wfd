/*
* Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
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
*/


#ifndef __MM_WFD_SINK_WLCLIENT_H__
#define __MM_WFD_SINK_WLCLIENT_H__
#include <stdio.h>
#include <tbm_bufmgr.h>
#include <tizen-extension-client-protocol.h>
#include <wayland-client.h>
#include <mm_types.h>
#include <mm_debug.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	struct wl_display *display;
	struct wl_registry *registry;
	struct tizen_surface *tz_surface;
	struct tizen_resource *tz_resource;
} wl_client;

int mm_wfd_sink_wlclient_create (wl_client ** wlclient);
int mm_wfd_sink_wlclient_get_wl_window_wl_surface_id (wl_client * wlclient, struct wl_surface *surface, struct wl_display *display);
void mm_wfd_sink_wlclient_finalize (wl_client * wlclient);

#ifdef __cplusplus
}
#endif

#endif                          /* __MM_WFD_SINK_WLCLIENT_H__ */
