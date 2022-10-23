
DEPENDPATH     += . $$PWD/include $$PWD/src
INCLUDEPATH    += . $$PWD/include $$PWD/src

HEADERS += \
    $$PWD/include/cn-cbor.h \
    $$PWD/src/cbor.h \
    $$PWD/src/dll-export.h

SOURCES += \
    $$PWD/src/cn-cbor.c \
    $$PWD/src/cn-create.c \
    $$PWD/src/cn-encoder.c \
    $$PWD/src/cn-error.c \
    $$PWD/src/cn-get.c \
    $$PWD/src/cn-parsejson.c \
    $$PWD/src/cn-print.c
