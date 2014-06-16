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

#include "mm_wfd_attrs.h"

typedef struct{
	char *name;
	int value_type;
	int flags;				// r, w
	void *default_value;
	int valid_type;			// validity type
	int value_min;			//<- set validity value range
	int value_max;		//->
}MMWfdAttrsSpec;

static gboolean __mmwfd_apply_attribute(MMHandleType handle, const char *attribute_name);

MMHandleType
_mmwfd_construct_attribute(MMHandleType handle)
{
	int idx = 0;
	MMHandleType attrs = 0;
	int num_of_attrs = 0;
	mmf_attrs_construct_info_t *base = NULL;

	debug_fenter();

    return_val_if_fail(handle, (MMHandleType)NULL);

	MMWfdAttrsSpec wfd_attrs[] =
	{
		{
			"server_ip",
			MM_ATTRS_TYPE_STRING,
			MM_ATTRS_FLAG_RW,
			(void *)"127.0.0.1",
			MM_ATTRS_VALID_TYPE_NONE,
			0,
			0
		},

		{
			"server_port",
			MM_ATTRS_TYPE_STRING,
			MM_ATTRS_FLAG_RW,
			(void *)"8554",
			MM_ATTRS_VALID_TYPE_NONE,
			0,
			0
		},

		{
			"max_client_count",
			MM_ATTRS_TYPE_INT,
			MM_ATTRS_FLAG_RW,
			(void *)1,
			MM_ATTRS_VALID_TYPE_INT_RANGE,
			0,
			10
		},

	};

	num_of_attrs = ARRAY_SIZE(wfd_attrs);

	base = (mmf_attrs_construct_info_t* )malloc(num_of_attrs * sizeof(mmf_attrs_construct_info_t));

	if ( !base )
	{
		debug_error("Cannot create mmwfd attribute\n");
		goto ERROR;
	}

	/* initialize values of attributes */
	for ( idx = 0; idx < num_of_attrs; idx++ )
	{
		base[idx].name = wfd_attrs[idx].name;
		base[idx].value_type = wfd_attrs[idx].value_type;
		base[idx].flags = wfd_attrs[idx].flags;
		base[idx].default_value = wfd_attrs[idx].default_value;
	}

	attrs = mmf_attrs_new_from_data(
					"mmwfd_attrs",
					base,
					num_of_attrs,
					NULL,
					NULL);

	MMWFD_FREEIF(base);

	if ( !attrs )
	{
		debug_error("Cannot create mmwfd attribute\n");
		goto ERROR;
	}

	/* set validity type and range */
	for ( idx = 0; idx < num_of_attrs; idx++ )
	{
		switch ( wfd_attrs[idx].valid_type)
		{
			case MM_ATTRS_VALID_TYPE_INT_RANGE:
			{
				mmf_attrs_set_valid_type (attrs, idx, MM_ATTRS_VALID_TYPE_INT_RANGE);
				mmf_attrs_set_valid_range (attrs, idx,
						wfd_attrs[idx].value_min,
						wfd_attrs[idx].value_max,
                        (int)wfd_attrs[idx].default_value);
			}
			break;

			case MM_ATTRS_VALID_TYPE_INT_ARRAY:
			case MM_ATTRS_VALID_TYPE_DOUBLE_ARRAY:
			case MM_ATTRS_VALID_TYPE_DOUBLE_RANGE:
			default:
			break;
		}
	}

	debug_fleave();

	return attrs;

ERROR:
	_mmwfd_deconstruct_attribute(handle);

    return (MMHandleType)NULL;
}

void
_mmwfd_deconstruct_attribute(MMHandleType handle) // @
{
	debug_fenter();

	mm_wfd_t* wfd = (mm_wfd_t*)handle;

	return_if_fail( wfd );

	if (wfd->attrs)
	{
		mmf_attrs_free (wfd->attrs);
		wfd->attrs = 0;
	}

	debug_fleave();
}


int
_mmwfd_get_attribute(MMHandleType handle,  char **err_attr_name, const char *attribute_name, va_list args_list)
{
	int result = MM_ERROR_NONE;
	MMHandleType attrs = 0;

	debug_fenter();

	/* NOTE : Don't need to check err_attr_name because it can be set NULL */
	/* if it's not want to know it. */
	return_val_if_fail(attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);
	return_val_if_fail(handle, MM_ERROR_COMMON_INVALID_ARGUMENT);

	attrs = MMWFD_GET_ATTRS(handle);

	return_val_if_fail(attrs, MM_ERROR_COMMON_INVALID_ARGUMENT);

	result = mm_attrs_get_valist(attrs, err_attr_name, attribute_name, args_list);

	if ( result != MM_ERROR_NONE)
		debug_error("failed to get %s attribute\n", attribute_name);

	debug_fleave();

	return result;
}

