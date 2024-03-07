#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QRect>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>

class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int width READ getWidth WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ getHeight WRITE setHeight NOTIFY heightChanged)
    Q_PROPERTY(QRect GeometryX READ getGeometryx WRITE setGeometryX  NOTIFY geometryxChanged)
    Q_PROPERTY(QRect GeometryY READ getGeometryy WRITE setGeometryY NOTIFY geometryyChanged)
    Q_PROPERTY(QRect GeometryWidth READ getGeometrywidth WRITE setGeometryWidth NOTIFY geometryWidthChanged)
    Q_PROPERTY(QRect GeometryHeight READ getGeometryheight WRITE setGeometryHeight NOTIFY geometryHeightChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);

    int getWidth() const;
    void setWidth(int width);

    int getHeight() const;
    void setHeight(int height);

    QRect getGeometryx() const;
    void setGeometryX(QRect& geometry);

    QRect getGeometryy() const;
    void setGeometryY(QRect& geometry);

    QRect getGeometrywidth() const;
    void setGeometryWidth(QRect& geometry);

    QRect getGeometryheight() const;
    void setGeometryHeight(QRect& geometry);

   Q_INVOKABLE void sendNotification(const QString& body);

signals:
    void widthChanged();
    void heightChanged();
    void geometryxChanged();
    void geometryyChanged();
    void geometryWidthChanged();
    void geometryHeightChanged();

private:
    int m_width;
    int m_height;
    QRect m_geometryx;
    QRect m_geometryy;
    QRect m_geometrywidth;
    QRect m_geometryheight;
    QDBusConnection m_sessionBus;
};

#endif // APPSETTINGS_H
