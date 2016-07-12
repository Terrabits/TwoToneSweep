#ifndef INTERMODMEASUREMENTTEST_H
#define INTERMODMEASUREMENTTEST_H


// RsaToolbox
#include <GenericBus.h>
#include <VnaTestClass.h>

// Qt
#include <QObject>
#include <QString>


class IntermodMeasurementTest : public RsaToolbox::VnaTestClass
{
    Q_OBJECT
public:
    IntermodMeasurementTest(RsaToolbox::ConnectionType type, const QString &address, QObject *parent = 0);
   ~IntermodMeasurementTest();

private slots:
    void basic();

private:
    void testMatrix(RsaToolbox::ComplexMatrix2D &m, uint rows, uint columns);
};


#endif // INTERMODMEASUREMENTTEST_H
