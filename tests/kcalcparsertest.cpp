#include "kcalc_parser.h"
#include <iostream>
#include <QtTest>
#include <QSignalSpy>

class KCalcParserTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        parser = new KCalcParser;
        parser->setupDefaultNodesAndTokens(QStringLiteral("main"));
        parser->setActiveMode(QStringLiteral("main"));
    }

    void cleanupTestCase()
    {
        delete parser;
    }

    void findMismatchedBrackets_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<int>("expected");

        QTest::addRow("One open mismatched") << "(10+20" << 1;
        QTest::addRow("One closed mismatched") << "10+20)" << 1;
        QTest::addRow("One closed mismatched com") << "(10)+20)" << 1;
        QTest::addRow("One open mismatched com") << "((10)+20" << 1;

        QTest::addRow("two open mismatched") << "((10+20" << 2;
        QTest::addRow("two closed mismatched") << "10+20))" << 2;
        QTest::addRow("two closed mismatched com") << "(10)+20))" << 2;
        QTest::addRow("two open mismatched com") << "(((10)+20" << 2;
    }

    void findMismatchedBrackets()
    {
        QFETCH(QString, input);
        QFETCH(int, expected);

        QSignalSpy spy(parser, SIGNAL(foundMismatchedBrackets(int)));
        auto list = parser->tokenize(input, NB_HEX);
        auto result = parser->parseTokens(list);

        QCOMPARE(spy.count(), expected);
    }

    void invalidTokens_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QList<int>>("positions");

        QTest::addRow("1") << "xxx10+12" << QList<int> { 0, 1, 2 };
        QTest::addRow("2") << "10x+12" << QList<int> { 2 };
        QTest::addRow("3") << "sin(10)t" << QList<int> { 7 };
    }

    void invalidTokens()
    {

        QFETCH(QString, input);
        QFETCH(QList<int>, positions);

        QSignalSpy spy(parser, SIGNAL(foundInvalidTokens(int)));
        auto list = parser->tokenize(input, NB_HEX);
        auto result = parser->parseTokens(list);

        QCOMPARE(spy.count(), positions.size());

        for (int i = 0; i < spy.count(); ++i) {
            auto result = spy.at(i);
            QCOMPARE(result.size(), 1);
            QCOMPARE(result.at(0).type(), QVariant::Int);
            QCOMPARE(result.at(0).toInt(), positions.at(i));
        }
    }

    void evaluateExpression_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<int>("result");

        QTest::addRow("1") << "10 + func(5)" << 25;
        QTest::addRow("2") << "5 * 10 + func(5)" << 65;
        QTest::addRow("3") << "func(5)" << 15;
        QTest::addRow("4") << "(10 - 2) + (func 5)" << 23;
        QTest::addRow("5") << "10 + func(5*10)" << 70;
        QTest::addRow("6") << "func(5) * 10" << 150;
        QTest::addRow("7") << "func(10) / 2" << 10;
        QTest::addRow("8") << "cos(0)" << 1;
        QTest::addRow("9") << "func(45 * 2) / 2 / 2" << 25;
        QTest::addRow("10") << "100^2" << 10000;
    }

    void evaluateExpression()
    {
        QFETCH(QString, input);
        QFETCH(int, result);

        auto list = parser->tokenize(input, NB_HEX);
        auto parsed = parser->parseTokens(list);
        auto evaluated = static_cast<int>(parser->evaluateTokens(parsed).toInt64());

        QCOMPARE(evaluated, result);

        QBENCHMARK {
             parser->tokenize(input, NB_HEX);
             parser->parseTokens(list);
             parser->evaluateTokens(parsed).toInt64();
        }
    }

private:
    KCalcParser *parser;
};

QTEST_MAIN(KCalcParserTest)
#include "kcalcparsertest.moc"
