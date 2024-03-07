/* hv2ray.cpp
 * Copyright © 2021 Frederick Valdez
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
 
#include "hv2ray.h"
#include "ui_hv2ray.h"
#include "qplaintext.h"
#include <QScrollBar>
#include <QPlainTextEdit>
#include <QProcess>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFile>
#include <QLabel>
#include <QTextStream> 
#include "about.h"

HV2ray::HV2ray(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HV2ray)
{
    ui->setupUi(this);
    console = new Console;
    console->setEnabled(false);
    setCentralWidget(console);

    about = new About(this);
    process = new QProcess(this);
    status = new QLabel;
    ui->statusBar->addWidget(status);

    QDir dir = QDir::homePath();
    if (!dir.exists("v2ray"))
    {
       ui->start->setEnabled(false);
    }

    connect(ui->start, &QAction::triggered, this, &HV2ray::start);
    connect(ui->selectfile, &QAction::triggered, this, &HV2ray::filed);
    connect(ui->about, &QAction::triggered, this, &HV2ray::aboutd);
}

void HV2ray::aboutd()
{
    about = new About(this);
    about->setAttribute(Qt::WA_DeleteOnClose);
    about->show();
}

void HV2ray::filed()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the V2ray Configuration"),
                                                     "/home",
                                                     tr("Java Script (*.json)"));
     QDir dir = QDir::homePath();
        if (!dir.exists("v2ray"))
        {
              dir.mkpath("v2ray");
              qWarning("Successfully created directory v2ray.");
              QFile::copy(cddir(fileName),cddir("~/v2ray/config.json"));
              ui->start->setEnabled(true);
        }
        else {
            QFile::copy(cddir(fileName),cddir("~/v2ray/config.json"));
            qWarning("Could not create directory.");
        }

}

void HV2ray::start()
{
    QStringList arguments;
    arguments <<"-config" << cddir("~/v2ray/config.json");
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process,SIGNAL(readyReadStandardOutput()),this,SLOT(readData()));
    process->start("/usr/lib/corev2ray/v2ray",arguments);
    
    proxy_apt();

   if(process->state() == QProcess::Starting)
    {
    console->setEnabled(true);
    status->setText("http://localhost:10809");
    notify_send("HV2ray","V2ray HTTP client is ready to use with http://localhost:10809 proxy server","/usr/share/icons/hicolor/128x128/apps/icon-hv2ray.png");
    }
}

void HV2ray::readData()
  {
    QString output=process->readAllStandardOutput();
    console->putData(output);
  }


HV2ray::~HV2ray()
{
    delete ui;
}

QString cddir(QString s)
 {
     if((s == "~") || (s.startsWith("~/")))
     {
         s.replace(0, 1, QDir::homePath());
     }
     return s;
 }

void HV2ray::proxy_apt()
{

     QFile fp("/etc/apt/apt.conf.d/proxy.conf");
     fp.open(QIODevice::WriteOnly | QIODevice::Text);
     QTextStream out(&fp);
     out << "Acquire::http::Proxy \"http://localhost:10809/\";\nAcquire::https::Proxy \"http://localhost:10809/\";";
     fp.close();    
}
