/* vi: set sw=4 ts=4 wrap ai: */
/*
 * xl-json.c: This file is part of ____
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

#include <string.h>
       #include <stdlib.h>
#include <json.h>
#include <bits.h>
#include <json_tokener.h>
#include "xl-json.h"
#include "smemory.h"
#include "logger.h"
#include "xl-url.h"

struct _VideoItem
{
	char *gcid;	/* maybe empty. */
	char *url_hash;
	//gcidlist; /* maybe not exist. */
	char *vod_info;
	char *cid; /* maybe empty. */
	char *url; /* maybe not exist. */
	char *file_name;
	char *userid;
	long ordertime;
	char *datafrom;
	int platform;
	char *src_url;
	size_t file_size;
	int64_t duration;
	char *playtime;
	int playflag;
	char *createtime;
};

char *json_parse_bt_hash(const char* json_str)
{
	/*
	 * json_str is {"result":"0","ret":"0","infohash":"004F50950256E66F128D528D0773FDEFBC298CCE"}
	 * return "bt://${infohash}"
	 */
	char *bt_hash= NULL;
	struct json_object *jsobj;
	struct json_object *jo_ret;
	struct json_object *jo_infohash;

	if (!json_str)
		return NULL;

	jsobj = json_tokener_parse(json_str);
	if( is_error(jsobj) || !jsobj)
	{
		json_object_put(jsobj);
		return NULL;
	}
	jo_ret = json_object_object_get(jsobj, "ret");
	if (jo_ret)
	{
		if (atoi(json_object_get_string(jo_ret)) != 0)
		{
			json_object_put(jo_ret);
			goto failed;
		}
	}
	jo_infohash = json_object_object_get(jsobj, "infohash"); 
	if (jo_infohash)
	{
		s_asprintf(&bt_hash, "bt://%s", json_object_get_string(jo_infohash));
		json_object_put(jo_infohash);
	}
failed:
	json_object_put(jsobj);
	return bt_hash;
}

int json_parse_bt_index(const char* json_str)
{
	/*
	 * json_str is {"resp": {"userid": "111111111", "ret": 0, "subfile_list": [{"index": 0, "url_hash": "10582384012816867477", "name": "aaa58256146@\u7fa4\u9b54\u8272\u821e@(AVopen)\u611b\u7530\u53cb,\u84bc\u4e95,\u7a57\u82b1,\u5c0f\u6fa4\u746a\u5229\u4e9e,\u9ebb\u7f8e,\u9752\u6728~\u4f86\u81eaS1\u7684\u885d\u64ca.rmvb", "cid": null, "gcid": null, "file_size": 0, "duration": 0}], "main_task_url_hash": "9918101846291549545", "info_hash": "004F50950256E66F128D528D0773FDEFBC298CCE", "record_num": 1}}
	 * return "${index}"
	 */
	int index = -1;
	struct json_object *jsobj;
	struct json_object *jo_resp;
	struct json_object *jo_ret;
	struct json_object *jo_subfile_list;
	struct json_object *jo_subfile_list_0;
	struct json_object *jo_index;

	if (!json_str)
		return -1;

	jsobj = json_tokener_parse(json_str);
	if( is_error(jsobj) || !jsobj)
	{
		json_object_put(jsobj);
		return -1;
	}
	jo_resp = json_object_object_get(jsobj, "resp");
	if (jo_resp)
	{
		jo_ret = json_object_object_get(jo_resp, "ret");
		if (jo_ret)
		{
			int ret = json_object_get_int(jo_ret);
			if (ret != 0)
			{
				json_object_put(jo_ret);
				goto failed;
			}
			json_object_put(jo_ret);
		}
		jo_subfile_list = json_object_object_get(jo_resp, "subfile_list"); 
		if (jo_subfile_list)
		{
			if (json_object_array_length(jo_subfile_list) >= 1)
			{
				jo_subfile_list_0 = json_object_array_get_idx(jo_subfile_list, 0);
				if (jo_subfile_list_0)
				{
					jo_index = json_object_object_get(jo_subfile_list_0, "index"); 
					if (jo_index)
					{
						index = json_object_get_int(jo_index);
						json_object_put(jo_index);
					}
					json_object_put(jo_subfile_list_0);
				}
			}
			json_object_put(jo_subfile_list);
		}
		json_object_put(jo_resp);
	}
failed:
	json_object_put(jsobj);
	return index;
}

