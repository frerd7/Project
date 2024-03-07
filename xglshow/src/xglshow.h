/* (07)
 * Copyright © 2020 Frederick Valdez.
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

#ifndef XGL_SHOW_H
#define XGL_SHOW_H

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

char nombre[1025];
int file;
double frame;
int i;
void DisplayScene();
void LoadDir();
float ration;
int width,height;
int matframe[100];
int num;

#endif // XGL_SHOW_H
