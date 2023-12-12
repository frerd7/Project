/* process_file.cpp
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

#include <stdio.h>
#include <string.h>
#include <iostream>
#include "pid.h"
#include "process_file.h"
#include "url.h"
#include "core.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

struct options opt;
struct mem_obj mem;

void remove_p(char *str, char c) {
    int readIndex = 0;
    int writeIndex = 0;

    while (str[readIndex] != '\0') {
        if (str[readIndex] != c) {
             str[writeIndex] = str[readIndex];
             writeIndex++;
        }
        readIndex++;
    }

    str[writeIndex] = '\0';
}

char* memory_cache(char *obj, char *val, size_t len)
{
	size_t Size = 0;
	obj = (char*) realloc(obj, Size + len + 1);
	if (obj == NULL) {
            perror("Error allocating memory");
        }
        
    strcpy(obj + Size, val);
    Size += len;  
    return obj;  
}

int open_read(FILE *file)
{
    mem.value = (char*) malloc (255); 
	while (fgets(mem.buffer, sizeof(mem.buffer), file) != NULL) {
		size_t len = strlen(mem.buffer);
		
		if (len > 0 && mem.buffer[len - 1] == '\n') {
			mem.buffer[len - 1] = '\0';
		}	
		
		for(int i = 0; i < len; i++)
		  {
			  if(mem.buffer[i] == '%')
			     {
					 remove_p(&mem.buffer[i], '%');	 
					 if(mem.buffer[i] == 'o')
					    {
							if(strcmp(&mem.buffer[i], "options:") == 1 && 
							   strcmp(&mem.buffer[i], "opt:") == 1);
							  {
								  mem.value = strchr(&mem.buffer[i], ' ');
								  remove_p(mem.value, '-');
								  remove_p(mem.value, ' ');
								  if(strcmp(mem.value, "auto_install") == 0)
								     {
										 std::cout << "\nAutomatic installation after compiling\n";
										 opt.install = false;
									 }
							  }
					    }
					    else if (mem.buffer[i] == 'i')
					    {
							if(strcmp(&mem.buffer[i], "install:") == 1)
							   {
								  mem.value = strchr(&mem.buffer[i], ' ');
								  size_t lent = strlen(mem.value);
								  mem.pkg = memory_cache(mem.pkg, mem.value, lent); 
						       }
						   break;
					    }
					     else if (mem.buffer[i] == 'u')
					     {
						    if(strcmp(&mem.buffer[i++], "url:") == 1)
							   {
								  libcurl_initsocket();	
								  mem.value = strchr(mem.buffer, ' ');
								  remove_p(mem.value, ' ');
								  const char *found = strstr(mem.value, "://");
								  if (found != NULL)
								    {
										std::string new_url = mem.value;
										std::cout << "Downloading File: " << new_url << "\n";
										add_download(new_url);
							       }
							       else
									   {
										   std::cout << "\nInvalid URL on line: " << mem.value;
									  }
						       }
						   break;
						 }
						 else if(mem.buffer[i] == 'p')
						 {
								if(strcmp(&mem.buffer[i], "prefix:") == 1)
							   {
						     mem.value = strchr(&mem.buffer[i], ' ');
						     remove_p(mem.value, ' ');
						     size_t lent = strlen(mem.value);
						     mem.prefix = memory_cache(mem.prefix, mem.value, lent); 
                }
					    }	
					     else if(mem.buffer[i] == 'm')
						 {
                if(strcmp(&mem.buffer[i], "make:") == 1)
							   {
						     mem.value = strchr(&mem.buffer[i], ' ');
						     remove_p(mem.value, ' ');
						     size_t lent = strlen(mem.value);
						     mem.make = memory_cache(mem.make, mem.value, lent); 
                 }
					     }
					     else if(mem.buffer[i] == 'c')
						 {
								if(strcmp(&mem.buffer[i], "cmake:") == 1)
							   {
						       mem.value = strchr(&mem.buffer[i], ' ');
						       remove_p(mem.value, ' ');
						       size_t lent = strlen(mem.value);
						       mem.cmake = memory_cache(mem.cmake, mem.value, lent); 
                  }
					     }
					     else if(mem.buffer[i] == 's')
						 {
                 if(strcmp(&mem.buffer[i], "configure:") == 1)
							   {
						     mem.value = strchr(&mem.buffer[i], ' ');
						     remove_p(mem.value, ' ');
						     size_t lent = strlen(mem.value);
						     mem.configure = memory_cache(mem.configure, mem.value, lent); 
                 }
					     }
					     else if(mem.buffer[i] == 'g')
						 {
							 if(strcmp(&mem.buffer[i], "gcc") == 1)
							   {
						        mem.value = strchr(&mem.buffer[i], ' ');
						        remove_p(mem.value, ' ');
						        size_t lent = strlen(mem.value);
						        mem.gcc = memory_cache(mem.gcc, mem.value, lent);
						    } 
						    else if(strcmp(&mem.buffer[i], "g++") == 1)
						    {
								mem.value = strchr(&mem.buffer[i], ' ');
						        remove_p(mem.value, ' ');
						        size_t lent = strlen(mem.value);
						        mem.gxx = memory_cache(mem.gxx, mem.value, lent);
							}
					     }
					     			    				 				  
					 break;
			     }
			    else
			      {
					  std::cout << "ERROR: A % character is missing";
					  break;
					  exit(-1);
				  }
		  }
  }
  
  mem.value[0] = '\0';
  mem.buffer[0] = '\0';
}

void pkg_dep(const char *p)
{
	  sprintf(mem.value, "sudo apt-get install %s", p);
	  std::cout << "Dependencies required to build the project => " << p << "\n";	
	  char *const apt [] = {"sudo","sh", "-c", mem.value, NULL};
      pid_process("/usr/bin/sudo", apt, "Package Apt");
}

int buffer_open(const std::string &e)
{
   FILE *file = fopen(e.c_str(), "r");
	if (file == NULL) {
		return 1;
		}
		
   open_read(file);
   
   if (mem.pkg != NULL)
     {
       pkg_dep(mem.pkg);
     }
     
   fclose(file);
   return 0;
}
