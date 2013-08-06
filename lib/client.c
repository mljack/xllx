/* vi: set sw=4 ts=4 wrap ai: */
/*
 * client.c: This file is part of ____
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

#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "xllx.h"
#include "client.h"
#include "http.h"
#include "smemory.h"
#include "logger.h"
#include "url.h"
#include "info.h"

struct _XLCookies {
    char *ptvfsession;          /**< ptvfsession */
    char *ptcz;
    char *skey;
    char *ptwebqq;
    char *ptuserinfo;
    char *uin;
    char *ptisp;
    char *pt2gguin;
    char *verifysession;
    char *lwcookies;
};

struct _XLClient
{
    char *username;             /**< Username */
    char *password;             /**< Password */
	char *vcode;
    XLCookies *cookies;
};

static long get_current_timestamp(void);
static void get_verify_code(XLClient *client, long now, XLErrorCode *err);
static void get_verify_image(XLClient *client, long now);
static char *string_toupper(const char *str);
static char* encode_password(const char* password, const char* vcode);
static int re_match(const char* pattern, const char* str);

XLClient *xl_client_new(const char *username, const char *password)
{

	if (!username || !password) {
		xl_log(LOG_ERROR, "Username or password is null\n");
		return NULL;
	}

	XLClient *lc = s_malloc0(sizeof(*lc));
	lc->username = s_strdup(username);
	lc->password = s_strdup(password);

	lc->cookies = s_malloc0(sizeof(*(lc->cookies)));

	xl_log(LOG_DEBUG, "Create a new client with username:%s, password:%s successfully\n", lc->username, lc->password);
	return lc;
}

static void create_post_data(XLClient *client, char *buf, int buflen)
{
    char *s;
    char m[512];
	char* md5 =  encode_password(client->password, client->vcode);
	//char* md5 =  encode_password("puhua418", "!CEP");
    snprintf(m, sizeof(m), "{\"u\":\"%s\",\"p\":\"%s\",\"login_enable\":\"0\",\"login_hour\":\"720\",\"verifycode\":\"%s\"}",
             client->username, md5, client->vcode);
	printf("m=%s\n", m);
    s = url_encode(m);
    snprintf(buf, buflen, "%s", s);
    s_free(s);
}

void do_login(XLClient *client, const char* encpwd, XLErrorCode *err)
{
    char msg[512] ={0};
	XLHttpRequest *req;
	char url[512];

	char response[256];
	int ret;

	snprintf(url, sizeof(url), "http://login.xunlei.com/sec2login/");
	create_post_data(client, msg, sizeof(msg));
	xl_log(LOG_NOTICE, "%s\n", msg);
	req = xl_http_request_create_default(url, err);
	if (!req) {
		goto failed;
	}
	ret = xl_http_request_open(req, HTTP_POST, msg);
	if (ret != 0) {
		*err = XL_ERROR_NETWORK_ERROR;
		goto failed;
	}

	if (xl_http_request_get_status(req) != 200)
	{
		*err = XL_ERROR_HTTP_ERROR;
		goto failed;
	}


failed:
	xl_http_request_free(req);
}

void do_login2(XLClient *client, const char* encpwd, XLErrorCode *err)
{
    char msg[512] ={0};
	XLHttpRequest *req;
	char url[512];

	char response[256];
	int ret;

	xl_http_request_free(req);
	snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/login?cachetime=%ld&from=0", get_current_timestamp());
	req = xl_http_request_create_default(url, err);
	if (!req) {
		goto failed;
	}
	ret = xl_http_request_open(req, HTTP_GET, NULL);
	if (ret != 0) {
		*err = XL_ERROR_NETWORK_ERROR;
		goto failed;
	}

	if (xl_http_request_get_status(req) != 200)
	{
		*err = XL_ERROR_HTTP_ERROR;
		goto failed;
	}

	char* userid;
	userid = xl_http_request_get_cookie(req, "userid");
	printf("the user id is %s\n", userid);
	xl_log(LOG_NOTICE, "Get response userid: %s\n", userid);
failed:
	xl_http_request_free(req);
}

int xl_client_login(XLClient *client, XLErrorCode *err)
{
	long now;
    if (!client) {
        xl_log(LOG_ERROR, "Invalid pointer\n");
        return XL_ERROR_ERROR;
    }

	now = get_current_timestamp();

	if (!client->vcode) {
		get_verify_code(client, now, err);
		printf("err=%d\n", *err);
		switch (*err) {
			case XL_ERROR_LOGIN_NEED_VC:
				get_verify_image(client, now);
				xl_log(LOG_WARNING, "Need to enter verify code\n");
				return ;
			case XL_ERROR_NETWORK_ERROR:
				xl_log(LOG_ERROR, "Network error\n");
				return ;
			case XL_ERROR_OK:
				xl_log(LOG_DEBUG, "Get verify code OK\n");
				break;
			default:
				xl_log(LOG_ERROR, "Unknown error\n");
				return ;
		}
	}
    
    /* Third: calculate the md5 */
    char *md5;// = lwqq_enc_pwd(client->password, client->vcode);

    /* Last: do real login */
    do_login(client, md5, err);
    do_login2(client, md5, err);
//    s_free(md5);

    /* Free old value */
//    lwqq_vc_free(client->vc);
//    client->vc = NULL;
}

