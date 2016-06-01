/*
 * Enumeration for WFD
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

#ifndef _MM_WFD_SINK_WFD_ENUM_H_
#define _MM_WFD_SINK_WFD_ENUM_H_

typedef enum {
	WFD_AUDIO_UNKNOWN 	= 0,
	WFD_AUDIO_LPCM		= (1 << 0),
	WFD_AUDIO_AAC		= (1 << 1),
	WFD_AUDIO_AC3		= (1 << 2)
} WFDAudioFormats;

typedef enum {
	WFD_FREQ_UNKNOWN = 0,
	WFD_FREQ_44100 	 = (1 << 0),
	WFD_FREQ_48000	 = (1 << 1)
} WFDAudioFreq;

typedef enum {
	WFD_CHANNEL_UNKNOWN = 0,
	WFD_CHANNEL_2 	 	= (1 << 0),
	WFD_CHANNEL_4		= (1 << 1),
	WFD_CHANNEL_6		= (1 << 2),
	WFD_CHANNEL_8		= (1 << 3)
} WFDAudioChannels;


typedef enum {
	WFD_VIDEO_UNKNOWN = 0,
	WFD_VIDEO_H264	  = (1 << 0)
} WFDVideoCodecs;

typedef enum {
	WFD_VIDEO_CEA_RESOLUTION = 0,
	WFD_VIDEO_VESA_RESOLUTION,
	WFD_VIDEO_HH_RESOLUTION
} WFDVideoNativeResolution;

#define WFD_CEA_UNKNOWN	        0
#define WFD_CEA_640x480P60	   (1 << 0)
#define WFD_CEA_720x480P60	   (1 << 1)
#define WFD_CEA_720x480I60	   (1 << 2)
#define WFD_CEA_720x576P50	   (1 << 3)
#define WFD_CEA_720x576I50	   (1 << 4)
#define WFD_CEA_1280x720P30    (1 << 5)
#define WFD_CEA_1280x720P60    (1 << 6)
#define WFD_CEA_1920x1080P30   (1 << 7)
#define WFD_CEA_1920x1080P60   (1 << 8)
#define WFD_CEA_1920x1080I60   (1 << 9)
#define WFD_CEA_1280x720P25    (1 << 10)
#define WFD_CEA_1280x720P50    (1 << 11)
#define WFD_CEA_1920x1080P25   (1 << 12)
#define WFD_CEA_1920x1080P50   (1 << 13)
#define WFD_CEA_1920x1080I50   (1 << 14)
#define WFD_CEA_1280x720P24    (1 << 15)
#define WFD_CEA_1920x1080P24   (1 << 16)
#define WFD_CEA_3840x2160P24   (1 << 17)
#define WFD_CEA_3840x2160P25   (1 << 18)
#define WFD_CEA_3840x2160P30   (1 << 19)
#define WFD_CEA_3840x2160P50   (1 << 20)
#define WFD_CEA_3840x2160P60   (1 << 21)
#define WFD_CEA_4096x2160P24   (1 << 22)
#define WFD_CEA_4096x2160P25   (1 << 23)
#define WFD_CEA_4096x2160P30   (1 << 24)
#define WFD_CEA_4096x2160P50   (1 << 25)
#define WFD_CEA_4096x2160P60   (1 << 26)

#define WFD_VESA_NONE            0
#define WFD_VESA_800x600P30     (1 << 0)
#define WFD_VESA_800x600P60     (1 << 1)
#define WFD_VESA_1024x768P30    (1 << 2)
#define WFD_VESA_1024x768P60    (1 << 3)
#define WFD_VESA_1152x864P30	  (1 << 4)
#define WFD_VESA_1152x864P60	  (1 << 5)
#define WFD_VESA_1280x768P30	  (1 << 6)
#define WFD_VESA_1280x768P60	  (1 << 7)
#define WFD_VESA_1280x800P30	  (1 << 8)
#define WFD_VESA_1280x800P60	  (1 << 9)
#define WFD_VESA_1360x768P30	  (1 << 10)
#define WFD_VESA_1360x768P60	  (1 << 11)
#define WFD_VESA_1366x768P30	  (1 << 12)
#define WFD_VESA_1366x768P60	  (1 << 13)
#define WFD_VESA_1280x1024P30	  (1 << 14)
#define WFD_VESA_1280x1024P60	  (1 << 15)
#define WFD_VESA_1400x1050P30	  (1 << 16)
#define WFD_VESA_1400x1050P60	  (1 << 17)
#define WFD_VESA_1440x900P30	  (1 << 18)
#define WFD_VESA_1440x900P60	  (1 << 19)
#define WFD_VESA_1600x900P30	  (1 << 20)
#define WFD_VESA_1600x900P60	  (1 << 21)
#define WFD_VESA_1600x1200P30	  (1 << 22)
#define WFD_VESA_1600x1200P60	  (1 << 23)
#define WFD_VESA_1680x1024P30	  (1 << 24)
#define WFD_VESA_1680x1024P60	  (1 << 25)
#define WFD_VESA_1680x1050P30	  (1 << 26)
#define WFD_VESA_1680x1050P60	  (1 << 27)
#define WFD_VESA_1920x1200P30	  (1 << 28)
#define WFD_VESA_1920x1200P60	  (1 << 29)
#define WFD_VESA_2560x1440P30	  (1 << 30)
#define WFD_VESA_2560x1440P60	  (1 << 31)
#define WFD_VESA_2560x1600P30	  (1ULL << 32)
#define WFD_VESA_2560x1600P60	  (1ULL << 33)

#define WFD_HH_NONE		     0
#define WFD_HH_800x480P30	(1 << 0)
#define WFD_HH_800x480P60	(1 << 1)
#define WFD_HH_854x480P30	(1 << 2)
#define WFD_HH_854x480P60	(1 << 3)
#define WFD_HH_864x480P30	(1 << 4)
#define WFD_HH_864x480P60	(1 << 5)
#define WFD_HH_640x360P30	(1 << 6)
#define WFD_HH_640x360P60	(1 << 7)
#define WFD_HH_960x540P30	(1 << 8)
#define WFD_HH_960x540P60	(1 << 9)
#define WFD_HH_848x480P30	(1 << 10)
#define WFD_HH_848x480P60	(1 << 11)


typedef enum {
	WFD_H264_UNKNOWN_PROFILE = 0,
	WFD_H264_BASE_PROFILE	= (1 << 0),
	WFD_H264_HIGH_PROFILE	= (1 << 1)
} WFDVideoH264Profile;

typedef enum {
	WFD_H264_LEVEL_UNKNOWN = 0,
	WFD_H264_LEVEL_3_1   = (1 << 0),
	WFD_H264_LEVEL_3_2   = (1 << 1),
	WFD_H264_LEVEL_4       = (1 << 2),
	WFD_H264_LEVEL_4_1   = (1 << 3),
	WFD_H264_LEVEL_4_2   = (1 << 4)
} WFDVideoH264Level;

#endif /*_MM_WFD_SINK_WFD_ENUM_H_*/
