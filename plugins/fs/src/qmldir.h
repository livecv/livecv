#ifndef DIR_H
#define DIR_H

#include <QObject>
#include <QJSValue>

namespace lv {
/// \private
class QmlDir : public QObject
{
    Q_OBJECT
public:
    explicit QmlDir(QObject *parent = nullptr);

public slots:
    QStringList list(QJSValue path);
    bool mkDir(QJSValue path);
    bool mkPath(QJSValue path);
    bool remove(QJSValue path);
    bool rename(QJSValue old, QJSValue nu);
};

}

#endif // DIR_H