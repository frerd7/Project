/* url.h
 * Copyright (C) 2023  Frederick Valdez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef URL_H
#define URL_H

#include "curl/curl.h"
#include "curl/easy.h"
#include <iostream>
#include <uv.h>

size_t write_data(void *ptr, 
                  size_t size, 
                  size_t nmemb,
                  void *stream);
/* write data */
void curl_close_cb(uv_handle_t *handle);
bool add_download(const std::string &url);
static void check_multi_info(void);
static void curl_perform(uv_poll_t *req, int status, int events);
void on_timeout(uv_timer_t *req);
int start_timeout(CURLM *multi, long timeout_ms, void *userp);
int handle_socket(CURL *easy, curl_socket_t s, int action, void *userp,
                  void *socketp);
int libcurl_initsocket();
int desscompress(const char *input);
#endif // URL_H
