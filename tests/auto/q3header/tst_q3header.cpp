/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include "q3header.h"

//TESTED_CLASS=
//TESTED_FILES=

class tst_Q3Header : public QObject
{
    Q_OBJECT
public:
    tst_Q3Header();


public slots:
    void initTestCase();
    void cleanupTestCase();
private slots:
    void bug_setOffset();
    void nullStringLabel();

private:
    Q3Header *testW;
};

tst_Q3Header::tst_Q3Header()

{
}

void tst_Q3Header::initTestCase()
{
    // Create the test class
    testW = new Q3Header( 0, "testObject" );
}

void tst_Q3Header::cleanupTestCase()
{
    delete testW;
}

/*!  info/arc-15/30171 described a bug in setOffset(). Supposedly
  fixed in change 59949. Up to Qt 3.0.2 the horizontal size was used
  to determine whether scrolling was possible at all.  Could be merged
  into a general setOffset() test that goes through several variations
  of sizes and orientation.
*/
void tst_Q3Header::bug_setOffset()
{
    // create a vertical header which is wider than high.
    testW->setOrientation( Qt::Vertical );
    testW->addLabel( "111111111111111111111111111111111111" );
    testW->addLabel( "222222222222222222222222222222222222" );
    testW->addLabel( "333333333333333333333333333333333333" );
    testW->addLabel( "444444444444444444444444444444444444" );
    testW->setFixedSize( testW->headerWidth() * 2, testW->headerWidth() / 2 );

    // we'll try to scroll down a little bit
    int offs = testW->sectionSize( 0 ) / 2;
    testW->setOffset( offs );

    // and check whether we succeeded. In case the method used width()
    // for the visible header length offset() would be 0.
    QCOMPARE( testW->offset(), offs );
}

// Task 95640
void tst_Q3Header::nullStringLabel()
{
    QString oldLabel = testW->label(0);
    testW->setLabel(0, QString());
    QCOMPARE(testW->label(0), QString());
    testW->setLabel(0, oldLabel);
    QCOMPARE(testW->label(0), oldLabel);
    QCOMPARE(testW->label(testW->addLabel(QString())), QString());
    QCOMPARE(testW->label(testW->addLabel(QString("Foo"))), QString("Foo"));
    testW->removeLabel(testW->count()-1);
    QCOMPARE(testW->label(testW->addLabel(QString())), QString());
    QCOMPARE(testW->label(testW->addLabel(QString(""))), QString(""));
}

QTEST_MAIN(tst_Q3Header)
#include "tst_q3header.moc"

