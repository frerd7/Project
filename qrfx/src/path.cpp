/* path.cpp
 *
 * Copyright © 2023,2022
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
 * Authored by: Frederick Valdez
 */ 
 
#include <QDir>

#include "qrfx.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void QRFX::clearUsersSettings(const QString &f2)
{
    QString st = "/home";
    QString tr;
    QDir dir(st);
    dir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);

    if(!dir.exists())
    {
         console->putData("Not Exists Dir");
         exit(1);
    }

    foreach(QString item, dir.entryList())
       {
          QDir subdir(dir.absoluteFilePath(item));
              subdir.setFilter(QDir::Hidden | QDir::Dirs);
              foreach(QString it __attribute_used__, subdir.entryList())
                        {
                            tr.sprintf("%s/%s/%s", qPrintable(st), qPrintable(item), qPrintable(f2));

                        }

       }

  QDir subDir(tr);
  if(!subDir.exists())
  {
     console->putData("Not Exists Dir");
       exit(1);
  }

  subDir.removeRecursively();

}


void QRFX::clearFile(const QString &f)
{
    QFile file(f);
    if(!file.remove())
    {
        console->putData("[Error]: Cannot be removed\n");
    }
    else
    {
       console->putData("[OK]: Finished\n");
    }
}

void QRFX::clearDir(const QString &str)
{
    QDir dir(str);
    dir.setFilter(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    foreach(QString item, dir.entryList())
    {
        QDir subDir(dir.absoluteFilePath(item));
        if(!dir.remove(item) && !subDir.removeRecursively())
        {
           console_msg(0, str, item);
        }
        else
        {
           console_msg(1, str, item);
        }


    }
}

 void QRFX::console_msg(int n, const QString &msg, const QString &item)
{
   int i = 1;

   if (i == n)
   {
       QString std;
       std.sprintf("Erasing: [OK] %s/%s\n", qPrintable(msg), qPrintable(item));
       console->putData(std);
   }
   else
   {
       QString st;
       st.sprintf("Erasing: [Error] %s/%s  Cannot be erased\n", qPrintable(msg), qPrintable(item));
       console->putData(st);

   }

}

