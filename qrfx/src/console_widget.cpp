/* console_widget.cpp
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
 
#include "console_widget.h"
#include <QtCore/QDebug>
#include <QScrollBar>

 Console::Console(QWidget *parent)
      : QPlainTextEdit(parent)
  {   
      document()->setMaximumBlockCount(300);
      QPalette p = palette();
      p.setColor(QPalette::Text, Qt::green);
      setPalette(p);

  }

 void Console::putData(const QString &text)
   {
       insertPlainText(QString(text));
       QScrollBar *bar = verticalScrollBar();
       bar->setValue(bar->maximum());
   }
