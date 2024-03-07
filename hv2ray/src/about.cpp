/* about.cpp
 * Copyright © 2023 Frederick Valdez
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
 
#include "about.h"
#include "ui_about.h"

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);
    ui->label_3->setText("1.0");
    ui->label_4->setText("Allows you to be a client of the");
    ui->label_5->setText("V2ray project using Java Script.");
    ui->label_6->setText("Writing in Qt Core Framework.");
   quitbutton();
}

void About::quitbutton()
{
    QObject::connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(close()));
}
