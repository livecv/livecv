#include "runnable.h"
#include "runnablecontainer.h"

#include "live/hookcontainer.h"
#include "live/exception.h"
#include "live/viewcontext.h"
#include "live/viewengine.h"
#include "live/project.h"
#include "live/projectfile.h"
#include "live/projectdocumentmodel.h"
#include "live/qmlbuild.h"

#include <QQmlProperty>
#include <QQmlListReference>

#include <QFileInfo>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTimer>

#ifdef BUILD_ELEMENTS
#include"live/elements/engine.h"
#endif

namespace lv{

Runnable::Runnable(
        ViewEngine* viwEngine,
        el::Engine* e,
        const QString &path,
        RunnableContainer* parent,
        const QString &name,
        const QSet<QString>& activations)
    : QObject(parent)
    , m_name(name)
    , m_path(path)
    , m_component(nullptr)
    , m_runSpace(nullptr)
    , m_viewEngine(viwEngine)
    , m_viewRoot(nullptr)
    , m_viewContext(nullptr)
    , m_type(Runnable::QmlFile)
    , m_activations(activations)
    , m_scheduleTimer(nullptr)
    , m_runTrigger(Project::RunManual)
    , m_engine(e)
{
    QString ext = QFileInfo(path).suffix();
    if ( ext == "lv" && m_engine )
        m_type = Runnable::LvFile;

    connect(viwEngine, &ViewEngine::objectAcquired,      this, &Runnable::engineObjectAcquired);
    connect(viwEngine, &ViewEngine::objectReady,         this, &Runnable::engineObjectReady);
    connect(viwEngine, &ViewEngine::objectCreationError, this, &Runnable::engineObjectCreationError);

    m_project = qobject_cast<Project*>(parent->parent());
}

Runnable::Runnable(ViewEngine* engine, QQmlComponent *component, RunnableContainer *parent, const QString &name)
    : QObject(parent)
    , m_name(name)
    , m_component(component)
    , m_runSpace(nullptr)
    , m_viewEngine(engine)
    , m_viewRoot(nullptr)
    , m_type(Runnable::QmlComponent)
    , m_scheduleTimer(nullptr)
    , m_runTrigger(Project::RunManual)
{
    m_project = qobject_cast<Project*>(parent->parent());
}

Runnable::~Runnable(){
    m_project->removeExcludedRunTriggers(m_activations);

    if ( m_viewRoot ){
        disconnect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);

        m_viewRoot->setParent(nullptr);
        auto item = qobject_cast<QQuickItem*>(m_viewRoot);
        if (item)
            item->setParentItem(nullptr);
        m_viewRoot->deleteLater();
    }
}

