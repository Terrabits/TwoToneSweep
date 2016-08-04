#include "IntermodTrace.h"


// RsaToolbox
using namespace RsaToolbox;

// Qt
#include <QVariant>


QString toString(TraceType type) {
    switch (type) {
    case TraceType::inputTone:
        return "Input";
    case TraceType::outputTone:
        return "Output";
    case TraceType::intermod:
        return "Intermod";
    case TraceType::relative:
        return "Relative";
    case TraceType::intercept:
        return "Intercept";
    default:
        return "Input";
    }
}
QString toString(Feature feature) {
    switch (feature) {
    case Feature::upper:
        return "Upper";
    case Feature::lower:
        return "Lower";
    case Feature::major:
        return "Major";
    default:
        return "Lower";
    }
}

//const QRegExp IntermodTrace::NAME_REGEX("^[a-z_][0-9a-z_]*$", Qt::CaseInsensitive);

IntermodTrace::IntermodTrace() :
    _type   (TraceType::inputTone),
    _feature(Feature::lower      ),
    _order  (1 ),
    _isVisible(true)
{
    //
}
IntermodTrace::IntermodTrace(TraceType type,
                             Feature feature,
                             uint order) :
    _type   (type   ),
    _feature(feature),
    _order  (order  ),
    _isVisible(true)
{

}
IntermodTrace::IntermodTrace(QString s) {
    _isVisible = true;

    s = s.toLower();

    // Type
    const QString input     = toString(TraceType::inputTone ).toLower();
    const QString output    = toString(TraceType::outputTone).toLower();
    const QString intermod  = toString(TraceType::intermod  ).toLower();
    const QString relative  = toString(TraceType::relative  ).toLower();
    const QString intercept = toString(TraceType::intercept ).toLower();
    if (s.contains(input)) {
        _type = TraceType::inputTone;
    }
    else if (s.contains(output)) {
        _type = TraceType::outputTone;
    }
    else if (s.contains(intermod)) {
        _type = TraceType::intermod;
    }
    else if (s.contains(relative)) {
        _type = TraceType::relative;
    }
    else if (s.contains(intercept)) {
        _type = TraceType::intercept;
    }
    else {
        // default type
        _type = TraceType::inputTone;
    }

    // Feature
    const QString upper = toString(Feature::upper).toLower();
    const QString lower = toString(Feature::lower).toLower();
    const QString major = toString(Feature::major).toLower();
    if      (s.contains(upper)) {
        _feature = Feature::upper;
    }
    else if (s.contains(lower)) {
        _feature = Feature::lower;
    }
    else if (s.contains(major)) {
        _feature = Feature::major;
    }
    else {
        // default feature
        _feature = Feature::lower;
    }

    // Order
    const QString third   = "3rd";
    const QString fifth   = "5th";
    const QString seventh = "7th";
    const QString ninth   = "9th";
    if      (s.contains(third  )) {
        _order = 3;
    }
    else if (s.contains(fifth  )) {
        _order = 5;
    }
    else if (s.contains(seventh)) {
        _order = 7;
    }
    else if (s.contains(ninth  )) {
        _order = 9;
    }
    else {
        // default
        const bool isTone = _type == TraceType::inputTone
                         || _type == TraceType::outputTone;
        if (isTone) {
            _order = 1;
        }
        else {
            _order = 3;
        }
    }
}

IntermodTrace::~IntermodTrace()
{
    //
}

bool IntermodTrace::isVisible() const {
    return _isVisible;
}
void IntermodTrace::hide     () {
    _isVisible = false;
}
void IntermodTrace::show     () {
    _isVisible = true;
}

// isType
bool IntermodTrace::isInputTone() const {
    return _type == TraceType::inputTone;
}
bool IntermodTrace::isOutputTone() const {
    return _type == TraceType::outputTone;
}
bool IntermodTrace::isIntermod() const {
    return _type == TraceType::intermod;
}
bool IntermodTrace::isRelative() const {
    return _type == TraceType::relative;
}
bool IntermodTrace::isIntercept() const {
    return _type == TraceType::intercept;
}

// isFeature
bool IntermodTrace::isLower() const {
    return _feature == Feature::lower;
}
bool IntermodTrace::isUpper() const {
    return _feature == Feature::upper;
}
bool IntermodTrace::isMajor() const {
    return _feature == Feature::major;
}

