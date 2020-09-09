QT += core testlib

QCE = ../src
INCLUDEPATH += $$QCE

SOURCES += \
#    main.cpp \
    $$QCE/asmparser.cpp \
    testasmparser.cpp

HEADERS += \
    $$QCE/asmparser.h \
    testasmparser.h

RESOURCES += \
    tests_rc.qrc
