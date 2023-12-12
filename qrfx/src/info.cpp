/* info.cpp
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
 
 
#include "info.h"
#include "ui_info.h"
#include <stdio.h>
#include <string.h>
#include <QStorageInfo>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

info::info(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::info)
{
    ui->setupUi(this);
    ui->widget->setStyleSheet("background-color: black;");  
    cpu_info("/proc/cpuinfo", "model name");
    quit();
}

void info::quit()
{
  QObject::connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(close()));
  QObject::connect(ui->pushButton_2,SIGNAL(clicked()),this,SLOT(close()));
}



void info::cpu_info(const char *f, const char *str)
{
    FILE *fp = fopen(f, "r");
    if(fp == 0)
    {
       qWarning("cannot open");
    }
    else
    {
        char line[1024];
        while(fgets(line, sizeof(line), fp) != NULL)
        {
             if(strncmp(line, str, 10) == 0)
             {
                 char *model = strchr(line, ':');
                 if (model != NULL)
                 {
                     model += (model[1] == ' ') ? 2 : 1;

                     if(strcmp(model, model) == 0)
                     {
                         QString st;
                         st.sprintf("%sx2",model);
                         ui->label_3->setText(st);
                    }
                     else
                     {
                         QString st;
                         st.sprintf("%s",model);
                         ui->label_3->setText(st);
                     }

                 }
             }
        }
    }

 mem_info();
 disk_info();
 os_release();

}

void info::mem_info()
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if(fp == 0)
    {
       qWarning("cannot open");
    }
    else
    {
        char line[1024];
        while(fgets(line, sizeof(line), fp) != NULL)
        {
           int ram;
           if(sscanf(line, "MemTotal: %d Kb", &ram) == 1)
           {
                QString st;
                st.sprintf("%d KB",ram);
                ui->label_6->setText(st);
           }

        }
    }
}

void info::disk_info()
{
    QStorageInfo storage = QStorageInfo::root();
    QString st;
    st.sprintf("%lld KB", storage.bytesTotal());
    ui->label_8->setText(st);
    ui->label_12->setText(storage.fileSystemType());
}
void info::os_release()
{
    FILE *fp = fopen("/etc/os-release", "r");
    if(fp == 0)
    {
       qWarning("cannot open");
    }
    else
    {
        char line[1024];
        while(fgets(line, sizeof(line), fp) != NULL)
        {
             if(strncmp(line, "PRETTY_NAME", 4) == 0)
             {
                 char *model = strchr(line, '=');
                 if (model != NULL)
                 {
                     model += (model[1] == ' ') ? 2 : 1;
                     QString st;
                     st.sprintf("%s",model);
                     ui->label_10->setText(st);
                 }
             }
        }


    }


}


info::~info()
{
    delete ui;
}
