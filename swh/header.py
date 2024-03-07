# header.py
# Copyright (C) 2023  Frederick Valdez
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import sys
import os
import urllib.request
import socket
import ssl

from term import currient_time
from term import term_debug

home = os.path.expanduser('~')

def file_debug_header(h, d):
    file = open(h + "/htmldoc" + "/log.txt", "a")
    env = "[ %s ] %s" % ((currient_time, d))
    file.write(env)
    file.close
    
def generated_html(h, host):
    with urllib.request.urlopen("http://" + host) as f:
         html = f.read().decode('utf-8')
         file = open(h + "/htmldoc/" +  host  , "a")
         file.write(html)
         file.close

def security_protocol(f):
    context = ssl.create_default_context()
    with socket.create_connection((f, 443)) as sock:
         with context.wrap_socket(sock, server_hostname=f) as ssock:
              term_debug("Security Protocol: " + ssock.version())
              buf = "Security Protocol: %s\n" % ssock.version()
              file_debug_header(home, buf)

