QT += core
QT -= gui
include(ZTPManager/ZTPManager.pri)

CONFIG += c++11

TARGET = room-daemon
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    mytask.cpp

HEADERS += \
    mytask.h
