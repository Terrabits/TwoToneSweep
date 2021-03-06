#include "ProcessTraces.h"


// RsaToolbox
#include "General.h"
#include <VnaChannel.h>
#include <VnaTrace.h>
using namespace RsaToolbox;

// Qt
#include <QDebug>


ProcessTraces::ProcessTraces(const QList<IntermodTrace> &traces,
                             const IntermodSettings &settings,
                             RsaToolbox::Vna *vna)
    : _traces(traces),
      _settings(settings),
      _vna(vna),
      _channels(vna, _settings.channel()),
      _genFreq(settings)
{
    preprocessTraces();
}

ProcessTraces::~ProcessTraces()
{

}

bool ProcessTraces::isReady(IntermodError &error) {
    error.clear();

    const uint vnaPorts = _vna->testPorts();

    // Lower port
    if (lowerPort() == 0 || lowerPort() > vnaPorts) {
        error.code    = IntermodError::Code::LowerSourcePort;
        error.message = "*lower input port is invalid";
        return false;
    }

    // Upper port
    if (isUpperSourceGenerator()) {
        if (!_vna->generators().contains(upperGenerator())) {
            error.code    = IntermodError::Code::UpperSource;
            error.message = "*upper generator is invalid";
            return false;
        }
    }
    else {
        if (upperPort() == 0 || upperPort() > vnaPorts) {
            error.code    = IntermodError::Code::UpperSource;
            error.message = "*upper port is invalid";
            return false;
        }
    }

    // Combiner
    if (!isExternalCombiner()) {
        if (combinerPort() == 0 || combinerPort() > vnaPorts) {
            error.code    = IntermodError::Code::Combiner;
            error.message = "*Combiner port is invalid";
            return false;
        }
    }

    // Receiving port
    if (outputPort() == 0 || outputPort() > vnaPorts) {
        error.code    = IntermodError::Code::ReceivingPort;
        error.message = "*receiving port is invalid";
        return false;
    }

    // Port overlap
    if (lowerPort() == outputPort()) {
        error.code    = IntermodError::Code::LowerSourcePort;
        error.message = "*port assignments overlap";
        return false;
    }
    if (!isUpperSourceGenerator()) {
        if (lowerPort() == upperPort()) {
            error.code    = IntermodError::Code::UpperSource;
            error.message = "*port assignments overlap";
            return false;
        }
        if (upperPort() == outputPort()) {
            error.code    = IntermodError::Code::UpperSource;
            error.message = "*port assignments overlap";
            return false;
        }
    }

    // Center frequency
    const double center_Hz = _settings.centerFrequency_Hz();
    const double min_Hz    = _vna->properties().minimumFrequency_Hz();
    if (center_Hz < min_Hz) {
        error.code    = IntermodError::Code::CenterFrequency;
        error.message = "*center frequency must be at least %1";
        error.message = error.message.arg(min_Hz);
        return false;
    }
    const double max_Hz    = _vna->properties().maximumFrequency_Hz();
    if (center_Hz > max_Hz) {
        error.code    = IntermodError::Code::CenterFrequency;
        error.message = "*center frequency must be at most %1";
        error.message = error.message.arg(max_Hz);
        return false;
    }

    // Tone distance
    const double startDist_Hz = _settings.startToneDistance_Hz();
    const double stopDist_Hz  = _settings.stopToneDistance_Hz ();
    if (startDist_Hz <= 0) {
        error.code    = IntermodError::Code::StartToneDistance;
        error.message = "*start tone distance must be greater than 0";
        return false;
    }
    if (stopDist_Hz <= 0) {
        error.code    = IntermodError::Code::StopToneDistance;
        error.message = "*stop tone distance must be greater than 0";
        return false;
    }
    if (startDist_Hz >= stopDist_Hz) {
        error.code    = IntermodError::Code::StartToneDistance;
        error.message = "*start tone distance must be greater than stop";
        return false;
    }
    if (center_Hz - 0.5 * stopDist_Hz < min_Hz) {
        error.code    = IntermodError::Code::StopToneDistance;
        error.message = "*tone distance too wide for VNA";
        return false;
    }
    if (center_Hz + 0.5 * stopDist_Hz > max_Hz) {
        error.code    = IntermodError::Code::StopToneDistance;
        error.message = "*tone distance too wide for VNA";
        return false;
    }

    // Points
    const uint points = _settings.points();
    if (points == 0) {
        error.code    = IntermodError::Code::Points;
        error.message = "*points must be greater than 0";
        return false;
    }
    const uint maxPoints = _vna->properties().maximumPoints();
    if (points > maxPoints) {
        error.code    = IntermodError::Code::Points;
        error.message = "*points must be at most %1";
        error.message = error.message.arg(maxPoints);
        return false;
    }

    // Power
    const double min_dBm   = _vna->properties().minimumPower_dBm();
    const double power_dBm = _settings.power_dBm();
    if (power_dBm < min_dBm) {
        error.code    = IntermodError::Code::Power;
        error.message = "*power must be at least %1";
        error.message = error.message.arg(min_dBm);
        return false;
    }
    const double max_dBm = _vna->properties().maximumPower_dBm();
    if (power_dBm > max_dBm) {
        error.code    = IntermodError::Code::Power;
        error.message = "*power must be at most %1";
        error.message = error.message.arg(max_dBm);
        return false;
    }

    // IF BW
    const double ifBw_Hz  = _settings.ifBw_Hz();
    const double newIfBw_Hz = findClosest(ifBw_Hz, _vna->properties().ifBandwidthValues_Hz());
    if (newIfBw_Hz != ifBw_Hz) {
        // if value rounded
        error.code    = IntermodError::Code::IfBw;
        error.message = "*IF BW value rounded to %1";
        error.message = error.message.arg(newIfBw_Hz);
        return false;
    }

    // Channel
    const uint c = _settings.channel();
    if (!_vna->channels().contains(c)) {
        error.code    = IntermodError::Code::Channel;
        error.message = "*Could not find channel %1";
        error.message = error.message.arg(c);
        return false;
    }

    // Traces
    if (_traces.isEmpty()) {
        error.code    = IntermodError::Code::Traces;
        error.message = "*Enter at least one trace";
        return false;
    }
    if (isUpperSourceGenerator() && hasUpperInputTrace()) {
        error.code    = IntermodError::Code::Traces;
        error.message = "*Cannot show upper input when using a generator";
        return false;
    }

    // Frequency range of traces
    for (int i = 0; i < _traces.size(); i++) {
        const IntermodTrace t = _traces[i];
        if (isFreqOutsideVna(t)) {
            error.code    = IntermodError::Code::Order;
            error.message = "*%1 outside VNA frequency range";
            error.message = error.message.arg(t.display());
            return false;
        }
    }

    return true;
}
uint ProcessTraces::lastChannel() const {
    return _channels.last();
}
void ProcessTraces::setupCalibration() {
//    _channels.collapse();
    VnaChannel c = _vna->channel(_channels.base());

    VnaLinearSweep swp = c.linearSweep();
    swp.setIfbandwidth(_settings.ifBw_Hz    ());
    swp.setPower      (_settings.power_dBm  ());
    c.setIfSelectivity(_settings.selectivity());

    c.port(lowerPort()).rfOff(false);
    c.port(lowerPort()).setGenerator(false);
    c.port(lowerPort()).arbitrarySourceFrequencyOff();
    if (isUpperSourceGenerator()) {
        c.generator(upperGenerator()).rfOff(false);
        c.generator(upperGenerator()).setPermanentlyOn(false);
        c.generator(upperGenerator()).arbitraryFrequencyOff();
    }
    else {
        c.port(upperPort()).rfOff(false);
        c.port(upperPort()).setGenerator(false);
        c.port(upperPort()).arbitrarySourceFrequencyOff();
    }
    if (_vna->properties().isZvaFamily())
        c.arbitraryReceiverFrequencyOff();

    c.setFrequencies(calFreq_Hz());
    c.select();
}
void ProcessTraces::run() {
    bool isOn = _vna->settings().isDisplayOn();
    _vna->settings().displayOff();

    _channels.collapse();
    _diagram = createOrReuseDiagram();
    uint numTraces = 0;
    for (int i = 0; i < _traces.size(); i++) {
        const IntermodTrace t = _traces[i];
        if (t.isVisible()) {
            if (numTraces >= 20) {
                numTraces = 0;
                _diagram = _vna->createDiagram();
            }
            numTraces++;
        }

        processTrace(_traces[i]);

        _vna->settings().displayOn(isOn);
    }
}

