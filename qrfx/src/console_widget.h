/* console_widget.h
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
 
#ifndef QPLAINTEXT_H
#define QPLAINTEXT_H

#include <QPlainTextEdit>

  class Console : public QPlainTextEdit
  {
      Q_OBJECT

     public:
      explicit Console(QWidget *parent = nullptr);
      void putData(const QString &text);
 };

  #endif // QPLAINTEXT_H
