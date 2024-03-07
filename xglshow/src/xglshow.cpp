/* (07)
 * Copyright © 2020 Frederick Valdez
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
 */

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include "xglshow.h"
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>

GLuint texture[200];
GLFWwindow* window;

static struct {
    unsigned int width=0;
    unsigned int height=0;
    unsigned int bytes_per_pixel=3;
    unsigned char *pixel_data=nullptr;
} image;

GLuint LoadIMGFile(const char *filename)
{
         GLuint texture;
            GError *error = nullptr;
            auto img = gdk_pixbuf_new_from_file(filename, &error);
                 if (img) 
                    {
                      image.width = gdk_pixbuf_get_width(img);
                      image.height = gdk_pixbuf_get_height(img);
                      image.bytes_per_pixel = gdk_pixbuf_get_has_alpha(img) ? 4 : 3;
                      image.pixel_data= (unsigned char*)gdk_pixbuf_read_pixels(img);
                     } else {
                      printf("Could not load wallpaper: %s\n", error->message);
                     }

           g_clear_error(&error);
           glGenTextures(1,&texture);
           glBindTexture(GL_TEXTURE_2D,texture);
           glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,
     GL_MODULATE);

                 switch(image.bytes_per_pixel)
                         {
                           case 3:
                               gluBuild2DMipmaps(GL_TEXTURE_2D,image.bytes_per_pixel, image.width, image.height,GL_RGB, GL_UNSIGNED_BYTE, image.pixel_data); 
                           break;

                           case 4:
                               gluBuild2DMipmaps(GL_TEXTURE_2D,image.bytes_per_pixel, image.width, image.height,GL_RGBA, GL_UNSIGNED_BYTE, image.pixel_data); 
                           break;
                         default:
                           printf("Could not Select Format Color (RGBA) or (RGB)\n");
                           exit(EXIT_FAILURE);
                        }

          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
     free(image.pixel_data);

     return texture;
}



void DisplayScene()
{

glfwGetFramebufferSize(window,&width,&height);
ration= width / (float) height;
glViewport(0,0,width,height);
glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glMatrixMode (GL_PROJECTION);
glLoadIdentity ();
glOrtho(-10,10,-10,10,-10,10);
glMatrixMode (GL_MODELVIEW);

frame+=1.3;
    if (frame > num)
    {
    frame = 1;
    }

   for(int d=0; d<num; d++)
        {
         texture[d-1] = d+0;
        }

glLoadIdentity();
glEnable(GL_TEXTURE_2D);
glBindTexture(GL_TEXTURE_2D, texture[(int)frame]);
glTranslatef(0,0,-5);

glBegin(GL_QUADS);
glTexCoord2d(0,0); 
glVertex3f(-10,10,0);
glTexCoord2d(1,0);
glVertex3f(10,10,0);
glTexCoord2d(1,1);
glVertex3f(10,-10,0);
glTexCoord2d(0,1);
glVertex3f(-10,-10,0);
glEnd();

glFlush(); 
glPopMatrix();

}


void LoadDir()
{
 struct dirent **files;
 const char *dirname = "/usr/share/xglshow/theme";
 file=scandir(dirname, &files, NULL, alphasort);
    if (file < 0)
          {
           fprintf(stderr,"Cannot open %s (%s)\n", dirname, strerror(errno));
           glfwTerminate();
           exit(EXIT_FAILURE);
	}

     for (i=0; i < file; i++)
           {
            matframe[i] = i+0;
            num = matframe[i];

         if(strcmp(".", files[i]->d_name) != 0 && strcmp("..", files[i]->d_name) != 0) {
            strcpy(nombre,dirname);
            strcat(nombre,"/");
            strcat(nombre,files[i]->d_name);
            texture[i] = LoadIMGFile(nombre);
            }
         }
}

static volatile sig_atomic_t run = 0;

static void shutdown(int signum)
{
    if (run)
    {
        run = 0;
        printf("Signal %d received. Good night.\n", signum);
       
    }
}

bool state_app()
{
    return run;
}

int main(int argc, const char *argv[])
try
{
    if(!glfwInit())
        {
            fprintf(stderr, "Failed to initialize GLFW\n");
            return -1;
        }
     
        run = 1;
        signal(SIGINT, shutdown);
        signal(SIGTERM, shutdown);

       
        GLFWmonitor *monitors = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitors);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        window = glfwCreateWindow(mode->width, mode->height, "XGLShow", monitors, NULL);

        if(window == NULL){
                fprintf(stderr, "Failed to open GLFW window.\n");
                glfwTerminate();
                return -1;
            }

            glfwMakeContextCurrent(window);
            LoadDir();  
               
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

             while(state_app())
                {
                 DisplayScene();
                 glfwSwapBuffers(window);
                 glfwPollEvents();
                }

    glfwTerminate();
 return EXIT_SUCCESS;

} catch (std::exception const& x) {
    printf("%s\n", x.what());
    return EXIT_FAILURE;
}