static char* encode_password(const char* password, const char* vcode)
{
	char buf[128] = {0};
	char new_buf[128] = {0};

	if (re_match("^[0-9a-f]{32}$", password) != 0)
	{
		lutil_md5_data((const unsigned char *)password, strlen(password), (char *)buf);
		lutil_md5_data((const unsigned char *)buf, strlen(buf), (char *)buf);
	} else {
		strcpy(buf, password);
	}
	char *vcode_upper = string_toupper(vcode);
	snprintf(new_buf, sizeof(new_buf), "%s%s", buf, vcode_upper);
	s_free(vcode_upper);
	lutil_md5_data((const unsigned char *)new_buf, strlen(new_buf), (char*)buf);
	return s_strdup(buf);
}

static long get_current_timestamp(void)
{
    struct timeval tv;
    long v;

    gettimeofday(&tv, NULL);
    v = tv.tv_usec;
    v = (v - v % 1000) / 1000;
    v = tv.tv_sec * 1000 + v;
    xl_log(LOG_NOTICE, "current timestamp=%ld\n", v);
	return v;
}

static void get_verify_code(XLClient *client, long now, XLErrorCode *err)
{
	XLHttpRequest *req;
	char url[512];

	char response[256];
	int ret;

	snprintf(url, sizeof(url), "http://login.xunlei.com/check?u=%s&cachetime=%ld", client->username, now);
	xl_log(LOG_NOTICE, "Request URL=%s\n", url);
	req = xl_http_request_create_default(url, err);
	if (!req) {
		goto failed;
	}
	ret = xl_http_request_open(req, HTTP_GET, NULL);
	if (ret != 0) {
		*err = XL_ERROR_NETWORK_ERROR;
		goto failed;
	}

	if (xl_http_request_get_status(req) != 200)
	{
		*err = XL_ERROR_HTTP_ERROR;
		goto failed;
	}

	char* check_result;
	check_result = xl_http_request_get_cookie(req, "check_result");
	xl_log(LOG_NOTICE, "Get response verify code: %s\n", check_result);

	if (*check_result == '0' && strlen(check_result) == 6) {
		*err = XL_ERROR_OK;
		client->vcode = s_strdup(check_result+2);
		s_free(check_result);
	} else if (*check_result == '1') {
		*err = XL_ERROR_LOGIN_NEED_VC;
	}
failed:
	xl_http_request_free(req);
}

static void get_verify_image(XLClient *client, long now)
{
	XLHttpRequest *req;
	char url[512];

    char image_file[256];
	char response[256];
	int ret;
	XLErrorCode err;

	snprintf(url, sizeof(url), "http://verify2.xunlei.com/image?cachetime=%ld", now);
	xl_log(LOG_NOTICE, "Request URL=%s\n", url);
	req = xl_http_request_create_default(url, &err);
	if (!req) {
		goto failed;
	}
	ret = xl_http_request_open(req, HTTP_GET, NULL);
	if (ret != 0) {
		goto failed;
	}

	if (xl_http_request_get_status(req) != 200)
	{
		goto failed;
	}

    int image_length = 0;
    char *content_length = xl_http_request_get_header(req, "Content-Length");
    if (content_length) {
        image_length = atoi(content_length);
        s_free(content_length);
    }

    snprintf(image_file, sizeof(image_file), "/tmp/xl_%s.jpeg", client->username);
    /* Delete old file first */
    unlink(image_file);
    int fd = creat(image_file, S_IRUSR | S_IWUSR);
    if (fd != -1) {
        ret = write(fd, xl_http_request_get_response(req), image_length);
        if (ret <= 0) {
            xl_log(LOG_ERROR, "Saving verify image file error\n");
        }
        close(fd);
    }

failed:
	xl_http_request_free(req);
}

void xl_client_set_verify_code(XLClient *client, const char *vcode)
{
	if (!client)
		return ;

	if (client->vcode != NULL)
		s_free(client->vcode);
	client->vcode = strdup(vcode);
}

void xl_client_get_verify_image(XLClient *client, const char *path)
{
    XLHttpRequest *req = NULL;  
    char url[512];
    int ret;
    char chkuin[64];
    char image_file[256];
    int image_length = 0;
    XLErrorCode err;
 
#if 0
		//verification_code_url = 'http://verify2.xunlei.com/image?cachetime=%s' % current_timestamp()
    snprintf(url, sizeof(url), LWQQ_URL_VERIFY_IMG, APPID, lc->username);
    req = lwqq_http_create_default_request(url, &err);
    if (!req) {
        goto failed;
    }
     
    snprintf(chkuin, sizeof(chkuin), "chkuin=%s", lc->username);
    req->set_header(req, "Cookie", chkuin);
    ret = req->do_request(req, 0, NULL);
    if (ret) {
        goto failed;
    }
    if (req->http_code != 200) {
        goto failed;
    }
 
    char *content_length = req->get_header(req, "Content-Length");
    if (content_length) {
        image_length = atoi(content_length);
        s_free(content_length);
    }
    update_cookies(lc->cookies, req, "verifysession", 1);
    snprintf(image_file, sizeof(image_file), "/tmp/lwqq_%s.jpeg", lc->username);
    /* Delete old file first */
    unlink(image_file);
    int fd = creat(image_file, S_IRUSR | S_IWUSR);
    if (fd != -1) {
        ret = write(fd, req->response, image_length);
        if (ret <= 0) {
            lwqq_log(LOG_ERROR, "Saving erify image file error\n");
        }
        close(fd);
    }
 
failed:
    lwqq_http_request_free(req);
#endif
}

