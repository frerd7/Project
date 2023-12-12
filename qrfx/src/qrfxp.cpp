/* qrfxp.cpp
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
 
#include <QProcess>
#include <QDebug>
#include <unistd.h>

int main()
{
    QProcess process;
    process.start("/usr/bin/startx", QStringList() << "/lib/qrfx/qrfx");
    process.waitForFinished(-1);

    if(process.exitCode() != 0)
    {
        qDebug () << "Error:" << process.exitCode() << process.readAllStandardError();
    }
    else
    {
        qDebug () << "\x1b[32mLoading the graphics mode do not turn off the computer ...";
        qDebug () << process.readAll();
        system("systemctl start graphical.target");
    }

    return pause();
}
