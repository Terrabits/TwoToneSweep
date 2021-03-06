#ifndef INTERMODTRACE_H
#define INTERMODTRACE_H


// RsaToolbox
#include <Definitions.h>

// Qt
#include <Qt>
#include <QDataStream>
#include <QString>


enum /*class*/ TraceType {
    inputTone,
    outputTone,
    intermod,
    relative,
    inputIntercept,
    outputIntercept
};
QString   toString   (TraceType type  );
TraceType toTraceType(QString   string);
Q_DECLARE_METATYPE(TraceType)


enum /*class*/ TraceFeature {
    lower,
    upper,
    major
};
QString      toString      (TraceFeature feature);
TraceFeature toTraceFeature(QString string);
Q_DECLARE_METATYPE(TraceFeature)

class IntermodTrace;
typedef QList<IntermodTrace> IntermodTraces;

class IntermodTrace
{
public:
    IntermodTrace();
    IntermodTrace(TraceType type, TraceFeature feature, uint order = 1);
    IntermodTrace(QString display);
   ~IntermodTrace();

    // Visible
    bool isVisible () const;
    void hide      ();
    void show      ();
    void setVisible(bool isVisible);

    // Dependenent calculation
    bool                 isDependent() const;
    QList<IntermodTrace> dependents () const;


    // is type
    bool isInputTone () const;
    bool isOutputTone() const;
    bool isIntermod  () const;
    bool isRelative  () const;
    bool isIntercept () const;
    bool isOutputIntercept() const;
    bool isInputIntercept () const;

    // is feature
    bool isLower() const;
    bool isUpper() const;
    bool isMajor() const;

    // is order
    bool hasOrder () const;
    bool isThird  () const;
    bool isFifth  () const;
    bool isSeventh() const;
    bool isNinth  () const;

    TraceType type      () const;
    TraceFeature feature() const;
    uint      order     () const;
    void      setType   (TraceType type );
    void      setFeature(TraceFeature feature);
    void      setOrder  (uint n         );

    QString   display   () const;
    QString   abbreviate() const;

private:
    TraceType _type;
    TraceFeature   _feature;
    uint      _order;
    bool      _isVisible;

    QList<IntermodTrace> intermodDependents () const;
    QList<IntermodTrace> relativeDependents () const;
    QList<IntermodTrace> interceptDependents() const;

    QString typeString()    const;
    QString featureString() const;
    QString orderString()   const;

    QString abbreviateFeature() const;
};
Q_DECLARE_METATYPE(IntermodTrace)

QDataStream &operator<<(QDataStream &stream, const IntermodTrace &trace);
QDataStream &operator>>(QDataStream &stream, IntermodTrace &trace);

bool operator==(const IntermodTrace &left, const IntermodTrace &right);
bool operator!=(const IntermodTrace &left, const IntermodTrace &right);

// for sorting according to
// order of processing
//
// Order by traits:
//   type < format < order
//
// Type sort:
//   input < output
//         < intermod
//         < relative
//         < intercept
//
// Feature sort:
//   lower < upper < major
//
// Sort order as you'd expect
//
// Example:
// | Type             | Feature | Order |
// |------------------|---------|-------|
// | input            | lower   | N/A   |
// | input            | upper   | N/A   |
// | output           | lower   | N/A   |
// | output           | upper   | N/A   |
// | intermod         | lower   | 3     |
// | intermod         | upper   | 3     |
// | intermod         | major   | 3     |
// | intermod         | lower   | 5     |
// | intermod         | ...     | ...   |
// | relative         | lower   | 3     |
// | relative         | upper   | 3     |
// | relative         | major   | 3     |
// | relative         | lower   | 5     |
// | relative         | ...     | ...   |
// | input  intercept | lower   | 3     |
// | input  intercept | upper   | 3     |
// | input  intercept | major   | 3     |
// | input  intercept | lower   | 5     |
// | input  intercept | ...     | ...   |
// | output intercept | lower   | 3     |
// | output intercept | upper   | 3     |
// | output intercept | major   | 3     |
// | output intercept | lower   | 5     |
// | output intercept | ...     | ...   |
//
bool operator< (const IntermodTrace &left, const IntermodTrace &right);
bool operator<=(const IntermodTrace &left, const IntermodTrace &right);
bool operator> (const IntermodTrace &left, const IntermodTrace &right);
bool operator>=(const IntermodTrace &left, const IntermodTrace &right);

#endif // INTERMODTRACE_H