// isReady
bool ProcessTraces::isUpperSourceGenerator() const {
    return _settings.upperSource().isGenerator();
}
bool ProcessTraces::hasUpperInputTrace() const {
    foreach (IntermodTrace t, _traces) {
        if (t.isInputTone() && t.isUpper())
            return true;
    }

    return false;
}
bool ProcessTraces::isFreqOutsideVna(const IntermodTrace &t) const {
    const double vnaMin = _vna->properties().minimumFrequency_Hz();
    const double vnaMax = _vna->properties().maximumFrequency_Hz();
    if (t.isLower() || t.isMajor()) {
        if (_genFreq.minLowerFreq_Hz(t.order()) < vnaMin)
            return true;
    }
    else if (t.isUpper() || t.isMajor()) {
        if (_genFreq.maxUpperFreq_Hz(t.order()) > vnaMax)
            return true;
    }

    // Else
    return false;
}

// Preprocess
void ProcessTraces::preprocessTraces() {
    removeDuplicateTraces();
    foreach (const IntermodTrace t, _traces) {
        preprocessTrace(t);
    }
    sort();
}
void ProcessTraces::removeDuplicateTraces() {
    sort();
    int i = 0;
    while (i < _traces.size() - 1) {
        if (_traces[i] == _traces[i+1])
            _traces.removeAt(i+1);
        else
            i++;
    }
}