int json_parse_get_return_code(const char* json_str)
{
	/*
	 * json_str is {"resp": {"kkk": "vvv", "ret": 0, "kkkk": ...}}
	 * return "${ret}"
	 */
	int ret = -1;
	struct json_object *jsobj;
	struct json_object *jo_resp;
	struct json_object *jo_ret;

	if (!json_str)
		return -1;

	jsobj = json_tokener_parse(json_str);
	if(is_error(jsobj) || !jsobj)
	{
		json_object_put(jsobj);
		return -1;
	}

	jo_resp = json_object_object_get(jsobj, "resp");
	if (jo_resp)
	{
		jo_ret = json_object_object_get(jo_resp, "ret");
		if (jo_ret)
		{
			ret = json_object_get_int(jo_ret);
			json_object_put(jo_ret);
		}
		json_object_put(jo_resp);
	}
	json_object_put(jsobj);
	return ret;
}

int json_parse_has_url(const char *json_str, const char *url)
{
	/*
	 *
	 *
	 * {"resp": {
	 *   "history_play_list":
	 *   [
	 *     {
	 *       "ip": "222.128.181.139",
	 *       "gcid": "A74C828D94C8E419D0238C168780C97C30AD6F15",
	 *       "url_hash": "7330933771486410590",
	 *       "res_list": null,
	 *       "from": "vlist",
	 *       "vod_info": null,
	 *       "cid": "DECE9E4F67AA199E3D7135757AD686AF35228F9D",
	 *       "url": "thunder://QUFmdHA6Ly9keWdvZDE6ZHlnb2QxQGQwNzAuZHlnb2Qub3JnOjEwOTAvJTVCJUU5JTk4JUIzJUU1JTg1JTg5JUU3JTk0JUI1JUU1JUJEJUIxd3d3LnlnZHk4LmNvbSU1RC4lRTUlOEYlQjYlRTklOTclQUUlRUYlQkMlOUElRTclQkIlODglRTYlOUUlODElRTQlQjglODAlRTYlODglOTguQkQuNzIwcC4lRTUlOUIlQkQlRTclQjIlQTQlRTUlOEYlOEMlRTglQUYlQUQlRTQlQjglQUQlRTUlQUQlOTcubWt2Wlo=",
	 *       "file_name": "%5B%E9%98%B3%E5%85%89%E7%94%B5%E5%BD%B1www.ygdy8.com%5D.%E5%8F%B6%E9%97%AE%EF%BC%9A%E7%BB%88%E6%9E%81%E4%B8%80%E6%88%98.BD.720p.%E5%9B%BD%E7%B2%A4%E5%8F%8C%E8%AF%AD%E4%B8%AD%E5%AD%97.mkv",
	 *       "userid": "288543553",
	 *       "ordertime": 1376044486,
	 *       "file_info": null,
	 *       "datafrom": "req_history_play_list",
	 *       "platform": 0,
	 *       "src_url": "thunder%3A//QUFmdHA6Ly9keWdvZDE6ZHlnb2QxQGQwNzAuZHlnb2Qub3JnOjEwOTAvJTVCJUU5JTk4JUIzJUU1JTg1JTg5JUU3JTk0JUI1JUU1JUJEJUIxd3d3LnlnZHk4LmNvbSU1RC4lRTUlOEYlQjYlRTklOTclQUUlRUYlQkMlOUElRTclQkIlODglRTYlOUUlODElRTQlQjglODAlRTYlODglOTguQkQuNzIwcC4lRTUlOUIlQkQlRTclQjIlQTQlRTUlOEYlOEMlRTglQUYlQUQlRTQlQjglQUQlRTUlQUQlOTcubWt2Wlo%3D",
	 *       "file_size": 1261414195,
	 *       "duration": 6013120,
	 *       "playtime": "2013-08-09 19:33:14",
	 *       "playflag": 4,
	 *       "createtime": "2013-08-09 18:34:46"
	 *     },
	 *     {
	 *       "ip": "222.128.181.139",
	 *       "gcid": null,
	 *       "url_hash": "7651461091677318816",
	 *       "res_list": null,
	 *       "from": "vlist",
	 *       "vod_info": null,
	 *       "cid": null,
	 *       "url": "thunder0X1.02728097026C8P-8720.0000000.000000QUFodHRwOi8vdGh1bmRlci5mZmR5LmNjLzk2NU MwQTk5NERDQUE1MzQ4REQwMTA4N0NDRDY1MzY0OEVFQjREM0Yv5Lit5Zu95ZCI5LyZ5Lq6QkQucm12Ylpa",
	 *       "file_name": "%E4%B8%AD%E5%9B%BD%E5%90%88%E4%BC%99%E4%BA%BABD.rmvb",
	 *       "userid": "288543553",
	 *       "ordertime": 1376040983,
	 *       "file_info": null,
	 *       "datafrom": "req_history_play_list",
	 *       "platform": 0,
	 *       "src_url": "thunder0X1.02728097026C8P-8720.0000000.000000QUFodHRwOi8vdGh1bmRlci5mZmR5LmNjLzk2NU%20MwQTk5NERDQUE1MzQ4REQwMTA4N0NDRDY1MzY0OEVFQjREM0Yv5Lit5Zu95ZCI5LyZ5Lq6QkQucm12Ylpa",
	 *       "file_size": null,
	 *       "duration": 0,
	 *       "playtime": "2013-08-09 17:36:23",
	 *       "playflag": 0,
	 *       "createtime": "2013-08-09 17:36:23"
	 *     }
	 *   ],
	 *   "max_num": 1500,
	 *   "userid": "288543553",
	 *   "ret": 0,
	 *   "end_t": null,
	 *   "record_num": 2,
	 *   "start_t": null,
	 *   "type": "all"}
	 * }
	 *
	 */
	int ret = -1;
	struct json_object *jsobj;
	struct json_object *jo_resp;
	struct json_object *jo_history_play_list;
	struct json_object *jo_history_play_list_n;
	struct json_object *jo_src_url;

	if (!json_str || !url)
		return -1;

	jsobj = json_tokener_parse(json_str);
	if(is_error(jsobj) || !jsobj)
	{
		json_object_put(jsobj);
		return -1;
	}

	jo_resp = json_object_object_get(jsobj, "resp"); 
	if (jo_resp)
	{
		jo_history_play_list = json_object_object_get(jo_resp, "history_play_list"); 
		if (jo_history_play_list)
		{
			int i;
			for (i = 0; i < json_object_array_length(jo_history_play_list); i++)
			{
				jo_history_play_list_n = json_object_array_get_idx(jo_history_play_list, i);
				if (jo_history_play_list_n)
				{
					jo_src_url = json_object_object_get(jo_history_play_list_n, "src_url"); 
					if (jo_src_url)
					{
						const char *src_url = json_object_get_string(jo_src_url);
						char *uri = xl_url_unquote(src_url);
						if (strcmp(uri, url) == 0)
						{
							ret = 0;
						}
						s_free(uri);
						json_object_put(jo_src_url);
					}
					json_object_put(jo_history_play_list_n);
				}
			}
			json_object_put(jo_history_play_list);
		}
		json_object_put(jo_resp);
	}
	json_object_put(jsobj);
	return ret;
}

