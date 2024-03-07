/* qrfx.h
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
 
#ifndef QRFX_H
#define QRFX_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QProcess>

#include "console_widget.h"
#include "info.h"

class Console;
class info;
class QRFX : public QMainWindow
{
    Q_OBJECT

public:
   QRFX(QWidget *parent = 0);
    ~QRFX();

    void setup_statusbar(QWidget *widget, int width);
    void gridlayout(QWidget *widget, int width);
    void stylesshet(QWidget *cwidget, QWidget *widget1,int w, int h);
    void actions_panel(QVBoxLayout *layout2);
    void console_widget(QWidget *widget, int width, int height);
    void slots_init();
    void process(const QString &p, const QStringList arguments);
    void clearDir(const QString &str);
    void console_msg(int n, const QString &msg, const QString &item);
    void clearFile(const QString &f);
    void clearUsersSettings(const QString &f2);

private slots:
    void action_init();
    void action1_init();
    void action2_init();
    void action4_init();
    void action5_init();
    void action3_init();
    void act_init();
    void putdata();
    void aboutf();
    void network();

private:
    QScreen *screen;
    QRect  screenGeometry;
    Console *console;
    int h;
    int wi;   

    QLabel *label;
    QFont font;

    QPushButton *action;
    QPushButton *action1;
    QPushButton *action2;
    QPushButton *action3;
    QPushButton *action4;
    QPushButton *action5;
    QPushButton *act;
    QPushButton *act1;
    QPushButton *act2;

    QIcon icon;
    QIcon icon1;
    QIcon icon2;

    QProcess *append;
    QStringList arg;
    info *about;
};

#endif // QRFX_H
