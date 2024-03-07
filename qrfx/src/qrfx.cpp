/* qrfx.cpp
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
 
#include <QPushButton>
#include <QWidget>
#include <QLabel>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QtWidgets/QTableView>

#include "qrfx.h"
#include "console_widget.h"


void QRFX::stylesshet(QWidget *cwidget, QWidget *widget1,int w, int h)
{
    widget1->setGeometry(QRect(0, 0, w, 24));
    widget1->setStyleSheet("background-color: black;");
    cwidget->setStyleSheet("background-color: #2c001e;");
    cwidget->setMinimumSize(w, h);
    cwidget ->setMaximumSize(w, h);

}

void QRFX::console_widget(QWidget *widget, int width, int height)
{
    console = new Console(widget);
    int result = width + height;

    if(result == 2304)
       {
          console->setGeometry(QRect(0, height-300, width-2, 235));
       }
       else if(result == 2134)
       {
          console->setGeometry(QRect(0, 570, width-2, 190));
       }

}


QRFX::QRFX(QWidget *parent)
    : QMainWindow(parent)
{
        QWidget *window = new QWidget;
        QWidget *wid = new QWidget(window);
         screen = QGuiApplication::primaryScreen();
         screenGeometry = screen->geometry();
         h = screenGeometry.height();
         wi = screenGeometry.width();

        QVBoxLayout *layout = new QVBoxLayout;
        QVBoxLayout *layout2 = new QVBoxLayout;

        actions_panel(layout2);
        setup_statusbar(wid, wi);
        stylesshet(window, wid, wi, h);
        console_widget(window, wi, h);
        slots_init();

        layout->addLayout(layout2);
        layout->setAlignment(layout2, Qt::AlignCenter);
        window->setLayout(layout);
        setCentralWidget(window);
}

QRFX::~QRFX()
{

}