void xl_client_free(XLClient *client)
{
	if (!client)
		return ;

	/* Free LwqqVerifyCode instance */
	s_free(client->username);
	s_free(client->password);
	s_free(client->vcode);
	//cookies_free(client->cookies);

	s_free(client);
}

static char *string_toupper(const char *str)
{
	char *newstr, *p;
	p = newstr = s_strdup(str);
	while(*p) {
		*p=toupper(*p);
		p++;
	}
	return newstr;
}

/*
 * return value:
 * error: -1
 * no match: 1
 * matched: 0
 */

static int re_match(const char* pattern, const char* str)
{
    regex_t re;            
    int err;
    int ret = -1;

    err = regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB);
    if (err)
    {
        return -1;
    }
    err = regexec(&re, str, 0, NULL, 0);
    if (err == REG_NOMATCH)
    {
         regfree(&re);
         return 1;
    }
    else if (err)
    {  
         return 1;
    }
    regfree(&re);
    return 0;
}

static void xl_tasks_with_URL(char *task_url, boolean has_next_page,TaskListType listtype)
{
	XLHttpRequest *req;
	char response[256];
	int ret;
	XLErrorCode err;
	xl_log(LOG_NOTICE, "Request URL=%s\n", url);
	req = xl_http_request_create_default(url, &err);
	if (!req) {
		goto failed;
	}
	ret = xl_http_request_open(req, HTTP_GET, NULL);
	if (ret != 0) {
		goto failed;
	}

	if (xl_http_request_get_status(req) != 200)
	{
		goto failed;
	}
	char *content_length = xl_http_request_get_header(req, "Content-Length");
	if (content_length) {
		char *response = xl_http_request_get_response(req);
		//here parse the response
	}

failed:
	xl_http_request_free(req);

}

static void xl_tasks_with_status(TaskListType listType)
{
	char url[512];
	char *userid = "288543553";
	switch (listType) {
		case TLTAll:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_task?userid=%s&st=0",userid);
			break;
		case TLTComplete:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_task?userid=%s&st=2",userid);
			break;
		case TLTDownloadding:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_task?userid=%s&st=1",userid);
			break;
		case TLTOutofDate:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_history?type=1&userid=%s",userid);
			break;
		case TLTDeleted:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_history?type=0userid=%s",userid);
			break;
		default:
			break;
	}

	xl_tasks_with_URL(url, false, listType);
}

static void xl_tasks_with_status(TaskListType listType,int pg,bool hasNextPage)
{
	char* userid = "288543553";
	char url[512];
	switch (listType) {
		case TLTAll:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_task?userid=%@&st=0&p=%ld",userid,pg);
			break;
		case TLTComplete:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_task?userid=%@&st=2&p=%ld",userid,pg);
			break;
		case TLTDownloadding:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_task?userid=%@&st=1&p=%ld",userid,pg);
			break;
		case TLTOutofDate:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_history?type=1&userid=%@&p=%ld",userid,pg);
			break;
		case TLTDeleted:
			snprintf(url, sizeof(url), "http://dynamic.cloud.vip.xunlei.com/user_history?type=0&userid=%@&p=%ld",userid,pg);
			break;
		default:
			break;
	}
	xl_tasks_with_URL(url, hasNextPage, listType);
}

void xl_read_all_tasks_with_stat(TaskListType listType){
	int pg=1;
	bool hasNP=false;
	do {
		xl_tasks_with_status(listType, pg, &hasNP);
		pg++;
	} while (hasNP);
	return;
}

void xl_read_all_complete_tasks()
{
	xl_read_all_tasks_with_stat(TLTComplete);
}
void xl_read_complete_tasks_with_page(int pg)
{
	xl_tasks_with_status(TLTComplete, pg, NULL);
}
void xl_read_all_downloading_tasks()
{
	xl_read_all_tasks_with_stat(TLTDownloadding);
}
void xl_read_downloading_tasks_with_page(int pg)
{
	xl_tasks_with_status(TLTDownloadding, pg, NULL);
}
void xl_read_all_outofdate_tasks()
{
	xl_read_all_tasks_with_stat(TLTOutofDate);
}
void xl_read_outofdate_tasks_with_page(int pg)
{
	xl_tasks_with_status(TLTOutofDate, pg, NULL);
}
void xl_read_all_delete_tasks()
{
	xl_read_all_tasks_with_stat(TLTDeleted);
}
void xl_read_delete_tasks_with_page(int pg)
{
	xl_tasks_with_status(TLTDeleted, pg, NULL);
}

