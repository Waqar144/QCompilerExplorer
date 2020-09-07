QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = QCompilerExplorer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

### For some reason, i am unable to change font size of the editor if i include like this in Qt 5.12
### It works fine with Qt 5.15 / 5.14 though
#include(QSourceHighlite/QSourceHighlite.pri)

SOURCES += \
    asmparser.cpp \
    compiler.cpp \
    compilerservice.cpp \
    main.cpp \
    settingsdialog.cpp \
    asmtextedit.cpp \
    mainwindow.cpp \
    QSourceHighlite/qsourcehighliter.cpp \
    QSourceHighlite/languagedata.cpp \
    QSourceHighlite/qsourcehighliterthemes.cpp \
    widgets/QCodeEditor.cpp \
    widgets/QLineNumberArea.cpp \
    widgets/filelistwidget.cpp \
    widgets/argslineedit.cpp

HEADERS += \
    asmhighlighter.h \
    asmparser.h \
    ce_endpoints.h \
    compiler.h \
    compilerservice.h \
    settingsdialog.h \
    asmtextedit.h \
    mainwindow.h \
    QSourceHighlite/qsourcehighliter.h \
    QSourceHighlite/languagedata.h \
    QSourceHighlite/qsourcehighliterthemes.h \
    widgets/QCodeEditor.h \
    widgets/QLineNumberArea.h \
    widgets/filelistwidget.h \
    widgets/argslineedit.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
