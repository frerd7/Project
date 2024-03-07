#!/usr/bin/python3

# host-info.sh
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


import argparse
import os
import sys
from method import *

home = os.path.expanduser('~')
           
if __name__ == "__main__":
	parser = argparse.ArgumentParser(description="Shows detailed information of a Web host.")
	parser.add_argument("-f", "--file", default=False, help="Read a file with Host to Process.")
	parser.add_argument("-p", "--ping", default=False, help="Test the Ping of Web Host, use log or term Options.")
	parser.add_argument("-d", "--dns", default=False, help="Shows information of the (DNS) Domain Name System, use log or term Options.")
	args = parser.parse_args()
	sys.tracebacklimit=0
	
	if args.file == False:
	    raise Exception("Error: a file has not been selected,  Please Selected a file using --file $")
	print(term_color("red", "[ Generating log info host file: " + home + "/htmldoc" + "/log.txt" + " ]"))
	              
	if os.path.exists(home + "/htmldoc") == False:
           os.mkdir(home + "/htmldoc")
	elif os.path.exists(home + "/htmldoc/log.txt") == True: 
             os.remove(home + "/htmldoc/log.txt")  
             
	method_read_line(args.file)        
	if args.dns == "term":
	     print(term_color("red", "[ " + "Printing Domain Name System (DNS) Info ..." + " ]"))
	     dns_info(args.file, 0)
	elif args.dns == "log":
	     print(term_color("red", "[ " + "Generated Log Domain Name System (DNS) Info ..." + " ]"))	    
	     dns_info(args.file, 1)
	elif args.dns == False:
	     print(term_color("red", "[ " + "Disabled Domain Name System (DNS) Info" + " ]"))		
	if args.ping == "term":
	    print(term_color("red","[ " + "Printing Enabled Ping Testing for 10 seconds..." + " ]"))
	    test_ping(args.file, 0)
	elif args.ping == False:
	    print(term_color("red","[ " + "Disabled Ping Testing" + " ]"))
	elif args.ping == "log":
	    print(term_color("red","[ " + "Generated Log Ping Testing for 10 seconds..." + " ]"))  
	    test_ping(args.file, 1)
