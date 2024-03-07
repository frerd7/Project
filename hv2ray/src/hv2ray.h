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
