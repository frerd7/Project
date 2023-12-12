/* (07)
 * Copyright © 2022-2023.
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
#include "qmlapp.h"
#include <QGuiApplication>
#include <QQmlEngine>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>

/* C class */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Debug qDebug()
#define Info qWarning

extern Options opt;

struct mem_read mem;
struct cache var;

char* parseAndCache(const char* line, char delimiter) {
    const char* valueStart = strchr(line, delimiter);

    if (valueStart != NULL) {
        ++valueStart;
        size_t valueLen = strlen(valueStart);
        char* cachedValue = (char*)malloc(valueLen + 1);
        if (cachedValue != NULL) {
            strcpy(cachedValue, valueStart);
            return cachedValue;
        } else {
            fprintf(stderr, "Memory allocation error\n");
            exit(EXIT_FAILURE);
        }
    }
    fprintf(stderr, "Delimiter not found in the line: %s\n", line);
    exit(EXIT_FAILURE);
}

void read_info(QQmlEngine *eng)
{
  FILE *fp;
  sprintf(mem.value, "%s/qml.txt", qPrintable(mem.home));
  fp = fopen(mem.value, "r");

 if (fp) {
   while(fgets(mem.buf, sizeof(mem.buf), fp) != NULL)
   {
       size_t len = strlen(mem.buf);
       if (len > 0 && mem.buf[len-1] == '\n')
       {
           mem.buf[len-1] = '\0';
       }

       for (int i = 0; i < len; i++)
       {
          switch (mem.buf[i])
          {
            case 'a':
                if (strncmp(&mem.buf[i], "appname:", 8) == 0) {
                    var.app = parseAndCache(&mem.buf[i], ' ');
                }
            break;
            case 'v':
                if (strncmp(&mem.buf[i], "version:", 8) == 0) {
                    var.version = parseAndCache(&mem.buf[i], ' ');
                }
            break;
            case 'i':
                if (strncmp(&mem.buf[i], "image:", 6) == 0) {
                    var.icon = parseAndCache(&mem.buf[i], ' ');
                }
            break;
            case 'w':
                if (strncmp(&mem.buf[i], "window:", 7) == 0) {
                    var.window = parseAndCache(&mem.buf[i], ' ');
                }
            break;
            case 'm':
                if (strncmp(&mem.buf[i++], "module:", 7) == 0) {
                    var.module = parseAndCache(&mem.buf[i++], ' ');
                    mem.list.append(dirpath(var.module));
                    addImport(eng, &mem.list);
                    printf("Module: %s\n", var.module);
                }
            break;
          }
       }
    }
   fclose(fp);
  }
}

void conf_parse(QGuiApplication *app)
{
    // conf parse info
  if (var.app != NULL)
    {
      app->setApplicationName(var.app);
      if(opt.debug)
        qDebug() << "AppName:" << var.app;
    }

    if (var.version != NULL)
    {
      app->setApplicationVersion(var.version);
      if(opt.debug)
       qDebug() << "Version:" << var.version;
    }

    if (var.icon != NULL)
    {
      if(opt.debug)
       qDebug() << "Icon:" << var.icon;
    }

    if (var.window != NULL)
    {
      var.win = windowtype(var.window);
    }
}

/* qml to parse script */
QByteArray qmlshell(const char *ur)
{
    QFile file(ur);
    QByteArray result;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (!line.startsWith("#"))
            {
                result.append(line + "\n");
            }
        }
    file.close();
    } else {
        exit(0);
    }
    return result;
}
