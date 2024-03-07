/* (07)
 * Copyright © 2022-2023.
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
 * Created by Frederick Valdez.
 */

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qfileinfo.h>

#include <QtGui/qguiapplication.h>

#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/QOpenGLFunctions>

#include <QTextCodec>
#include <QDebug>
#include <QDir>

#include <stdio.h>
#include "appsettings.h"
#include "qmlapp.h"

#include <QRect>
#include <QResource>

#define Debug qDebug()
#define Info qWarning

extern struct cache var;
Options opt;

void options(void)
{
    puts("Usage: qmlapp [options] <filename>");
    puts("");
    puts(" Options:");
    puts("-i <path>       Add <path> to the list of import paths");
    puts("-d, --debug  debug all info");
    puts("--quit          Force Quit after starting"              );
    puts("");
    exit(1);
}

#include <QOpenGLFunctions>

void onOpenGlContextCreated(QOpenGLContext *context, QQuickWindow *window)
{
    if (!window || !context)
        return;

    context->makeCurrent(window);
    QOpenGLFunctions functions(context);
    QByteArray output = "Vendor  : ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_VENDOR));
    output += "\nRenderer: ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_RENDERER));
    output += "\nVersion : ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_VERSION));
    output += "\nLanguage: ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_SHADING_LANGUAGE_VERSION));
    puts(output.constData());
    context->doneCurrent();
}

