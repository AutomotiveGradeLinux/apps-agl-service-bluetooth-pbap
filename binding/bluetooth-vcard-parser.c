/*
 * Copyright (C) 2019 Konsulko Group
 * Author: Matt Ranostay <matt.ranostay@konsulko.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <string.h>
#include <glib.h>

#include <json-c/json.h>

#define CRLF	"\r\n"
#define SUPPORTED_VERSION "3.0"

enum {
	VC_FORMATTED_NAME,
	VC_TELEPHONE,
	VC_DATETIME,
	VC_PHOTO,
	VC_FIELD_COUNT,
};

static const gchar *VC_PREFIXES[VC_FIELD_COUNT] = {
	"FN",
	"TEL",
	"X-IRMC-CALL-DATETIME",
	"PHOTO",
};

#define vc_len(name)		strlen(VC_PREFIXES[name])

#define vc_strstr(ptr, name) \
	({ \
		void *_ptr = strstr(ptr, name); \
		if (_ptr) _ptr += strlen(name); \
		_ptr; \
	 })

static void add_fn(json_object *resp, gchar *msg)
{
	gchar *name = strstr(msg, ":");

	if (!name || !strlen(name + 1))
		return;

	json_object_object_add(resp, "fn", json_object_new_string(++name));
}

static void add_phone(json_object *array, gchar *msg)
{
	gchar *ptr = strstr(msg, ":");
	json_object *dict, *number;

	if (!ptr || !strlen(ptr + 1))
		return;

	number = json_object_new_string(ptr + 1);

	ptr = vc_strstr(msg, "TYPE=");
	if (!ptr)
		ptr = "UNKNOWN";
	else
		ptr = strsep(&ptr, ",;:");

	dict = json_object_new_object();
	json_object_object_add(dict, ptr, number);
	json_object_array_add(array, dict);
}

static void add_datetime(json_object *resp, gchar *msg)
{
	gchar *ptr;

	msg += vc_len(VC_DATETIME) + 1;

	ptr = strsep(&msg, ":");
	if (!ptr)
		return;
	json_object_object_add(resp, "type", json_object_new_string(ptr));

	ptr = strsep(&msg, "");
	if (!ptr)
		return;
	json_object_object_add(resp, "timestamp", json_object_new_string(ptr));
}

static gchar **extract_photo(json_object *resp, gchar **msg)
{
	GString *data;
	gchar *ptr;
	json_object *obj;

	// TODO: handle other mimetypes
	if (!strstr(*msg, "TYPE=JPEG"))
		return msg;

	ptr = strstr(*msg, ":");
	if (!ptr)
		return msg;

	data = g_string_new(NULL);
	g_string_append(data, ptr + 1);

	if (!*msg++)
		return msg;

	for (; *msg; msg++) {
		if (!strlen(*msg))
			break;
		if (!g_ascii_isspace(**msg))
			break;
		g_string_append(data, g_strchug(*msg));
	}

	obj = json_object_new_object();
	json_object_object_add(obj, "mimetype", json_object_new_string("image/jpeg"));
	json_object_object_add(obj, "data", json_object_new_string(data->str));
	json_object_object_add(resp, "photo", obj);

	g_string_free(data, TRUE);

	return msg;
}

static int tag_index(gchar *msg)
{
	gchar *ptr;
	int i;

	for (i = 0; i < VC_FIELD_COUNT; i++) {
		if (!g_str_has_prefix(msg, VC_PREFIXES[i]))
			continue;

		ptr = vc_strstr(msg, VC_PREFIXES[i]);
		if (*ptr == ':' || *ptr == ';')
			return i;

		break;
	}

	return -EINVAL;
}

static void process_vcard(json_object *resp, gchar **start, gchar **end)
{
	json_object *vcard = json_object_new_object();
	json_object *phone = NULL;
	gboolean history = FALSE;

	for (; start != end; start++) {
		int idx = tag_index(*start);

		switch (idx) {
		case VC_FORMATTED_NAME:
			add_fn(vcard, *start);
			break;
		case VC_TELEPHONE:
			if (!phone)
				phone = json_object_new_array();
			add_phone(phone, *start);
			break;
		case VC_DATETIME:
			history = TRUE;
			add_datetime(vcard, *start);
			break;
		case VC_PHOTO:
			start = extract_photo(vcard, start);
			break;
		default:
			continue;
		}
	}

	if (phone) {
		if (!json_object_array_length(phone)) {
			// No phone numbers, so discard
			json_object_put(phone);
			json_object_put(vcard);
			return;
		}

		if (!history)
			json_object_object_add(vcard, "telephone", phone);
		else {
			json_object *number = json_object_array_get_idx(phone, 0);

			json_object_object_foreach(number, unused, val) {
				json_object_object_add(vcard, "telephone", val);
				(void) unused;
			}
		}
	}
	json_object_array_add(resp, vcard);
}

json_object *parse_vcards(const char *message)
{
	gchar **lines, **tmp, **ptr = NULL;
	json_object *resp;

	if (!message || !strlen(message))
		return NULL;

	resp = json_object_new_array();
	lines = g_strsplit(message, CRLF, -1);

	for (tmp = lines; *tmp; tmp++) {
		// BEGIN:VCARD
		if (!g_strcmp0(*tmp, "BEGIN:VCARD")) {
			ptr = tmp + 1;
			continue;
		}

		if (!ptr)
			continue;

		// VERSION should be the line after BEGIN:VCARD
		if (tmp == ptr && g_strcmp0(*tmp, "VERSION:" SUPPORTED_VERSION)) {
			ptr = NULL;
			continue;
		}

		// END:VCARD
		if (!g_strcmp0(*tmp, "END:VCARD")) {
			process_vcard(resp, ptr, tmp);
			ptr = NULL;
		}
	}

	g_strfreev(lines);

	return resp;
}
