TEMPLATE = lib
TARGET   = lcvvideo
QT      += qml quick
CONFIG  += qt plugin

TARGET = $$qtLibraryTarget($$TARGET)
uri = plugins.lcvvideo

OTHER_FILES = qmldir

DEFINES += Q_LCV
DEFINES += Q_LCVVIDEO_LIB

include($$PWD/../use_lcvcore.pri)
include($$PWD/../use_liblive.pri)
include($$PWD/src/lcvvideo.pri)
include($$PWD/include/lcvvideoheaders.pri)
include($$PWD/../../3rdparty/opencvconfig.pro)
deployOpenCV()

# Destination

win32:CONFIG(debug, debug|release): DLLDESTDIR = $$quote($$OUT_PWD/../../application/debug/plugins/lcvvideo)
else:win32:CONFIG(release, debug|release): DLLDESTDIR = $$quote($$OUT_PWD/../../application/release/plugins/lcvvideo)
else:unix: DESTDIR = $$quote($$OUT_PWD/../../application/plugins/lcvvideo)

# Qml

OTHER_FILES = \
    $$PWD/qml/qmldir \
    $$PWD/qml/plugins.qmltypes

# Deploy qml

PLUGIN_DEPLOY_FROM = $$PWD/qml
win32:CONFIG(debug, debug|release): PLUGIN_DEPLOY_TO = $$OUT_PWD/../../application/debug/plugins/lcvvideo
else:win32:CONFIG(release, debug|release): PLUGIN_DEPLOY_TO = $$OUT_PWD/../../application/release/plugins/lcvvideo
else:unix: PLUGIN_DEPLOY_TO = $$OUT_PWD/../../application/plugins/lcvvideo

win32:PLUGIN_DEPLOY_TO ~= s,/,\\,g
win32:PLUGIN_DEPLOY_FROM ~= s,/,\\,g

plugincopy.commands = $(COPY_DIR) \"$$PLUGIN_DEPLOY_FROM\" \"$$PLUGIN_DEPLOY_TO\"

first.depends = $(first) plugincopy
export(first.depends)
export(plugincopy.commands)

QMAKE_EXTRA_TARGETS += first plugincopy
