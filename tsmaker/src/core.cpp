/* core.cpp
 * Copyright (C) 2023 Frederick Valdez
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

#include <iostream>
#include <boost/filesystem.hpp>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "core.h"
#include "process_file.h"
#include "url.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

namespace fs = boost::filesystem; /* define namespace boost::filesystem */
using namespace std;

extern struct options opt;
extern struct mem_obj mem;

bool c = true;

 /* define struct */
struct home_dir p;          
/* end struct */

const char *extract_path(const char *p)
{
	const char *filename = NULL;
    const char *lastSlash = strrchr(p, '/');
    
	if (lastSlash != NULL) {
		filename = strdup(lastSlash + 1);
	}	
  return filename;
}

void msg_err(const char *arg, int code)
{
	cout << TERM_COLOR_DEFAULT << "\n";
	cout << TERM_COLOR_DEFAULT  << "ERROR: " << arg << " >> [Return code: " << code << "]" <<TERM_COLOR_DEFAULT << "\n";
	exit(RETURN_CODE_SUCESS);
}
void msg_print(std::string arg)
{
  cout << TERM_COLOR_GREEN << arg << TERM_COLOR_DEFAULT << "\n";
}

void run_process(std::string dir_path, std::string name, const char *c, const char *desc)
{
  chdir(dir_path.c_str());
  msg_print("Autoloading ... " + name);
  char *const run [] = { "sh", "-c", (char*)c,  NULL};
  pid_process("/bin/sh", run, desc);
}

void found_file(const std::string &path)
{    
	std::string make = path + "/Makefile";
	std::string cmake = path + "/CMakeLists.txt";
	std::string configure = path + "/configure";
	std::string autogen = path + "/autogen.sh";
	
    if(!fs::exists(make) && 
       !fs::exists(cmake) && 
       !fs::exists(configure) &&
       !fs::exists(autogen))
         {
             msg_err("No files found MakeFile, CMakeLists.txt, configure, autogen.sh,"
                     " to fix the error put one of these files in the directory", RETURN_CODE_FAILED);   
		 }	 
}

void initalize_gcc(const char *g)
{
   strcpy(mem.value, "gcc ");
   strcat(mem.value, g);
   std::cout << "Options => " << g << "\n";
   char *const sh [] = {"sh", "-c", mem.value, NULL};
   pid_process("/bin/sh", sh, "Compile for GCC");
}

void initalize_gxx(const char *g)
{
   strcpy(mem.value, "g++ ");
   strcat(mem.value, g);
    std::cout << "Options => " << g << "\n";
   char *const sh [] = {"sh", "-c", mem.value, NULL};
   pid_process("/bin/sh", sh, "Compile for CXX");
   
}

int start(const std::string &path)
{
	/* define fs and directory path */
	fs::path rootPath(path);	
	chdir(path.c_str());
    buffer_open(path + "/deps");
    
    if(mem.gcc != NULL)
		{
		  initalize_gcc(mem.gcc);				  
		}		
    else if(mem.gxx != NULL)
	    {
		  initalize_gxx(mem.gxx);	
		}
	
	if (fs::exists(rootPath) && fs::is_directory(rootPath)) {
        fs::directory_iterator end_itr;
         /* directory entry */
        for (fs::directory_iterator itr(rootPath); itr != end_itr; ++itr) {
            if (fs::is_regular_file(itr->status())) {
				found_file(path);
				const char *value = extract_path(itr->path().string().c_str()); /* pointer in a value */				
                if(strcmp(value, "Makefile") == 0)
                   {
					  if(mem.make != NULL)
					      {
							  strcpy(mem.value, "make ");
							  strcat(mem.value, mem.make);
							  std::cout << "Options => " << mem.make << "\n";
							  run_process(path, path + "/Makefile", mem.value, "GNU MAKE");
						  }
						  else
						    {
					          run_process(path, path + "/Makefile", "make", "GNU MAKE");
				            }
				      break;
				   }
				else if(strcmp(value, "CMakeLists.txt") == 0)
				 {
					      if(mem.prefix != NULL)
					         {
							  if(c == true)
								{
							     strcpy(mem.value, "cmake -DCMAKE_INSTALL_PREFIX=");
							     strcat(mem.value, mem.prefix);
							     strcat(mem.value, " .");
							     std::cout << "InstallPath => " << mem.prefix << "\n";
					             run_process(path, path + "/CMakeLists.txt", mem.value, "CMAKE");         
								 run_process(path, path + "/Makefile", "make", "GNU MAKE");
					            }	
					         }				            
					       else if(mem.cmake != NULL)
					        {
								if(c == true)
								{
								 strcpy(mem.value, "cmake ");
							     strcat(mem.value, mem.cmake); 
							     std::cout << "Options => " << mem.cmake << "\n"; 
							     run_process(path, path + "/CMakeLists.txt", mem.value, "CMAKE");					            
								 run_process(path, path + "/Makefile", "make", "GNU MAKE");	    
							    }
							} 
							else
							{ 
							    run_process(path, path + "/CMakeLists.txt", "cmake ./", "CMAKE"); 
                                run_process(path, path + "/Makefile", "make", "GNU MAKE");
						    }
				        break;    
			    }
				else if(strcmp(value, "configure") == 0)
				{
					  std::string configure = path + "/autogen.sh";
					  if(!fs::exists(configure) &&  
					     !fs::exists(path + "/CMakeLists.txt"))
					    {
							if(mem.configure != NULL)
							   {
								 strcpy(mem.value, "./configure ");
							     strcat(mem.value, mem.configure); 
							     std::cout << "Options => " << mem.configure << "\n"; 
							     run_process(path, path + "/configure", mem.value, "Configuration File");
							     run_process(path, path + "/Makefile", "make", "GNU MAKE");
							   }
							   else {
							   run_process(path, path + "/configure", "./configure", "Configuration File");
							   run_process(path, path + "/Makefile", "make", "GNU MAKE");
						   }
						}
				  break; 
				}
				else if(strcmp(value, "autogen.sh") == 0)
				{
					std::string autogen = p.cwd + "/configure";
					if(fs::exists(autogen))
					    {
							run_process(path, path + "/autogen.sh", "./autogen.sh", "Autogenerate File");
							run_process(path, path + "/configure", "./configure", "Configuration File");
							run_process(path, path + "/Makefile", "make", "GNU MAKE");
						}
						else {
							run_process(path, path + "/autogen.sh", "./autogen.sh", "Autogenerate File");
							run_process(path, path + "/configure", "./configure", "Configuration File");
							run_process(path, path + "/Makefile", "make", "GNU MAKE");
				}	
				break;	
			}
        }
    }
  }
   
 if(mem.prefix != NULL)
   {
     free(mem.prefix);
     mem.prefix = NULL;
   }
else if(mem.gcc != NULL)
   {
     free(mem.gcc);
     mem.gcc = NULL;
   }
else if(mem.gxx != NULL)
   {
    free(mem.gxx);
    mem.gxx = NULL;
   }
else if(mem.configure != NULL)
   {
    free(mem.configure);
    mem.configure = NULL;
   }
else if(mem.cmake != NULL)
   {
    free(mem.cmake);
    mem.cmake = NULL;
   }
else if(mem.make != NULL)
    {
     free(mem.make);
     mem.make = NULL;
    }
else if(mem.pkg != NULL)
    {
       free(mem.pkg);
       mem.pkg = NULL;
    }
 return 0;
}
