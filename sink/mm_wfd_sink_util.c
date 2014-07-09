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
#include "mm_wfd_sink_priv.h"
#include "mm_wfd_sink_util.h"

gboolean
_mm_wfd_sink_dump_pipeline_state(mm_wfd_sink_t *wfd_sink)
{
	GstIterator*iter = NULL;
	gboolean done = FALSE;

	GstElement *item = NULL;
	GstElementFactory *factory = NULL;

	GstState state = GST_STATE_VOID_PENDING;
	GstState pending = GST_STATE_VOID_PENDING;
	GstClockTime time = 200*GST_MSECOND;

	debug_fenter();

	return_val_if_fail ( wfd_sink && wfd_sink->sink_pipeline, FALSE );

	iter = gst_bin_iterate_recurse(GST_BIN(wfd_sink->sink_pipeline));

	if ( iter != NULL )
	{
		while (!done) {
			 switch ( gst_iterator_next (iter, (gpointer)&item) )
			 {
			   case GST_ITERATOR_OK:
			   	gst_element_get_state(GST_ELEMENT (item),&state, &pending,time);

			   	factory = gst_element_get_factory (item) ;
				if (factory)
				{
					 debug_error("%s:%s : From:%s To:%s   refcount : %d\n", GST_OBJECT_NAME(factory) , GST_ELEMENT_NAME(item) ,
					 	gst_element_state_get_name(state), gst_element_state_get_name(pending) , GST_OBJECT_REFCOUNT_VALUE(item));
				}
				 gst_object_unref (item);
				 break;
			   case GST_ITERATOR_RESYNC:
				 gst_iterator_resync (iter);
				 break;
			   case GST_ITERATOR_ERROR:
				 done = TRUE;
				 break;
			   case GST_ITERATOR_DONE:
				 done = TRUE;
				 break;
			 }
		}
	}

	item = GST_ELEMENT(wfd_sink->sink_pipeline);

	gst_element_get_state(GST_ELEMENT (item),&state, &pending,time);

	factory = gst_element_get_factory (item) ;

	if (factory)
	{
		debug_error("%s:%s : From:%s To:%s  refcount : %d\n",
			GST_OBJECT_NAME(factory),
			GST_ELEMENT_NAME(item),
			gst_element_state_get_name(state),
			gst_element_state_get_name(pending),
			GST_OBJECT_REFCOUNT_VALUE(item) );
	}

	if ( iter )
		gst_iterator_free (iter);

	debug_fleave();

	return FALSE;
}

