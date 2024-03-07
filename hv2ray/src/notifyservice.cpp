/* notifyservice.cpp
 * Copyright © 2023, 2022
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 * Authored by: Frederick Valdez
 */ 

#include <iostream>
#include <libnotify/notify.h>
#include "qplaintext.h"
#include <unistd.h>
#include <stdlib.h>

void notify_send(const char *app_name, const char *text, const char *icon)
{
  system("notify-send hello");

  notify_init("HV2ray");
  NotifyNotification* n = notify_notification_new(app_name,
                                                    text,
                                                    icon);
   notify_notification_set_timeout(n, 5000);
  if(!notify_notification_show(n,0))
    {
    }
}