char *json_parse_get_name(const char *json_str)
{
	/*
	 *
	 * json_str is {"resp": {"res": [{"url": "thunder%3A%2F%2FQUFodHRwOi...12Ylpa", "result": 0, "id": 0, "name": "\u4e2d\u56fd\u5408\u4f19\u4ebaBD.rmvb"}], "ret": 0}}
	 * return resp->res->0->${name}
	 *
	 */
	char *name = NULL;
	struct json_object *jsobj;
	struct json_object *jo_resp;
	struct json_object *jo_res;
	struct json_object *jo_res_n;
	struct json_object *jo_name;

	if (!json_str)
		return name;

	jsobj = json_tokener_parse(json_str);
	if( is_error(jsobj) || !jsobj)
	{
		json_object_put(jsobj);
		return name;
	}
	jo_resp = json_object_object_get(jsobj, "resp");
	if (jo_resp)
	{
		jo_res = json_object_object_get(jo_resp, "res"); 
		if (jo_res)
		{
			jo_res_n = json_object_array_get_idx(jo_res, 0);
			if (jo_res_n)
			{
				jo_name = json_object_object_get(jo_res_n, "name"); 
				if (jo_name)
				{
					if (name != NULL)
						s_free(name);
					name = strdup(json_object_get_string(jo_name));
					json_object_put(jo_name);
				}
				json_object_put(jo_res_n);
			}
			json_object_put(jo_res);
		}
		json_object_put(jo_resp);
	}
	json_object_put(jsobj);
	return name;
}

