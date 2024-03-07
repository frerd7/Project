/* Copyright © 2021 Frederick Valdez
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

#ifndef HV2RAY_H
#define HV2RAY_H

#include <QMainWindow>
#include <QProcess>
#include <QLabel>


namespace Ui {
class HV2ray;
}

class About;
class Console;

class HV2ray : public QMainWindow
{
    Q_OBJECT

public:
    explicit HV2ray(QWidget *parent = 0);
    ~HV2ray();
    void start();
    void filed();
    void aboutd();
    void proxy_apt();

private slots:
       void readData();

private:
    Ui::HV2ray *ui;
    Console *console;
    QProcess *process;
    About *about;
    QLabel *status;
};

QString cddir(QString s);

#endif // HV2RAY_H
