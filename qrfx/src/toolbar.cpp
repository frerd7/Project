/* toolbar.cpp
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

#include <QLabel>
#include <QPushButton>
#include <QLabel>
#include <QStyle>

#include "qrfx.h"
#include "info.h"

void QRFX::setup_statusbar(QWidget *widget, int width)
{
    label = new QLabel(widget);
    label->setText("Recovery Menu");
    label->setGeometry(QRect(0, 0, 130, 30));
    label->setAlignment(Qt::AlignLeft);
    label->setStyleSheet("color: white;");

    font.setPointSize(12);
    label->setFont(font);

    gridlayout(widget, width-120);
}

void QRFX::gridlayout(QWidget *widget, int width)
{
    QWidget *gridLayoutWidget = new QWidget(widget);
    gridLayoutWidget->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignRight, QSize(121, 31), QRect(width, -2, 121, 31)));
    QGridLayout *gridLayout = new QGridLayout(gridLayoutWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    act =  new QPushButton(gridLayoutWidget);
    icon.addFile(QStringLiteral(":/icon-poweroff.png"), QSize(), QIcon::Normal, QIcon::Off);
    act->setIcon(icon);
    act->setIconSize(QSize(28, 22));
    act->setFlat(true);
    gridLayout->addWidget(act, 0, 2, 1, 1);

    act1 = new QPushButton(gridLayoutWidget);
    icon1.addFile(QStringLiteral(":/icon-info.png"), QSize(), QIcon::Normal, QIcon::Off);
    act1->setIcon(icon1);
    act1->setIconSize(QSize(28, 22));
    act1->setFlat(true);
    gridLayout->addWidget(act1, 0, 1, 1, 1);

     act2 = new QPushButton(gridLayoutWidget);
     icon2.addFile(QStringLiteral(":/network-error.png"), QSize(), QIcon::Normal, QIcon::Off);

     act2->setIcon(icon2);
     act2->setIconSize(QSize(28, 22));
     act2->setCheckable(true);
     act2->setFlat(true);
     gridLayout->addWidget(act2, 0, 0, 1, 1);

}

void QRFX::aboutf()
{
    console->putData("Starting About Dialog ...\n");
    about = new info(this);
    about->setAttribute(Qt::WA_DeleteOnClose);
    about->show();

}

