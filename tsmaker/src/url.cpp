/* url.cpp
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

#include <chrono>
#include "curl/curl.h"
#include "curl/easy.h"
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <uv.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <string>
#include "url.h"
#include "core.h"

#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>

#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace indicators;

uv_loop_t *loop;
CURLM *curl_handle;
uv_timer_t timeout;
CURL *handle = curl_easy_init();

typedef struct curl_context_s {
  uv_poll_t poll_handle;
  curl_socket_t sockfd;
} curl_context_t;

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
   {
     size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
     return written;
   }

curl_context_t *create_curl_context(curl_socket_t sockfd)
{
  curl_context_t *context;
  context = (curl_context_t *) malloc(sizeof(*context));
  context->sockfd = sockfd;
  uv_poll_init_socket(loop, &context->poll_handle, sockfd);
  context->poll_handle.data = context;
  return context;
}

int download_progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    indicators::ProgressBar* progress_bar = static_cast<indicators::ProgressBar*>(clientp);
            if (progress_bar->is_completed())
                 {
                    ;
                 }
            else if (dltotal == 0)
            {
                progress_bar->set_progress(0);
            }
            else
            {
              int percentage =
              static_cast<float>(dlnow) / static_cast<float>(dltotal) * 100;
              progress_bar->set_progress(percentage);
            }
         
    return 0;
}

void curl_close_cb(uv_handle_t *handle)
{
  curl_context_t *context = (curl_context_t *) handle->data;
  free(context);
}

void destroy_curl_context(curl_context_t *context)
{
  uv_close((uv_handle_t *) &context->poll_handle, curl_close_cb);
}
bool add_download(const std::string &url)
{
  FILE *fp;
  CURLcode res;
  const char *filename = extract_path(url.c_str());
  std::string ext = filename;

  fp = fopen(filename, "wb");
  if(!fp) {
    exit(1);
  }
  
// Hide cursor
  indicators::show_console_cursor(false);
  
 indicators::ProgressBar bar{
    indicators::option::BarWidth{32},
    indicators::option::Start{" ["},
    indicators::option::Fill{"="},
    indicators::option::Lead{">>"},
    indicators::option::Remainder{" "},
    indicators::option::End{" ]"},
    indicators::option::ShowPercentage{true}, 
    indicators::option::ShowElapsedTime{true},
    indicators::option::ShowRemainingTime{true},  
    indicators::option::ForegroundColor{indicators::Color::blue},
    indicators::option::FontStyles{
        std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
};
  
  curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(handle, CURLOPT_PRIVATE, fp);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, download_progress_callback);
  curl_easy_setopt(handle, CURLOPT_XFERINFODATA, 
                     static_cast<void*>(&bar));
  curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
  curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, true);
  curl_multi_add_handle(curl_handle, handle);

  res = curl_easy_perform(handle);
  if(res == CURLE_OK)
      {	
		 fclose(fp);
		 if(ext.substr(ext.find_last_of(".") + 1) == "zip")
          {
			  std::cout << "\033[0m\n";
			  desscompress(filename); 
		  }
      }
      
  // Show cursor
  indicators::show_console_cursor(true);
  
}

static void check_multi_info(void)
{
  char *done_url;
  CURLMsg *message;
  int pending;
  CURL *easy_handle;

  while((message = curl_multi_info_read(curl_handle, &pending))) {
    switch(message->msg) {
    case CURLMSG_DONE:
      easy_handle = message->easy_handle;
      curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &done_url);
      curl_multi_remove_handle(curl_handle, easy_handle);
      curl_easy_cleanup(easy_handle);
      curl_global_cleanup();
      break;

    default:
      fprintf(stderr, "CURLMSG default\n");
      break;
    }
  }
}

static void curl_perform(uv_poll_t *req, int status, int events)
{
  int running_handles;
  int flags = 0;
  curl_context_t *context;

  if(events & UV_READABLE)
    flags |= CURL_CSELECT_IN;
  if(events & UV_WRITABLE)
    flags |= CURL_CSELECT_OUT;
  context = (curl_context_t *) req->data;
  curl_multi_socket_action(curl_handle, context->sockfd, flags,
                           &running_handles);
  check_multi_info();
}

void on_timeout(uv_timer_t *req)
{
  int running_handles;
  curl_multi_socket_action(curl_handle, CURL_SOCKET_TIMEOUT, 0,
                           &running_handles);
  check_multi_info();
}

int start_timeout(CURLM *multi, long timeout_ms, void *userp)
{
  if(timeout_ms < 0) {
    uv_timer_stop(&timeout);
  }
  else {
    if(timeout_ms == 0)
      timeout_ms = 1;
      
    uv_timer_start(&timeout, on_timeout, timeout_ms, 0);
  }
  return 0;
}

int handle_socket(CURL *easy, curl_socket_t s, int action, void *userp,
                  void *socketp)
{
  curl_context_t *curl_context;
  int events = 0;
  switch(action) {
  case CURL_POLL_IN:
  case CURL_POLL_OUT:
  case CURL_POLL_INOUT:
    curl_context = socketp ?
      (curl_context_t *) socketp : create_curl_context(s);

    curl_multi_assign(&curl_handle, s, (void *) curl_context);

    if(action != CURL_POLL_IN)
      events |= UV_WRITABLE;
    if(action != CURL_POLL_OUT)
      events |= UV_READABLE;

    uv_poll_start(&curl_context->poll_handle, events, curl_perform);
    break;
  case CURL_POLL_REMOVE:
    if(socketp) {
      uv_poll_stop(&((curl_context_t*)socketp)->poll_handle);
      destroy_curl_context((curl_context_t*) socketp);
      curl_multi_assign(&curl_handle, s, NULL);
    }
    break;
  default:
    abort();
  }

  return 0;
}

int libcurl_initsocket()
{
  loop = uv_default_loop();
  if(curl_global_init(CURL_GLOBAL_ALL)) {
    fprintf(stderr, "Could not init curl\n");
    return 1;
  }

  uv_timer_init(loop, &timeout);

  curl_handle = curl_multi_init();
  curl_multi_setopt(curl_handle, CURLMOPT_SOCKETFUNCTION, handle_socket);
  curl_multi_setopt(curl_handle, CURLMOPT_TIMERFUNCTION, start_timeout);
  
  uv_run(loop, UV_RUN_DEFAULT);
  curl_multi_cleanup(curl_handle);
  curl_global_cleanup();
  uv_loop_close(loop); 
  return 0;
}
