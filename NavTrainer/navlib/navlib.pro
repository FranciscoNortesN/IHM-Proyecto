TEMPLATE = lib
CONFIG += staticlib c++17
CONFIG -= app_bundle

TARGET = navlib

QT += core gui sql

INCLUDEPATH += .

HEADERS += \
    navdaoexception.h \
    navtypes.h \
    navigationdao.h \
    navigation.h

SOURCES += \
    navigationdao.cpp \
    navigation.cpp

# Configurar el directorio de salida para que la librería se compile en el build del proyecto padre
# Esto asegura que NavTrainer pueda encontrar la librería
