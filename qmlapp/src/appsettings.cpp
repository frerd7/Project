#include "appsettings.h"
#include "qmlapp.h"
#include <QRect>
#include <QDebug>
#include <QDBusConnection>

extern struct cache var;

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_width(800)
    , m_height(600)
    , m_sessionBus(QDBusConnection::sessionBus())
{
}

int AppSettings::getWidth() const
{
    return m_width;
}

void AppSettings::setWidth(int width)
{
    if (m_width != width) {
        m_width = width;
        emit widthChanged();
    }
}

int AppSettings::getHeight() const
{
    return m_height;
}

void AppSettings::setHeight(int height)
{
    if (m_height != height) {
        m_height = height;
        emit heightChanged();
    }
}

QRect AppSettings::getGeometryx() const
{
    return m_geometryx;
}

void AppSettings::setGeometryX(QRect& geometry)
{
    int x = geometry.x();
    if (m_geometryx.x() != x)
    {
        m_geometryx.setX(x);
        emit geometryxChanged();
    }
}

QRect AppSettings::getGeometryy() const
{
    return m_geometryy;
}

void AppSettings::setGeometryY(QRect& geometry)
{
    int y = geometry.y();
    if (m_geometryy.y() != y)
    {
        m_geometryy.setY(y);
        emit geometryxChanged();
    }
}

QRect AppSettings::getGeometrywidth() const
{
   return m_geometrywidth;
}

void AppSettings::setGeometryWidth(QRect& geometry)
{
    int fullwidth = geometry.width();
    if(m_geometrywidth.width() != fullwidth)
    {
        m_geometrywidth.setWidth(fullwidth);
        emit geometryWidthChanged();
    }
}

QRect AppSettings::getGeometryheight() const
{
   return m_geometryheight;
}

void AppSettings::setGeometryHeight(QRect& geometry)
{
    int fullheight = geometry.height();
    if(m_geometryheight.height() != fullheight)
    {
        m_geometryheight.setHeight(fullheight);
        emit geometryWidthChanged();
    }
}

void AppSettings::sendNotification(const QString& body)
{
    if (!m_sessionBus.isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.");
        return;
    }

    QString name = "QMLAPP";
    QString icon = " ";

    QDBusMessage message = QDBusMessage::createMethodCall(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify"
    );

    if(var.app != NULL)
    {
        name = var.app;
    }

    if(var.icon != NULL)
    {
        icon = dirpath(var.icon);
    }

    QVariantList args;
    args << "App"
         << uint(0)
         << icon
         << name
         << body
         << QStringList()
         << QVariantMap()
         << int(-1);

    message.setArguments(args);
    QDBusMessage reply = m_sessionBus.call(message);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning("Failed to send D-Bus message. Error: %s", qPrintable(reply.errorMessage()));
    }
}
