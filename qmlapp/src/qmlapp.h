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

#ifndef QMLAPP_H
#define QMLAPP_H

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QDir>
#include <QQmlEngine>
#include <QGuiApplication>

# define BUFFER_SIZE 255

struct Options
{
    Options()
        : quitForce(false),
          debug(false)
       {
       }
    bool quitForce;
    bool debug;
};

struct mem_read {
    char buf[BUFFER_SIZE];
    char *value = (char*) malloc(255);
    QStringList list;
    QString home = QDir::currentPath();
    QString str;
};

struct cache {
    char *app = NULL;
    char *version = NULL;
    char *icon = NULL;
    char *window = NULL;
    char *module = NULL;
    int win;
    QByteArray array;
    QUrl url;
};

 void read_info(QQmlEngine *eng);
 void conf_parse(QGuiApplication *app);
 QString dirpath(char *dir);
 void addImport(QQmlEngine *eng, QStringList *imports);
 int syntaxerror(char *err);
 int windowtype(char *type);
 QByteArray qmlshell(const char *ur);
 void options(void);
#endif // QMLAPP_H
