#include <QString>
#include <QtTest>
#include <QCoreApplication>

class Deferedtask_testTest : public QObject
{
    Q_OBJECT

public:
    Deferedtask_testTest();

private Q_SLOTS:
    void testCase1();
};

Deferedtask_testTest::Deferedtask_testTest()
{
}

void Deferedtask_testTest::testCase1()
{
    QVERIFY2(true, "Failure");
}

QTEST_MAIN(Deferedtask_testTest)

#include "tst_deferedtask_testtest.moc"
