/* auto_comp.cpp
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

#include "core.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <sys/resource.h>

struct home_dir entry; 
namespace fs = boost::filesystem; /* define namespace boost::filesystem */ 

int main()
{
  fs::path dir(entry.cwd + "/autoload");
  if (fs::exists(dir) && fs::is_directory(dir)) {
        fs::directory_iterator end_itr;
       if(fs::is_empty(dir))
       {
          std::cout << "Error: Is empty dir\n";
          exit(1);
       }
       
       std::cout << "Automatic Compiler (N) Objects\n";
       
        for (fs::directory_iterator itr(dir); itr != end_itr; ++itr) {
			 if(fs::is_directory(itr->path().string())) {
				 
			   if(fs::is_empty(itr->path().string()))
              {
                std::cout << "WARNING: \"" << itr->path().string() << "\" Is empty dir\n";
              }
              
             std::cout << "Loading the project >> " << itr->path().string() << "\n";
             start(itr->path().string());
		}
  }
 }
 else 
 {
	 start(entry.cwd);
 }

return 0;       
}