int main(int argc, char ** argv)
{
    QStringList imports;
    int i;

    for (i = 1; i < argc; ++i) {
        if(*argv[i] != '-'
          && QFileInfo(QFile::decodeName(argv[i])).exists())
        {
              if(argc == 2)
              {
                  var.array = qmlshell(argv[i]);
              }
              else if(argc == 3)
              {
                  var.url =  QUrl::fromLocalFile(argv[i]);
              }
         }
        else {
            const QString Argument = QString::fromLatin1(argv[i]).toLower();
             if(Argument == QLatin1String("--quit")
                     || Argument == QLatin1String("--force"))
                  opt.quitForce = true;
             else if(Argument == QLatin1String("--debug")
                     || Argument == QLatin1String("-debug"))
                  opt.debug = true;
             else if (Argument == QLatin1String("-i") && i + 1 < argc)
                             imports.append(QString::fromLatin1(argv[++i]));
             else if (Argument == QLatin1String("--help")
                                  || Argument == QLatin1String("-help")
                                  || Argument == QLatin1String("--h")
                                  || Argument == QLatin1String("-h"))
                            options();
           }
        }

    /* define variable */
    QGuiApplication app(argc, argv); /* start Event loop */
    QQmlEngine engine;
    QPointer<QQmlComponent> component = new QQmlComponent(&engine);  
    AppSettings Settings;
    engine.rootContext()->setContextProperty("Settings", &Settings);


    /* check qrc file */
    QString file = QDir::currentPath() + "/resources.rcc";
        if(QFileInfo::exists(file))
        {
             QResource::registerResource(QDir::currentPath() + "/resources.rcc");
             if(opt.debug)
               qDebug() << "Register Resources [ OK ]";
        }


    /* Active actributes */
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_PluginApplication);
    QCoreApplication::setAttribute(Qt::AA_NativeWindows);
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_SetPalette);
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents);

    /* set env */
    QByteArray DebugValue = qgetenv("ENABLE_DEBUG");
    QByteArray Import = qgetenv("QML_IMPORT_PATH");

     if (DebugValue.toLower() == "true") {
         opt.debug = true;
     }

     if(!Import.isNull())
     {
        imports.append(QString::fromUtf8(Import));
        addImport(&engine, &imports);
     }

    /* quit engine */
    QObject::connect(&engine, SIGNAL(quit())
                     , QCoreApplication::instance(), SLOT(quit()));
    QObject::connect(&engine, &QQmlEngine::exit
                      , QCoreApplication::instance(), &QCoreApplication::exit);

    /* read info section */
    read_info(&engine);
    conf_parse(&app);
    addImport(&engine, &imports);

    if(argc == 2)
    {
      component->setData(var.array, QUrl());
    }
    else if(argc == 3)
    {
      component->loadUrl(var.url);
    }

    while (component->isLoading())
    QCoreApplication::processEvents();

    if(opt.quitForce)
    {
        QMetaObject::invokeMethod(QCoreApplication::instance()
                                , "quit", Qt::QueuedConnection);
    }

    /* check ready componet */
    if (!component->isReady() ) {
         fprintf(stderr, "Error%s\n", qPrintable(component->errorString()));
         return -1;
     }
    QObject *top = component->create();
    if (!top && component->isError()) {
           fprintf(stderr, "Error%s\n", qPrintable(component->errorString()));
           return -1;
      }
    /* create window */
    QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(top));

    if(var.icon != NULL)
    {
        QString icon = dirpath(var.icon);
        window->setIcon(QIcon(icon));
    }

    /* set window size */
    Settings.setWidth(window->width());
    Settings.setHeight(window->height());

    /* QRect */
    QRect geometry = window->geometry();
    Settings.setGeometryX(geometry);
    Settings.setGeometryY(geometry);
    Settings.setGeometryWidth(geometry);
    Settings.setGeometryHeight(geometry);

    switch(var.win)
    {
        case 1: // window type
         {
           if (opt.debug) {
             qDebug() << "Window: engine";
           }
          if(window){
              engine.setIncubationController(window->incubationController());
           } else {
              Debug << "[ ERROR ]:" << "Window Manager not valid";
              exit(1);
           }
         }
        break;
        case 2:
         {
                if  (opt.debug){
                    qDebug() << "Window: host";
                }
                /* active Attributes */
                QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
                QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
                QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
                QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

               /* Create item Quick */
                QQuickItem *Item = qobject_cast<QQuickItem *>(top);
                if (Item) {
                    QQuickView* xView = new QQuickView(&engine, NULL);
                    window.reset(xView);
                    xView->setResizeMode(QQuickView::SizeRootObjectToView);
                    xView->setContent(QUrl(), component, Item);
                    xView->show();
                  }

              if(window) {
                  if(opt.debug) {
                     QOpenGLContext context;
                     if (!context.create())
                         return -1;
                     onOpenGlContextCreated(&context, window.data());
                   }
                  /* Create surface window */
                  QSurfaceFormat surface = window->requestedFormat(); /* Rendering OpenGL window in qml */
                  surface.setRenderableType(QSurfaceFormat::OpenGL);
                  surface.setProfile(QSurfaceFormat::CoreProfile);
                  surface.setSamples(16);
                  surface.setAlphaBufferSize(8);
                  window->setClearBeforeRendering(true);
                  window->setColor(QColor(Qt::transparent));
                  window->setFormat(surface);
                  window->isPersistentOpenGLContext();
               }
             }
        break;
       default:
             {
             if(window)
                {
                    engine.setIncubationController(window->incubationController());
                } else {
                    QQuickItem *Item = qobject_cast<QQuickItem *>(top);
                    if (Item) {
                        QQuickView* xView = new QQuickView(&engine, NULL);
                        window.reset(xView);
                        xView->setResizeMode(QQuickView::SizeRootObjectToView);
                        xView->setContent(QUrl(), component, Item);
                        xView->show();
                   }
                }
            }
         }

    if (window->flags() == Qt::Window) // Fix window flags QML.
        window->setFlags(Qt::Window | Qt::WindowSystemMenuHint
                         | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint
                         | Qt::WindowCloseButtonHint | Qt::WindowFullscreenButtonHint
                         | Qt::WindowMaximizeButtonHint | Qt::WindowSystemMenuHint
                         | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput
                         | Qt::WindowStaysOnBottomHint | Qt::WindowDoesNotAcceptFocus
                         | Qt::MaximizeUsingFullscreenGeometryHint | Qt::WindowType_Mask
                         | Qt::WindowMinimizeButtonHint | Qt::WindowShadeButtonHint | Qt::WindowContextHelpButtonHint);


     /* delete */
     delete component;

     return app.exec();  

}

