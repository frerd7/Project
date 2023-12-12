/* pid.cpp
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

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <iostream>
#include "core.h"

using namespace std;

int pid_process(const char *exec, char *const arg[], const char *note)
{
 pid_t pid;
 pid = fork();
 int status;
 if(pid == -1)
 {
    cout << "Process failed\n";
 }
 else if (pid == 0)
 {
	cout << TERM_COLOR_RED << "Running " << note << " PID [" << getpid() << "]" << TERM_COLOR_DEFAULT << "\n";
    execvp(exec, arg);
 }
 else {
      waitpid(pid, &status, 0);
      if(WIFEXITED(status))
      {
		cout << TERM_COLOR_RED << "<< Process Successfully Terminate >> [Return code: " << status <<"]\n" << TERM_COLOR_DEFAULT;
      }
      else if(WIFSIGNALED(status))
        {
          if (WTERMSIG(status) == SIGTERM)
              {
                cout << TERM_COLOR_DEFAULT << "SIgnal Terminate Process signal >> "<< WTERMSIG(status) << "\n";
              }
              if (WTERMSIG(status) == SIGKILL)
              {
                cout << TERM_COLOR_DEFAULT << "SIgnal Kill Process signal >> " << WTERMSIG(status) << "\n";
              }
        }
 }
}
