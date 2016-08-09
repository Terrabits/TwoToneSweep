#ifndef PROCESSTRACESTEST_H
#define PROCESSTRACESTEST_H


// Project
#include "IntermodTrace.h"

// RsaToolbox
#include <VnaTestClass.h>

// Qt
#include <QString>


class ProcessTracesTest : public RsaToolbox::VnaTestClass
{
    Q_OBJECT
public:
    ProcessTracesTest(RsaToolbox::ConnectionType type, const QString &address, QObject *parent = 0);
    ~ProcessTracesTest();

private slots:
    virtual void initTestCase();

    void preprocess_data();
    void preprocess     ();

    void calibration_data();
    void calibration     ();

    void run_data();
    void run     ();

    void traceMath();

private:
    void debugPrint(QString header, QList<IntermodTrace> traces);

    static QList<IntermodTrace> allTraces();
    double getMarkerValue(const QString prefix);
    bool isEqual(double left, double right);
};

#endif // PROCESSTRACESTEST_H