int
_mmwfd_set_attribute(MMHandleType handle,  char **err_attr_name, const char *attribute_name, va_list args_list)
{
	int result = MM_ERROR_NONE;
	MMHandleType attrs = 0;

	debug_fenter();

	/* NOTE : Don't need to check err_attr_name because it can be set NULL */
	/* if it's not want to know it. */
	return_val_if_fail(attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);
	return_val_if_fail(handle, MM_ERROR_COMMON_INVALID_ARGUMENT);

	attrs = MMWFD_GET_ATTRS(handle);

	return_val_if_fail(attrs, MM_ERROR_COMMON_INVALID_ARGUMENT);

	/* set attributes and commit them */
	result = mm_attrs_set_valist(attrs, err_attr_name, attribute_name, args_list);

	if ( result != MM_ERROR_NONE)
	{
		debug_error("failed to set %s attribute\n", attribute_name);
		return result;
	}

	__mmwfd_apply_attribute(handle, attribute_name);

	debug_fleave();

	return result;
}


static gboolean
__mmwfd_apply_attribute(MMHandleType handle, const char *attribute_name)
{
	MMHandleType attrs = 0;
	mm_wfd_t* wfd = 0;

	debug_fenter();

	return_val_if_fail(handle, MM_ERROR_COMMON_INVALID_ARGUMENT);
	return_val_if_fail(attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);

	attrs = MMWFD_GET_ATTRS(handle);;

	return_val_if_fail(attrs, MM_ERROR_COMMON_INVALID_ARGUMENT);

	wfd = (mm_wfd_t*)handle;

	// TODO: This function is not useful at this moment

	debug_fleave();

	return TRUE;
}

int
_mmwfd_get_attributes_info(MMHandleType handle,  const char *attribute_name, MMWfdAttrsInfo *dst_info)
{
	int result = MM_ERROR_NONE;
	MMHandleType attrs = 0;
	MMAttrsInfo src_info = {0, };

	debug_fenter();

	return_val_if_fail(attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);
	return_val_if_fail(dst_info, MM_ERROR_COMMON_INVALID_ARGUMENT);
	return_val_if_fail(handle, MM_ERROR_COMMON_INVALID_ARGUMENT);

	attrs = MMWFD_GET_ATTRS(handle);

	return_val_if_fail(attrs, MM_ERROR_COMMON_INVALID_ARGUMENT);

	result = mm_attrs_get_info_by_name(attrs, attribute_name, &src_info);

	if ( result != MM_ERROR_NONE)
	{
		debug_error("failed to get attribute info\n");
		return result;
	}

	memset(dst_info, 0x00, sizeof(MMWfdAttrsInfo));

	dst_info->type = src_info.type;
	dst_info->flag = src_info.flag;
	dst_info->validity_type= src_info.validity_type;

	switch(src_info.validity_type)
	{
		case MM_ATTRS_VALID_TYPE_INT_ARRAY:
			dst_info->int_array.array = src_info.int_array.array;
			dst_info->int_array.count = src_info.int_array.count;
			dst_info->int_array.d_val = src_info.int_array.dval;
		break;

		case MM_ATTRS_VALID_TYPE_INT_RANGE:
			dst_info->int_range.min = src_info.int_range.min;
			dst_info->int_range.max = src_info.int_range.max;
			dst_info->int_range.d_val = src_info.int_range.dval;
		break;

		case MM_ATTRS_VALID_TYPE_DOUBLE_ARRAY:
			dst_info->double_array.array = src_info.double_array.array;
			dst_info->double_array.count = src_info.double_array.count;
			dst_info->double_array.d_val = src_info.double_array.dval;
		break;

		case MM_ATTRS_VALID_TYPE_DOUBLE_RANGE:
			dst_info->double_range.min = src_info.double_range.min;
			dst_info->double_range.max = src_info.double_range.max;
			dst_info->double_range.d_val = src_info.double_range.dval;
		break;

		default:
		break;
	}

	debug_fleave();

	return result;
}