// isOrder
bool IntermodTrace::hasOrder () const {
    if (isInputTone ())
        return false;
    if (isOutputTone())
        return false;

    // else:
    //   intermod
    //   relative intermod
    //   intercept
    return true;
}
bool IntermodTrace::isThird  () const {
    if (!hasOrder())
        return false;

    return _order == 3;
}
bool IntermodTrace::isFifth  () const {
    if (!hasOrder())
        return false;

    return _order == 5;
}
bool IntermodTrace::isSeventh() const {
    if (!hasOrder())
        return false;

    return _order == 7;
}
bool IntermodTrace::isNinth  () const {
    if (!hasOrder())
        return false;

    return _order == 9;
}

// get
TraceType IntermodTrace::type   () const {
    return _type;
}
Feature   IntermodTrace::feature() const {
    return _feature;
}
uint      IntermodTrace::order  () const {
    return _order;
}

// set
void IntermodTrace::setType   (TraceType type)  {
    _type = type;
}
void IntermodTrace::setFeature(Feature feature) {
    _feature = feature;
}
void IntermodTrace::setOrder  (uint n)          {
    _order = n;
}

QString IntermodTrace::display()       const {
    QString result;
    switch (_type) {
    case TraceType::inputTone:
    case TraceType::outputTone:
        result = "%1 %2";
        result = result.arg(featureString());
        result = result.arg(typeString   ());
        return result;
    case TraceType::intermod:
    case TraceType::relative:
    case TraceType::intercept:
        result = "%1 %2 %3";
        result = result.arg(orderString  ());
        result = result.arg(featureString());
        result = result.arg(typeString   ());
        return result;
    default:
        return "Lower Input";
    }
}
QString IntermodTrace::abbreviate()    const {
    QString result;
    switch (_type) {
    case TraceType::inputTone:
        result = "%1ti";
        result = result.arg(abbreviateFeature());
        return result;
    case TraceType::outputTone:
        result = "%1to";
        result = result.arg(abbreviateFeature());
        return result;
    case TraceType::intermod:
        result = "im%1%2o";
        result = result.arg(_order);
        result = result.arg(abbreviateFeature());
        return result;
    case TraceType::relative:
        result = "im%1%2or";
        result = result.arg(_order);
        result = result.arg(abbreviateFeature());
    case TraceType::intercept:
        result = "ip%1%2o";
        result = result.arg(_order);
        result = result.arg(abbreviateFeature());
        return result;
    default:
        return "lti";
    }
}

QString IntermodTrace::typeString()    const {
    return toString(_type);
}
QString IntermodTrace::featureString() const {
    return toString(_feature);
}
QString IntermodTrace::orderString()   const {
    if (_order == 1)
        return "1st";
    if (_order == 2)
        return "2nd";
    if (_order == 3)
        return "3rd";

    return QString("%1th").arg(_order);
}

QString IntermodTrace::abbreviateFeature() const {
    switch (_feature) {
    case Feature::lower:
        return "l";
    case Feature::upper:
        return "u";
    case Feature::major:
        return "m";
    default:
        return "l";
    }
}

// Operators
bool operator==(const IntermodTrace &left, const IntermodTrace &right) {
    // not equal?
    if (left.type() != right.type())
        return false;
    if (left.feature() != right.feature())
        return false;
    if (left.hasOrder()) {
        if (left.order() != right.order())
            return false;
    }

    // equal!
    return true;
}
bool operator!=(const IntermodTrace &left, const IntermodTrace &right) {
    return !(left == right);
}

// Sort
bool operator< (const IntermodTrace &left, const IntermodTrace &right) {
    if (left.type() < right.type())
        return true;
    if (left.type() != right.type())
        return false;

    // ... same type

    if (left.feature() < right.feature())
        return true;
    if (left.feature() != right.feature())
        return false;

    // .. same feature

    if (!left.hasOrder()) {
        // No more properties
        // to compare.
        // They are equal
        return false;
    }

    // last property to compare
    return left.order() < right.order();
}
bool operator<=(const IntermodTrace &left, const IntermodTrace &right) {
    return  (left < right) || (left == right);
}
bool operator> (const IntermodTrace &left, const IntermodTrace &right) {
    return !(left <= right);
}
bool operator>=(const IntermodTrace &left, const IntermodTrace &right) {
    return !(left <  right);
}