void ProcessTraces::preprocessTrace(const IntermodTrace &t) {
    if (!dependencyInTraces(t)) {
        QList<IntermodTrace> deps = t.dependents();
        foreach (IntermodTrace d, deps) {
            if (!_traces.contains(d)) {
                d.hide();
                _traces << d;
                preprocessTrace(d);
            }
        }
    }
}

bool ProcessTraces::dependencyInTraces(const IntermodTrace &t) const {
    if (!t.isDependent())
        return true;

    const QList<IntermodTrace> dependents = t.dependents();
    foreach (const IntermodTrace d, dependents) {
        if (!_traces.contains(d)) {
            return false;
        }
    }

    // Else
    return true;
}
void ProcessTraces::sort() {
    std::sort(_traces.begin(), _traces.end());
}

void ProcessTraces::clearPortAndGeneratorSettings() {
    VnaChannel c = _vna->channel(_settings.channel());
    const uint numPorts = _vna->properties().physicalPorts();
    for (uint i = 1; i <= numPorts; i++) {
        VnaPortSettings p = c.port(i);
        p.rfOff(false);
        p.setGenerator(false);
        p.arbitrarySourceFrequencyOff();
    }
    foreach (uint g, _vna->generators()) {
        c.generator(g).rfOff(false);
        c.generator(g).setPermanentlyOn(false);
        c.generator(g).arbitraryFrequencyOff();
    }
}

void ProcessTraces::processTrace(const IntermodTrace &t) {
    switch (t.type()) {
    case TraceType::inputTone:
        processInputTrace(t);
        return;
    case TraceType::outputTone:
        processOutputTrace(t);
        return;
    case TraceType::intermod:
        processIntermodTrace(t);
        return;
    case TraceType::relative:
        processRelativeTrace(t);
        return;
    case TraceType::inputIntercept:
    case TraceType::outputIntercept:
        processInterceptTrace(t);
        return;
    default:
        return;
    }
}

void ProcessTraces::configureChannel(VnaChannel c) {
    // Sweep setup
    c.setSweepType(VnaChannel::SweepType::Linear);

    VnaLinearSweep swp = c.linearSweep();
    swp.setStart (_genFreq.fbStart_Hz());
    swp.setStop  (_genFreq.fbStop_Hz ());
    swp.setPoints(_settings.points());

    swp.setIfbandwidth(_settings.ifBw_Hz    ());
    swp.setPower      (_settings.power_dBm  ());
    c.setIfSelectivity(_settings.selectivity());

    // Port setup
    c.port(lowerPort()).rfOff(false);
    c.port(lowerPort()).setGenerator(false);
    c.port(lowerPort()).setArbitrarySourceFrequency(lowerAf());

    if (_settings.upperSource().isPort()) {
        c.port(upperPort()).rfOff(false);
        c.port(upperPort()).setGenerator(true);
        c.port(upperPort()).setArbitrarySourceFrequency(upperAf());
    }
    else {
        const uint g = _settings.upperSource().generator();
        c.generator(g).rfOff(false);
        c.generator(g).setPermanentlyOn(true);
        c.generator(g).setArbitraryFrequency(upperAf());
    }
}

