/* (07)
 * Copyright © 2020-2021.
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
 * Created by Frederick Valdez.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#ifndef PR_SET_CHILD_SUBREAPER
#define PR_SET_CHILD_SUBREAPER 36
#endif

int main(int argc, char** argv)
{
pid_t pid, wtr;
int status;
char *comand = "/usr/bin/xglshow";
char *list [] = {"/usr/bin/xglshow", NULL, NULL};


 if(prctl(PR_SET_CHILD_SUBREAPER, 1) < 0) {
        perror("could not set subreaper process attribute");
        exit(EXIT_FAILURE);
    }

    if((pid = fork()) < 0)
        {
         exit(1);
         }

        else if(pid == 0)
        {
         execvp(comand,list);
         perror("could not exec");
         exit(EXIT_FAILURE);
         }
         else
           {
                 int seg = 18;
                 printf("Shutdown in %d seg.\n",seg);
                 sleep(seg);
                 kill(pid,SIGTERM);

           wtr = waitpid(pid, &status, WUNTRACED | WCONTINUED);
            if(wtr == -1)
                 {
                  perror("waitpid");
                  exit(1);
                  }

                 if(WIFEXITED(status))
                    {
                      printf("exited, status=%d\n", WEXITSTATUS(status));
                    }

                else if(WIFSIGNALED(status))
                  {
                   printf("killed by signal %d\n",WTERMSIG(status));
                   }
            }

exit(EXIT_SUCCESS);        
}


