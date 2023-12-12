# term.py
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

from blessings import Terminal
from datetime import datetime

term = Terminal()

now = datetime.now()
currient_time = now.strftime(" %H:%M:%S")

def term_color(c, arg):
    if c == "red":
       st = term.bright_red + arg + term.bright_white
       return st
       
def term_colorpos(c, arg, arg1):
     if c == "green":
        st = arg + term.bright_green + arg1 + term.bright_white
        return st
     elif c == "yellow":
          st = arg + term.bright_yellow + arg1 + term.bright_white
          return st
     elif c == "red":
          st = arg + term.bright_red + arg1 + term.bright_white
          return st
          
def term_debug(c):
    print("[ %s ] %s" % (currient_time, c))
              
