CONFIG(debug, debug|release) {
    BINPATH = $$PWD/bin/$${QMAKE_TARGET.arch}/debug
} else {
    BINPATH = $$PWD/bin/$${QMAKE_TARGET.arch}/release
}

CONFIG += c++11
DESTDIR = $${BINPATH}
LIBS += -L$${BINPATH}