void Runnable::run(){
    if ( m_type == Runnable::QmlFile ){

        RunnableContainer* runnableContainer = static_cast<RunnableContainer*>(parent());

        ProjectDocument* document = ProjectDocument::castFrom(m_project->isOpened(m_path));
        if ( document ){
            auto documentList = m_project->documentModel()->listUnsavedDocuments();

            if ( m_project->active() == this ){

                QQmlContext* ctx = createContext();

                QmlBuild* build = static_cast<QmlBuild*>(ctx->contextProperty("build").value<QObject*>());

                runnableContainer->announceQmlBuild(this, build);
                build->setState(QmlBuild::Compiling);

                m_viewEngine->createObjectAsync(
                    document->content(),
                    m_runSpace,
                    QUrl::fromLocalFile(m_path),
                    this,
                    ctx,
                    !(documentList.size() == 1 && documentList[0] == document->file()->path())
                );
            } else {
                QQmlContext* ctx = createContext();
                QmlBuild* build = static_cast<QmlBuild*>(ctx->contextProperty("build").value<QObject*>());

                runnableContainer->announceQmlBuild(this, build);
                build->setState(QmlBuild::Compiling);

                QObject* obj = createObject(document->content(), QUrl::fromLocalFile(m_path), ctx);

                if ( obj ){

                    emptyRunSpace();

                    m_viewContext = ctx;
                    m_viewRoot = obj;
                    connect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);

                    obj->setParent(m_runSpace);

                    QQuickItem *parentItem = qobject_cast<QQuickItem*>(m_runSpace);
                    QQuickItem *item = qobject_cast<QQuickItem*>(obj);
                    if (parentItem && item){
                        item->setParentItem(parentItem);
                    }

                    build->setState(QmlBuild::Ready);

                } else {
                    ctx->deleteLater();
                }

                emit objectReady(obj);
            }
        } else {
            QFile f(m_path);
            if ( !f.open(QFile::ReadOnly) )
                THROW_EXCEPTION(Exception, "Failed to read file for running:" + m_path.toStdString(), Exception::toCode("~File"));

            QByteArray contentBytes = f.readAll();

            if ( m_project->active() == this ){
                QQmlContext* ctx = createContext();
                QmlBuild* build = static_cast<QmlBuild*>(ctx->contextProperty("build").value<QObject*>());
                runnableContainer->announceQmlBuild(this, build);
                build->setState(QmlBuild::Compiling);

                m_viewEngine->createObjectAsync(
                    contentBytes,
                    m_runSpace,
                    QUrl::fromLocalFile(m_path),
                    this,
                    ctx,
                    true
                );
            } else {
                QQmlContext* ctx = createContext();

                QmlBuild* build = static_cast<QmlBuild*>(ctx->contextProperty("build").value<QObject*>());
                runnableContainer->announceQmlBuild(this, build);
                build->setState(QmlBuild::Compiling);

                QObject* obj = createObject(contentBytes, QUrl::fromLocalFile(m_path), ctx);

                if ( obj ){
                    emptyRunSpace();

                    m_viewContext = ctx;
                    m_viewRoot = obj;
                    connect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);

                    obj->setParent(m_runSpace);

                    QQuickItem *parentItem = qobject_cast<QQuickItem*>(m_runSpace);
                    QQuickItem *item = qobject_cast<QQuickItem*>(obj);
                    if (parentItem && item){
                        item->setParentItem(parentItem);
                    }

                    build->setState(QmlBuild::Ready);

                } else {
                    ctx->deleteLater();
                }

                emit objectReady(obj);
            }

        }
    } else if ( m_type == Runnable::QmlComponent ){

        QObject* obj = qobject_cast<QObject*>(m_component->create(m_component->creationContext()));
        if (!obj){
            THROW_EXCEPTION(Exception, "Failed to create item: " + m_component->errorString().toStdString(), Exception::toCode("~Component") );
        }

        emptyRunSpace();

        m_viewRoot = obj;
        connect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);

        obj->setParent(m_runSpace);

        QQuickItem* appRootItem = qobject_cast<QQuickItem*>(obj);
        QQuickItem* runspaceItem = qobject_cast<QQuickItem*>(m_runSpace);

        if ( appRootItem && runspaceItem ){
            appRootItem->setParentItem(runspaceItem);
        }

        emit objectReady(obj);
    } else if ( m_type == Runnable::LvFile ){
        ProjectDocument* document = ProjectDocument::castFrom(m_project->isOpened(m_path));

        std::string content;

        if ( document ){
            content = document->content().toStdString();
        } else {
            content = m_project->lockedFileIO()->readFromFile(m_path.toStdString());
        }

#ifdef BUILD_ELEMENTS
        try{
            el::Object o = m_engine->loadFile(m_path.toStdString(), content);
            std::string componentName = QFileInfo(m_path).baseName().toStdString();
            el::Object::Accessor lo(o);
            el::ScopedValue lval = lo.get(m_engine, componentName);

            el::Element* e = lval.toElement(m_engine);
            m_runtimeRoot = e;
        } catch ( lv::Exception& e ){
            vlog().e() << e.message();
        }
#endif
    }
}

