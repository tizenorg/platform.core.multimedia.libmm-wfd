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

#ifndef __MM_WFD_SINK_DLOG_H__
#define __MM_WFD_SINK_DLOG_H__

#include <dlog.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MM_WFD_SINK"

#define FONT_COLOR_RESET    "\033[0m"
#define FONT_COLOR_RED      "\033[31m"
#define FONT_COLOR_GREEN    "\033[32m"
#define FONT_COLOR_YELLOW   "\033[33m"
#define FONT_COLOR_BLUE     "\033[34m"
#define FONT_COLOR_PURPLE   "\033[35m"
#define FONT_COLOR_CYAN     "\033[36m"
#define FONT_COLOR_GRAY     "\033[37m"

#define wfd_sink_debug(fmt, arg...) do { \
		LOGD(FONT_COLOR_RESET""fmt"", ##arg);     \
	} while (0)

#define wfd_sink_info(fmt, arg...) do { \
		LOGI(FONT_COLOR_GREEN""fmt""FONT_COLOR_RESET, ##arg);     \
	} while (0)

#define wfd_sink_error(fmt, arg...) do { \
		LOGE(FONT_COLOR_RED""fmt""FONT_COLOR_RESET, ##arg);     \
	} while (0)

#define wfd_sink_warning(fmt, arg...) do { \
		LOGW(FONT_COLOR_YELLOW""fmt""FONT_COLOR_RESET, ##arg);     \
	} while (0)

#define wfd_sink_debug_fenter() do { \
		LOGD(FONT_COLOR_RESET"<Enter>");     \
	} while (0)

#define wfd_sink_debug_fleave() do { \
		LOGD(FONT_COLOR_RESET"<Leave>");     \
	} while (0)

#define wfd_sink_error_fenter() do { \
		LOGE(FONT_COLOR_RED"NO-ERROR : <Enter>"FONT_COLOR_RESET);     \
	} while (0)

#define wfd_sink_error_fleave() do { \
		LOGE(FONT_COLOR_RED"NO-ERROR : <Leave>"FONT_COLOR_RESET);     \
	} while (0)

#define wfd_sink_sucure_info(fmt, arg...) do { \
		SECURE_LOGI(FONT_COLOR_GREEN""fmt""FONT_COLOR_RESET, ##arg);     \
	} while (0)

#define wfd_sink_return_if_fail(expr)	\
	if(!(expr)) {	\
		wfd_sink_error(FONT_COLOR_RED"failed [%s]\n"FONT_COLOR_RESET, #expr);	\
		return; \
	}

#define wfd_sink_return_val_if_fail(expr, val)	\
	if (!(expr)) {	\
		wfd_sink_error(FONT_COLOR_RED"failed [%s]\n"FONT_COLOR_RESET, #expr);	\
		return val; \
	}

#define wfd_sink_assert_not_reached() \
	{ \
		wfd_sink_error(FONT_COLOR_RED"assert_not_reached()"FONT_COLOR_RESET); \
		assert(0); \
	}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MM_WFD_SINK_DLOG_H__ */