void ProcessTraces::processInputTrace    (const IntermodTrace &t) {
    // Channel
    VnaChannel ch = _channels.create(t);
    configureChannel(ch);

    // Receiver frequency is
    // independent from source settings
    // on ZVA
    if (_vna->properties().isZvaFamily())
        ch.setArbitraryReceiverFrequency(receiverAf(t));

    // Wave port
    uint wavePort;
    if (_settings.combiner().isPort()) {
        wavePort = _settings.combiner().port();
    }
    // External combiner:
    else if (t.isLower()) {
        wavePort = lowerPort();
    }
    // Upper trace
    else if (_settings.upperSource().isPort()) {
        wavePort = upperPort();
    }
    else {
        // Cannot display upper source
        // when it is a generator!
        wavePort = 0;
    }

    // Trace
    const QString name = traceName(t);
    _vna->createTrace(traceName(t), ch.index());
    VnaTrace vnaTrc = _vna->trace(name);
    vnaTrc.setWaveQuantity(WaveQuantity::a, wavePort, lowerPort());
    if (t.isVisible())
        vnaTrc.setDiagram(_diagram);
}
void ProcessTraces::processOutputTrace   (const IntermodTrace &t) {
    // Channel
    VnaChannel ch = _channels.create(t);
    configureChannel(ch);

    // Output port frequency
    if (_vna->properties().isZvaFamily())
        ch.setArbitraryReceiverFrequency(receiverAf(t));
    else
        ch.port(outputPort()).setArbitrarySourceFrequency(receiverAf(t));

    // Trace
    const QString name = traceName(t);
    _vna->createTrace(name, ch.index());
    VnaTrace vnaTrc = _vna->trace(name);
    vnaTrc.setWaveQuantity(WaveQuantity::b, outputPort(), lowerPort());
    if (t.isVisible())
        vnaTrc.setDiagram(_diagram);
}
void ProcessTraces::processIntermodTrace (const IntermodTrace &t) {
    VnaChannel ch;
    if (!t.isDependent()) {
        ch = _channels.create(t);
        configureChannel(ch);

        // Output port frequency
        if (_vna->properties().isZvaFamily())
            ch.setArbitraryReceiverFrequency(receiverAf(t));
        else
            ch.port(outputPort()).setArbitrarySourceFrequency(receiverAf(t));
    }
    else {
        ch = _vna->channel(lastChannel());
    }

    // Trace
    const QString name = traceName(t);
    _vna->createTrace(name, ch.index());
    VnaTrace vnaTrc = _vna->trace(name);
    vnaTrc.setWaveQuantity(WaveQuantity::b, outputPort(), lowerPort());
    if (t.isMajor()) {
        vnaTrc.math().setExpression(math(t));
        vnaTrc.math().on();
        vnaTrc.math().setWaveQuantity();
    }
    if (t.isVisible())
        vnaTrc.setDiagram(_diagram);
}
void ProcessTraces::processRelativeTrace (const IntermodTrace &t) {
    // Trace
    const QString name = traceName(t);
    _vna->createTrace(name, lastChannel());
    VnaTrace vnaTrc = _vna->trace(name);
    vnaTrc.setWaveRatio(WaveQuantity::b, outputPort(), lowerPort(),  // num
                        WaveQuantity::b, outputPort(), lowerPort()); // den
    vnaTrc.math().setExpression(math(t));
    vnaTrc.math().on();
    if (t.isVisible())
        vnaTrc.setDiagram(_diagram);
}
void ProcessTraces::processInterceptTrace(const IntermodTrace &t) {
    // Trace
    const QString name = traceName(t);
    _vna->createTrace(name, lastChannel());
    VnaTrace vnaTrc = _vna->trace(name);
    vnaTrc.setWaveQuantity(WaveQuantity::b, outputPort(), lowerPort());
    vnaTrc.math().setExpression(math(t));
    vnaTrc.math().on();
    vnaTrc.math().setWaveQuantity();
    if (t.isVisible())
        vnaTrc.setDiagram(_diagram);
}

// Helpers
QString ProcessTraces::traceName(const IntermodTrace &t) const {
    QString name = "%1_im_ch%2";
    name  = name.arg(t.abbreviate  ());
    name  = name.arg(_channels.base());
    return  name;
}
uint ProcessTraces::createOrReuseDiagram() {
    if (_vna->traces().isEmpty()) {
        _vna->createDiagram(1);
        return 1;
    }
    else {
        return _vna->createDiagram();
    }
}