void Runnable::_activationContentsChanged(int, int, int){
    m_scheduleTimer->start();
}

void Runnable::_documentOpened(Document *document){
    ProjectDocument* pd = ProjectDocument::castFrom(document);
    if ( !pd )
        return;

    if ( m_activations.contains(document->file()->path()) ){
        if ( m_runTrigger == Project::RunOnChange ){
            connect(pd->textDocument(), &QTextDocument::contentsChange, this, &Runnable::_activationContentsChanged);
        } else {
            connect(document, &ProjectDocument::saved, this, &Runnable::_documentSaved);
        }
    }
}

void Runnable::_documentSaved(){
    m_scheduleTimer->start();
}

void Runnable::setRunTrigger(int runTrigger){
    if (m_runTrigger == runTrigger)
        return;

    if ( m_activations.size() ){

        if ( m_runTrigger == Project::RunOnChange ){
            m_project->removeExcludedRunTriggers(m_activations);
            m_scheduleTimer->deleteLater();
            m_scheduleTimer = nullptr;
            disconnect(m_project, &Project::documentOpened, this, &Runnable::_documentOpened);
            for ( auto it = m_activations.begin(); it != m_activations.end(); ++it ){
                const QString& activation = *it;
                ProjectDocument* document = ProjectDocument::castFrom(m_project->isOpened(activation));
                if ( document ){
                    disconnect(document->textDocument(), &QTextDocument::contentsChange, this, &Runnable::_activationContentsChanged);
                }
            }

        } else if ( m_runTrigger == Project::RunOnSave ){
            m_project->removeExcludedRunTriggers(m_activations);
            m_scheduleTimer->deleteLater();
            m_scheduleTimer = nullptr;
            disconnect(m_project, &Project::documentOpened, this, &Runnable::_documentOpened);
            for ( auto it = m_activations.begin(); it != m_activations.end(); ++it ){
                const QString& activation = *it;
                ProjectDocument* document = ProjectDocument::castFrom(m_project->isOpened(activation));
                if ( document ){
                    disconnect(document, &ProjectDocument::saved, this, &Runnable::_documentSaved);
                }
            }

        }

        if ( runTrigger == Project::RunOnChange ){
            m_project->excludeRunTriggers(m_activations);
            m_scheduleTimer = new QTimer(this);
            m_scheduleTimer->setInterval(1000);
            m_scheduleTimer->setSingleShot(true);
            connect(m_scheduleTimer, &QTimer::timeout, this, &Runnable::run);
            connect(m_project, &Project::documentOpened, this, &Runnable::_documentOpened);

            for ( auto it = m_activations.begin(); it != m_activations.end(); ++it ){
                const QString& activation = *it;
                ProjectDocument* document = ProjectDocument::castFrom(m_project->isOpened(activation));
                if ( document ){
                    connect(document->textDocument(), &QTextDocument::contentsChange, this, &Runnable::_activationContentsChanged);
                }
            }

        } else if ( runTrigger == Project::RunOnSave ){
            m_project->excludeRunTriggers(m_activations);
            m_scheduleTimer = new QTimer(this);
            m_scheduleTimer->setInterval(1000);
            m_scheduleTimer->setSingleShot(true);
            connect(m_project, &Project::documentOpened, this, &Runnable::_documentOpened);
            for ( auto it = m_activations.begin(); it != m_activations.end(); ++it ){
                const QString& activation = *it;
                ProjectDocument* document = ProjectDocument::castFrom(m_project->isOpened(activation));
                if ( document ){
                    connect(document, &ProjectDocument::saved, this, &Runnable::_documentSaved);
                }
            }
        }
    }

    m_runTrigger = runTrigger;
    emit runTriggerChanged();
}

void Runnable::clearRoot()
{
    if (m_viewRoot != sender()) return;
    m_viewRoot = nullptr;
    m_viewContext->deleteLater();
    m_viewContext = nullptr;
}

void Runnable::runLv(){
    //TODO
}

