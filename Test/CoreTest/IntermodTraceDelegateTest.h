#ifndef INTERMODTRACEDELEGATETEST_H
#define INTERMODTRACEDELEGATETEST_H


// Project
#include "IntermodSettings.h"

// RsaToolbox
#include <TestClass.h>

// Qt
#include <QObject>


class IntermodTraceDelegateTest : public RsaToolbox::TestClass
{
    Q_OBJECT
public:
    explicit IntermodTraceDelegateTest(QObject *parent = 0);
    ~IntermodTraceDelegateTest();

private slots:
    void set_data();
    void set     ();
};

#endif // INTERMODTRACEDELEGATETEST_H