char *json_parse_get_download_url(const char *json_str, VideoType type)
{
	/*
	 *
	 * XL_CLOUD_FX_INSTANCEqueryBack({
     * "resp": {
     *     "status": 0,
     *     "url_hash": "288301201412044132",
     *     "trans_wait": -1,
     *     "userid": "288543553",
     *     "ret": 0,
     *     "src_info": {
     *         "file_name": "梦幻天堂·龙网(killman.net).720p.大话西游之大圣娶亲",
     *         "cid": "",
     *         "file_size": "0",
     *         "gcid": "820EA640D99A37E0AC301CC8BA01E00302622889"
     *     },
     *     "vodinfo_list": [{
     *         "vod_urls": [],
     *         "spec_id": 225536,
     *         "vod_url": "http://124.95.174.190/download?dt=16&g=E8DCDA6F78065905114B0E619B0F334834ADA576&t=2&ui=288543553&s=471222714&v_type=-1&scn=c13&it=1376504026&p=0&cc=9042386459858147283&n=0A4A0F9076D805696CBF854C89DE006574FAC61AD5C01E2EE5774FC5486D86A5BF3550950349E5E5A4740DB14415C6B6E4695A2DE7F06E0000",
     *         "has_subtitle": 0
     *     }, {
     *         "vod_urls": [],
     *         "spec_id": 282880,
     *         "vod_url": "http://124.95.174.190/download?dt=16&g=FFD69470DFE198EC443BB51D054995F74F646A71&t=2&ui=288543553&s=790926882&v_type=-1&scn=c13&it=1376504026&p=0&cc=16893651737588826745&n=0A4A0F9076D805696CBF854C89DE006574FAC61AD5C01E2EE5774FC5486D86A5BF3550950349E5E5A4740DB14415C6B6E4695A2DE7F06E0000",
     *         "has_subtitle": 0
     *     }, {
     *         "vod_urls": [],
     *         "spec_id": 356608,
     *         "vod_url": "http://124.95.174.190/download?dt=16&g=5BB401588FA5978DB9EA90DC205922ADFAF9EFE5&t=2&ui=288543553&s=1273308635&v_type=-1&scn=c13&it=1376504026&p=0&cc=117367614423376147&n=0A4A0F9076D805696CBF854C89DE006574FAC61AD5C01E2EE5774FC5486D86A5BF3550950349E5E5A4740DB14415C6B6E4695A2DE7F06E0000",
     *         "has_subtitle": 0
     *     }],
     *     "duration": 5984172000,
     *     "vod_permit": {
     *         "msg": "OK",
     *         "ret": 0
     *     },
     *     "error_msg": ""
     * }
	 * })
	 */

	int spec_id;
	struct json_object *jsobj;
	struct json_object *jo_resp;
	struct json_object *jo_vodinfo_list;
	struct json_object *jo_vodinfo_list_n;
	struct json_object *jo_spec_id;
	struct json_object *jo_vod_url;
	struct json_object *jo_duration;
	char *vod_url = NULL;
	int64_t duration;
	char *download_url = NULL;

	if (!json_str)
		return download_url;

	jsobj = json_tokener_parse(json_str);
	if( is_error(jsobj) || !jsobj)
	{
		json_object_put(jsobj);
		return download_url;
	}
	jo_resp = json_object_object_get(jsobj, "resp");
	if (jo_resp)
	{
		//FIXME: trans_wait -1 is not ready.
		jo_vodinfo_list = json_object_object_get(jo_resp, "vodinfo_list"); 
		if (jo_vodinfo_list)
		{
			int i;
			for (i = 0; i < json_object_array_length(jo_vodinfo_list); i++)
			{
				jo_vodinfo_list_n = json_object_array_get_idx(jo_vodinfo_list, i);
				if (jo_vodinfo_list_n)
				{
					jo_spec_id = json_object_object_get(jo_vodinfo_list_n, "spec_id"); 
					if (jo_spec_id)
					{
						spec_id = json_object_get_int(jo_spec_id);
						json_object_put(jo_spec_id);
					}
					if (type == VIDEO_480P && spec_id == 225536)
					{
						jo_vod_url = json_object_object_get(jo_vodinfo_list_n, "vod_url"); 
						if (jo_vod_url)
						{
							if (vod_url != NULL)
								s_free(vod_url);
							vod_url = s_strdup(json_object_get_string(jo_vod_url));
							json_object_put(jo_vod_url);
						}
						json_object_put(jo_vodinfo_list_n);
						break;
					} else if (type == VIDEO_720P && spec_id == 282880)
					{
						jo_vod_url = json_object_object_get(jo_vodinfo_list_n, "vod_url"); 
						if (jo_vod_url)
						{
							if (vod_url != NULL)
								s_free(vod_url);
							vod_url = s_strdup(json_object_get_string(jo_vod_url));
							json_object_put(jo_vod_url);
						}
						json_object_put(jo_vodinfo_list_n);
						break;
					} else if (type == VIDEO_1080P && spec_id == 356608)
					{
						jo_vod_url = json_object_object_get(jo_vodinfo_list_n, "vod_url"); 
						if (jo_vod_url)
						{
							if (vod_url != NULL)
								s_free(vod_url);
							vod_url = s_strdup(json_object_get_string(jo_vod_url));
							json_object_put(jo_vod_url);
						}
						json_object_put(jo_vodinfo_list_n);
						break;
					}
				}
			}
			json_object_put(jo_vodinfo_list);
		}
		jo_duration = json_object_object_get(jo_resp, "duration"); 
		if (jo_duration) {
			duration = json_object_get_int64(jo_duration)/1000/1000;
		}
		json_object_put(jo_resp);
	}
	json_object_put(jsobj);
	if (vod_url != NULL)
	{
		char *substr = strstr(vod_url, "s=");
		char num[20];
		int i =0;
		substr += 2;
		while (substr && *substr != '&')
		{
			num[i] = *substr;
			i++;
			substr++;
		}
		num[i] = '\0';
		s_asprintf(&download_url, "%s&start=0&end=%s&flash_meta=0&type=loadmetadata&du=%d", vod_url, num, duration);
		s_free(vod_url);
	}
	return download_url;
}