QObject* Runnable::createObject(const QByteArray &code, const QUrl &file, QQmlContext *context){
    m_viewEngine->engine()->clearComponentCache();

    QQmlComponent component(m_viewEngine->engine());
    component.setData(code, file);

    QList<QQmlError> errors = component.errors();
    if ( errors.size() ){
        emit runError(m_viewEngine->toJSErrors(errors));
        return nullptr;
    }

    QObject* obj = component.create(context);
    m_viewEngine->engine()->setObjectOwnership(obj, QQmlEngine::JavaScriptOwnership);

    errors = component.errors();
    if ( errors.size() ){
        emit runError(m_viewEngine->toJSErrors(errors));
        return nullptr;
    }

    return obj;
}


QQmlContext *Runnable::createContext(){
    QQmlContext* ctx = new QQmlContext(m_viewEngine->engine()->rootContext());

    HookContainer* hooks = new HookContainer(m_project->dir(), this, ctx);
    QmlBuild* build      = new QmlBuild(this, ctx);
    ctx->setContextProperty("hooks", hooks);
    ctx->setContextProperty("build", build);
    return ctx;
}

void Runnable::swapViewRoot(QObject *newViewRoot){
    newViewRoot->setParent(m_runSpace);
    QQuickItem* newRootItem = qobject_cast<QQuickItem*>(newViewRoot);
    QQuickItem* runSpaceItem = qobject_cast<QQuickItem*>(m_runSpace);
    if ( newRootItem && runSpaceItem ){
        newRootItem->setParentItem(runSpaceItem);
    }

    if (m_viewRoot)
        disconnect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);
    m_viewRoot = newViewRoot;

    QQuickItem* rootItem = qobject_cast<QQuickItem*>(m_runSpace);
    if ( rootItem ){
        QQmlProperty pp(rootItem);
        QQmlListReference ppref = qvariant_cast<QQmlListReference>(pp.read());
        if ( ppref.canAppend() ){
            ppref.append(newViewRoot);
        }
    }

    connect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);
}

void Runnable::engineObjectAcquired(const QUrl &, QObject *ref){
    if ( ref == this ){
        emptyRunSpace();
    }
}

void Runnable::engineObjectReady(QObject *object, const QUrl &, QObject *ref, QQmlContext* context){
    if ( ref == this ){
        if (m_viewRoot)
            disconnect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);
        m_viewRoot    = object;

        QQuickItem* rootItem = qobject_cast<QQuickItem*>(m_runSpace);
        if ( rootItem ){
            QQmlProperty pp(rootItem);
            QQmlListReference ppref = qvariant_cast<QQmlListReference>(pp.read());
            if ( ppref.canAppend() ){
                ppref.append(object);
            }
        }

        m_viewContext = context;

        QmlBuild* build = static_cast<QmlBuild*>(context->contextProperty("build").value<QObject*>());
        build->setState(QmlBuild::Ready);

        connect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);
        emit objectReady(object);
    }
}

void Runnable::engineObjectCreationError(QJSValue errors, const QUrl &, QObject *ref, QQmlContext *context){
    if ( ref == this ){
        delete context;
        emit runError(errors);
    }
}

void Runnable::emptyRunSpace(){
    if ( m_viewRoot ){
        QQuickItem* appRootItem = qobject_cast<QQuickItem*>(m_viewRoot);
        if ( appRootItem ){
            appRootItem->setParentItem(nullptr);
        }
        disconnect(m_viewRoot, &QObject::destroyed, this, &Runnable::clearRoot);
        m_viewRoot->setParent(nullptr);
        m_viewRoot->deleteLater();
        m_viewRoot = nullptr;
        m_viewContext->deleteLater();
        m_viewContext = nullptr;
    }
}

void Runnable::setRunSpace(QObject *runspace){
    if ( m_runSpace == runspace )
        return;

    if ( m_runSpace ){
        emptyRunSpace();
    }

    m_runSpace = runspace;
}

}// namespace
