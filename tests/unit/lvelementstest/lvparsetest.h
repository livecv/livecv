#ifndef LVPARSETEST_H
#define LVPARSETEST_H

#include <QObject>
#include "testrunner.h"
#include "live/lockedfileiosession.h"

class LvParseTest : public QObject{

    Q_OBJECT
    Q_TEST_RUNNER_SUITE

public:
    explicit LvParseTest(QObject *parent = 0);
    ~LvParseTest(){}

private slots:
    void initTestCase();

    void constructorParseTest();
    void functionParseTest();
    void nestingAndShortIdParseTest();
    void scopeParseTest();
    void propertyNewExpressionParseTest();
    void propertyDeclarationParseTest();
    void propertyExpressionsParseTest();
    void taggedStringParseTest();
    void instanceParseTest();
    void namespaceInheritance();
    void testComplexPropertyAssignment();
    void testSimplePropertyAssignment();
    void testScenarioTest();
    void nestedLanguageScannerTest();
    void nestedFunctionAssignment();
    void arrowFunctionElement();
    void todoListItemSmall();
    void todoList();
    void todoListItem();
    void todoForm();
    void todoApp();
    void complexTernaryOperator();
    void doublyNestedElement();
    void idObjectInJsScope();
    void nestedObjectsInJsScope();
    void sameIdInBothScopes();

private:
    void parseTestTemplate(std::string name);

    lv::LockedFileIOSession::Ptr m_fileSession;
    std::string                  m_scriptPath;
};

#endif // LVPARSETEST_H
