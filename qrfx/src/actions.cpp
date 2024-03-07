/* actions.cpp
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
#include <QAction>
#include <QProcess>
#include <QDir>
#include <QCoreApplication>
#include <QFileDialog>

#include "qrfx.h"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


void QRFX::actions_panel(QVBoxLayout *layout2)
{
    action = new QPushButton("Next Normal Boot");
    action1 = new QPushButton("Exec Bash Script");
    action2 = new QPushButton("Fix Bootloader");
    action3 = new QPushButton("Software Update");
    action4 = new QPushButton("Run dpkg");
    action5 = new QPushButton("Clean System");

    action->setIcon(QIcon(":/icon-next.png"));
    action->setIconSize(QSize(39,39));
    action->setStyleSheet("text-align:left; color:white;");
    action->setFlat(true);

    action1->setIcon(QIcon(":/exec_script.png"));
    action1->setIconSize(QSize(41,41));
    action1->setStyleSheet("text-align:left; color:white;");
    action1->setFlat(true);

    action2->setIcon(QIcon(":/icon-updateboot.png"));
    action2->setIconSize(QSize(39,39));
    action2->setStyleSheet("text-align:left; color:white;");
    action2->setFlat(true);


    action3->setIcon(QIcon(":/system-software-update.png"));
    action3->setIconSize(QSize(39,39));
    action3->setStyleSheet("text-align:left; color:white;");
    action3->setFlat(true);


    action4->setIcon(QIcon(":/icon-package.png"));
    action4->setIconSize(QSize(39,39));
    action4->setStyleSheet("text-align:left; color:white;");
    action4->setFlat(true);

    action5->setIcon(QIcon(":/user-trash.png"));
    action5->setIconSize(QSize(39,39));
    action5->setFlat(true);
    action5->setStyleSheet("text-align:left; color:white;");

    QLabel *lab = new QLabel;
    lab->setText("Select an Options use mouse");
    lab->setStyleSheet("color: yellow");

    layout2->addWidget(lab);
    layout2->addWidget(action);
    layout2->addWidget(action1);
    layout2->addWidget(action2);
    layout2->addWidget(action3);
    layout2->addWidget(action4);
    layout2->addWidget(action5);
}

void QRFX::slots_init()
{
      QObject::connect(action2, SIGNAL(clicked()), this, SLOT(action2_init()));
      QObject::connect(action1, SIGNAL(clicked()), this, SLOT(action1_init()));
      QObject::connect(act, SIGNAL(clicked()), this, SLOT(act_init()));
      QObject::connect(action4, SIGNAL(clicked()), this, SLOT(action4_init()));
      QObject::connect(action5, SIGNAL(clicked()), this, SLOT(action5_init()));
      QObject::connect(action3, SIGNAL(clicked()), this, SLOT(action3_init()));
      QObject::connect(action, SIGNAL(clicked()), this, SLOT(action_init()));
      QObject::connect(act1,SIGNAL(clicked()),this,SLOT(aboutf()));
      QObject::connect(act2,SIGNAL(clicked()),this,SLOT(network()));

}

void QRFX::action_init()
{
  console->putData("Starting Normal Boot ... Goodbye\n");
  QProcess* umount = new QProcess();
  umount->start("umount /media/usb");
  QCoreApplication::quit();
  exit(0);
}

void QRFX::action3_init()
{
  console->putData("Updating Software ...  Wait do not select another function\n");
  clearFile("/var/lib/dpkg/lock");
  clearFile("/var/lib/dpkg/lock-frontend");

  arg.clear();
  arg << "dist-upgrade";
  process("apt-get", arg);
  append->write("y");
  append->closeWriteChannel();
}

void QRFX::act_init()
{
    arg.clear();
    arg << "-p" << "-f";
    console->putData("Turning off device ...\n");
    process("poweroff", arg);
}

void QRFX::action2_init()
{
  arg.clear();
  console->putData("Trying to repair the Bootloader ...  Wait do not select another function\n");
  process("update-grub", arg);
}

void QRFX::action5_init()
{
  console->putData("Cleaning the System ... Wait do not select another function\n");
  console->putData("Deleting User Settings ...\n");
  clearDir("/var/cache");
  clearDir("/var/log");
  clearUsersSettings(".config");
  clearUsersSettings(".cache");
  clearUsersSettings(".local");
}


void QRFX::network()
{
    console->putData("Starting Network ...\n");

    QFile script(":/network");
    if(!script.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        exit(0);
    }

    QStringList arg;
    arg << "-c" << script.readAll();
    process("/bin/bash", arg);
    if(append->state() == QProcess::Starting)
    {
        icon2.addFile(QStringLiteral(":/network-transmit-receive.png"), QSize(), QIcon::Normal, QIcon::On);
        act2->setStyleSheet("QPushButton { border: none; }");
        act2->setIcon(icon2);
    }

}


void QRFX::action4_init()
{   
  console->putData("Trying to repair the Package Manager ... Wait do not select another function\n");
  console->putData("Removing Dpkg Lock ...\n");

  clearFile("/var/lib/dpkg/lock");
  clearFile("/var/lib/dpkg/lock-frontend");

  console->putData("Trying to resolve dependencies ...\n");
  arg.clear();
  arg << "--configure" << "-a";
  process("dpkg", arg);
  arg.clear();
  arg << "update";
  process("apt-get", arg);
  arg.clear();
  append->write("y");
  append->closeWriteChannel();

  arg << "install" << "-f";
  process("apt-get", arg);
  append->write("y");
  append->closeWriteChannel();
}

void QRFX::action1_init()
{
  console->putData("Opening Dialog File Script ... Wait do not select another function\n");
  
  QProcess* chkproc = new QProcess();
  chkproc->start("blkid");
  chkproc->waitForFinished();

  QString output(chkproc->readAllStandardOutput());
  int st = output.indexOf("/dev/sdb", 0, Qt::CaseInsensitive);
  
   QDir dir;
   dir.mkdir("/media/usb");

   QString usbPath = output.mid(st, 9);
   QProcess* mount = new QProcess();
   mount->start("mount "+ usbPath +" /media/usb");
   mount->waitForFinished();
   
   QString fileName;

    QProcess::ExitStatus Status = mount->exitStatus();
    if (Status == 0)
     {
       console->putData("USB Mount /media/usb!\n");
       fileName = QFileDialog::getOpenFileName(this, tr("Load a Shell Script"),
                                                       "/media/usb",
                                                      tr("Bash Script (*.bst)"));
     }
     else 
      {

       fileName = QFileDialog::getOpenFileName(this, tr("Load a Shell Script"),
                                                       "/",
                                                      tr("Bash Script (*.bst)"));
      }
      
  QFile script(fileName);
     if(!script.open(QIODevice::ReadOnly | QIODevice::Text))
     {
         console->putData("Error: Cannot Open\n");
     }

     QStringList arg;
     arg << "-c" << script.readAll();
     process("/bin/bash",arg);
}

void QRFX::process(const QString &p, const QStringList arguments)
{
    append = new QProcess;
    append->setProcessChannelMode(QProcess::MergedChannels);
    connect(append, SIGNAL(readyReadStandardOutput()),this,SLOT(putdata()));
    connect(append, SIGNAL(readyReadStandardError()),this,SLOT(putdata()));
    append->start(p, arguments);

       if(append->state() == QProcess::Starting)
        {
        console->putData("");
        }
}

void QRFX::putdata()
{
    QString output=append->readAllStandardOutput();
   console->putData(output);
   QString put=append->readAllStandardError();
  console->putData(put);
}
