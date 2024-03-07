/* main.cpp
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
 
#include <QApplication>
#include <QLibrary>
#include <QDebug>

#include "qrfx.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLibrary *qr = new QLibrary("/lib/qrfx/libqrc.so");
      typedef void (*Resource)();
      Resource rc = (Resource) qr->resolve("_Z7rc_initv");
      if (!rc)
      {
          qDebug() << qr->errorString();
          delete qr;
          return 0;
       }

    rc();
    system("/bin/plymouth quit");
    QRFX w;
    w.show();

    return a.exec();
}
