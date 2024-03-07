# method.py
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

import http.client 
import subprocess
from term import *
from header import *
   
def test_ping(f, p):
       file = open(f, "r+")
       for lines in file:
           lines = lines[:-1]
           output = subprocess.run(['ping',"-c","4", lines],  stdout=subprocess.PIPE,encoding='utf-8')
           lines = "%s%s%s" % (term.blue, output.stdout, term.bright_white)
           if p == 1:
              file_debug_header(home, output.stdout)
           else:
              term_debug(lines)
                                     
def dns_info(f, d):
        file = open(f, "r+")
        for lines in file:
            lines = lines[:-1]
            output = subprocess.run(['dig', lines], stdout=subprocess.PIPE, encoding='utf-8')
            lines = "%s%s%s" % (term.blue, output.stdout, term.bright_white)
            if d == 1:
               file_debug_header(home, output.stdout)
            else:
               term_debug(lines)            

def method_connect(f):  
      host = http.client.HTTPSConnection(f)
      host.request("GET", "/")
      stat = host.getresponse()
      if stat.status == 200:
           term_debug("Host: " + f)
           term_debug(term_colorpos("green", "Response: ", "[ 200 OK ]"))
           file_debug_header(home, "Host: " + f + "\n")
           buf = "Headers: %s\n" % stat.headers 
           file_debug_header(home, buf)
           generated_html(home, f)
           security_protocol(f)
      elif stat.status == 301:
           term_debug("Host: " + f)
           term_debug(term_colorpos("yellow","Response: ","[ 301 Moved Permanetly ]"))
           file_debug_header(home, "Host: " + f + "\n")
           buf = "Headers: %s\n" % stat.headers 
           file_debug_header(home, buf)
           security_protocol(f)
      elif stat.status == 302:
           term_debug("Host: " + f)
           term_debug(term_colorpos("yellow", "Response: ","[ 302 Found ]"))
           buf = "Headers: %s\n" % stat.headers
           file_debug_header(home, buf)
           security_protocol(f)
      elif stat.status == 307:
           term_debug("Host: " + f)
           term_debug(term_colorpos("yellow","Response: ", "[ 307 Temporary Redirect ]"))
           file_debug_header(home, "Host: " + f + "\n")
           buf = "Headers: %s\n" % stat.headers 
           file_debug_header(home, buf)
           security_protocol(f)
      elif stat.status == 308:
           term_debug("Host: " + f)
           term_debug(term_colorpos("yellow","Response: ", "[ 308 Permanent Redirect ]"))
           file_debug_header(home, "Host: " + f + "\n")
           buf = "Headers: %s\n" % stat.headers 
           file_debug_header(home, buf)
           security_protocol(f)
      elif stat.status == 403:
           term_debug("Host: " + f)
           term_debug(term_colorpos("red","Response: ", "[ 400 Forbidden ]"))
      elif stat.status == 404:
           term_debug("Host: " + f)
           term_debug(term_colorpos("red", "Response: ","[ 404 Not Found ]")) 
      else:
           term_debug(term_colorpos("red","Response: ","[ ERROR ]"))
           
def method_read_line(l):
    file = open(l, "r+")
    for lines in file:
        lines = lines[:-1]
        method_connect(lines)         
