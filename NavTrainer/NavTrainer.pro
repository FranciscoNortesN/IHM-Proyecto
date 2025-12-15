QT       += core gui svg svgwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    carta.cpp \
    help.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    mapoverlaypanel.cpp \
    problem.cpp \
    selecpro.cpp \
    stats.cpp \
    usermanagement.cpp \
    navlib/navigation.cpp \
    navlib/navigationdao.cpp \
    user.cpp

HEADERS += \
    carta.h \
    help.h \
    login.h \
    mainwindow.h \
    mapoverlaypanel.h \
    maptooltypes.h \
    problem.h \
    selecpro.h \
    stats.h \
    usermanagement.h \
    navlib/navigation.h \
    navlib/navigationdao.h \
    navlib/navdaoexception.h \
    navlib/navtypes.h \
    user.h

FORMS += \
    help.ui \
    login.ui \
    mainwindow.ui \
    problem.ui \
    selecpro.ui \
    stats.ui \
    user.ui \
    usermanagement.ui

TRANSLATIONS += \
    NavTrainer_es_ES.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Assets.qrc

# Incluir directorio de navlib para los includes
INCLUDEPATH += $$PWD/navlib

# Agregar SQL para usar la base de datos de navegación
QT += sql
