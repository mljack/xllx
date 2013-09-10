/* vi: set sw=4 ts=4 wrap ai: */
/*
 * xl-json.h: This file is part of ____
 *
 * Copyright (C) 2013 yetist <xiaotian.wu@i-soft.com.cn>
 *
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#ifndef __XL_JSON_H__ 
#define __XL_JSON_H__  1
#include "xl-vod.h"
#include "xl-videos.h"

char *json_parse_bt_hash(const char* json_str);
int json_parse_bt_count(const char* json_str);
int json_parse_bt_index(const char* json_str);
int json_parse_bt_index_by_index(const char* json_str, int idx);
char *json_parse_bt_name_by_index(const char* json_str, int idx);
int json_parse_get_return_code(const char* json_str);
//int json_parse_has_url(const char *json_str, const char *url);
int json_parse_list_videos(const char *json_str, XLVideos *videos);
//int json_parse_has_url(const char *json_str, const char *url, char **url_hash);
int json_parse_get_name_and_url(const char *json_str, char **name, char **url);
char *json_parse_get_download_url(const char *json_str, VideoType type);
//char *json_parse_get_url_hash(const char* json_str, const char *url);
VideoStatus json_parse_get_video_status(const char *json_str);

#endif /* __XL_JSON_H__ */