uint ProcessTraces::lowerPort() const {
    return _settings.lowerSourcePort();
}
uint ProcessTraces::upperPort() const {
    return _settings.upperSource().port();
}
uint ProcessTraces::upperGenerator() const {
    return _settings.upperSource().generator();
}
bool ProcessTraces::isExternalCombiner() const {
    return _settings.combiner().isExternal();
}
uint ProcessTraces::combinerPort() const {
    return _settings.combiner().port();
}
uint ProcessTraces::outputPort() const {
    return _settings.receivingPort();
}
VnaArbitraryFrequency ProcessTraces::lowerAf() const {
    return _genFreq.lowerInput();
}
VnaArbitraryFrequency ProcessTraces::upperAf() const {
    return _genFreq.upperInput();
}
VnaArbitraryFrequency ProcessTraces::receiverAf(const IntermodTrace &t) const {
    if (t.isLower())
        return _genFreq.lowerOutput(t.order());
    if (t.isUpper())
        return _genFreq.upperOutput(t.order());

    // Else (doesn't matter)
    return VnaArbitraryFrequency();
}
QString ProcessTraces::math(const IntermodTrace &t) const {
    if (t.isIntermod())
        return intermodMath (t);
    if (t.isRelative())
        return relativeMath (t);
    if (t.isIntercept())
        return interceptMath(t);

    // method shouldn't've been called
    return QString();
}
QString ProcessTraces::intermodMath(const IntermodTrace &t) const {
    if (!t.isMajor())
        return QString();

    IntermodTrace iml(t);
    iml.setFeature(TraceFeature::lower);

    IntermodTrace imu(t);
    imu.setFeature(TraceFeature::upper);

    QString math;
    math = "Max(linMag(%1),linMag(%2))";
    math = math.arg(traceName(iml));
    math = math.arg(traceName(imu));
    return math;
}
QString ProcessTraces::relativeMath(const IntermodTrace &t) const {
    IntermodTrace lto(TraceType::outputTone, TraceFeature::lower           );
    IntermodTrace iml(TraceType::intermod,   TraceFeature::lower, t.order());
    IntermodTrace imu(TraceType::intermod,   TraceFeature::upper, t.order());

    QString imExpr;
    if (t.isLower()) {
        imExpr = "linMag(%1)";
        imExpr = imExpr.arg(traceName(iml));
    }
    else if (t.isUpper()) {
        imExpr = "linMag(%1)";
        imExpr = imExpr.arg(traceName(imu));
    }
    else { // major
        imExpr = "Max(linMag(%1),linMag(%2))";
        imExpr = imExpr.arg(traceName(iml));
        imExpr = imExpr.arg(traceName(imu));
    }

    QString math;
    math = "linMag(%1)/%2";
    math = math.arg(traceName(lto));
    math = math.arg(imExpr);
    return math;
}
QString ProcessTraces::interceptMath(const IntermodTrace &t) const {
    IntermodTrace tr(TraceType::relative, t.feature(), t.order());
    QString relExpr;
    relExpr = "((%1)^%2)";
    relExpr = relExpr.arg(math(tr));
    relExpr = relExpr.arg(1.0/(t.order()-1.0));

    IntermodTrace lt;
    if (t.isInputIntercept()) {
        lt.setType   (TraceType::inputTone);
        lt.setFeature(TraceFeature::lower );
    }
    else {
        lt.setType   (TraceType::outputTone);
        lt.setFeature(TraceFeature::lower  );
    }

    QString math;
    math = "linMag(%1)*%2";
    math = math.arg(traceName(lt));
    math = math.arg(relExpr);
    return math;
}

QRowVector ProcessTraces::fb_Hz() const {
    return _genFreq.fb_Hz();
}
QRowVector ProcessTraces::upperFreq_Hz() const {
    return _genFreq.upperInputFreq_Hz();
}
QRowVector ProcessTraces::outputFreq_Hz(const IntermodTrace &t) const {
    if (t.isLower())
        return _genFreq.lowerOutputFreq_Hz(t.order());
    else
        return _genFreq.upperOutputFreq_Hz(t.order());
}
QRowVector ProcessTraces::calFreq_Hz() const {
    // Original tones
    QRowVector f;
    f << fb_Hz();
    f << upperFreq_Hz();

    // Harmonics
    foreach (IntermodTrace t, _traces) {
        // Skip originals
        if (t.isInputTone())
            continue;
        if (t.isOutputTone())
            continue;

        // Skip math traces
        if (t.isDependent())
            continue;

        includeOneOf(f, outputFreq_Hz(t));
    }
    std::sort(f.begin(), f.end());
    return f;
}
void ProcessTraces::includeOneOf(QRowVector &vector, double value) {
    if (!vector.contains(value))
        vector << value;
}
void ProcessTraces::includeOneOf(QRowVector &vector, const QRowVector &values) {
    foreach (const double v, values) {
        includeOneOf(vector, v);
    }
}
