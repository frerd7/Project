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

#include <QDir>
#include <QDebug>
#include "qmlapp.h"

#define Debug qDebug()
#define Info qWarning

void remove_char(char *pt, const char *pt1)
{
   strtok(pt, pt1);
}

QString dirpath(char *dir)
{
   struct mem_read st;
   st.str = QString::fromLocal8Bit(dir);
   if(dir[0] == '~' || dir[0] == '.')
   {
        st.str.replace(0, 2, QDir::currentPath() + "/");
   }
    return st.str;
}

void addImport(QQmlEngine *eng, QStringList *imports)
{
    for (int i = 0; i < imports->size(); ++i)
          eng->addImportPath(imports->at(i));

    for (int i = 0; i < imports->size(); ++i)
          eng->addPluginPath(imports->at(i));
}

int syntaxerror(char *err)
{
  Info("[ ERROR ]: (%s) Invalid Syntax", err);
  exit(-1);

return false;
}

int windowtype(char *type)
{
    int value = 0;
    if(strcmp(&type[0], "engine") == 0)
    {
        value = 1;
    }
    else if(strcmp(&type[0], "host") == 0)
    {
        value = 2;
    }
    else
        {
            Debug << "Valid Window Manager is host and engine";
            syntaxerror(&type[0]);
        }
return value;
}
