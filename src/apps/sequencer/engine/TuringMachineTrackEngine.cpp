#include "TuringMachineTrackEngine.h"

#include "Engine.h"
#include "Slide.h"

#include "core/math/Math.h"

#include "model/Scale.h"

static uint16_t makeLoopMask(int loopStart, int loopLength) {
    if (loopLength >= 16) {
        return 0xffff;
    }
    return static_cast<uint16_t>(((1u << loopLength) - 1u) << loopStart);
}

static uint8_t tileBits(uint16_t loopBits, int loopLength) {
    if (loopLength >= 8) {
        return static_cast<uint8_t>((loopBits >> (loopLength - 8)) & 0xff);
    }
    uint8_t value = 0;
    int filled = 0;
    while (filled < 8) {
        int chunk = std::min(loopLength, 8 - filled);
        uint16_t mask = (1u << chunk) - 1u;
        value |= static_cast<uint8_t>((loopBits & mask) << filled);
        filled += chunk;
    }
    return value;
}

void TuringMachineTrackEngine::reset() {
    _currentStep = -1;
    _activity = false;
    _gateOutput = false;
    _cvOutput = 0.f;
    _cvOutputTarget = 0.f;

    changePattern();
    _initialRegister = _sequence->initialRegister();
    _liveRegister = _initialRegister;
}

void TuringMachineTrackEngine::restart() {
    _currentStep = -1;
    _liveRegister = _sequence->initialRegister();
}

TrackEngine::TickResult TuringMachineTrackEngine::tick(uint32_t tick) {
    syncRegisters();

    const auto *linkData = _linkedTrackEngine ? _linkedTrackEngine->linkData() : nullptr;

    uint32_t divisor = CONFIG_PPQN / CONFIG_SEQUENCE_PPQN;
    uint32_t relativeTick = tick;

    if (linkData) {
        _linkData = *linkData;
        divisor = linkData->divisor;
        relativeTick = linkData->relativeTick;
    } else {
        _linkData.divisor = divisor;
        _linkData.relativeTick = relativeTick;
        _linkData.sequenceState = nullptr;
    }

    if (relativeTick % divisor != 0) {
        return TickResult::NoUpdate;
    }

    int loopLength = _sequence->loopLength();
    int loopStart = _sequence->loopStart();
    uint16_t loopMask = makeLoopMask(loopStart, loopLength);
    uint16_t loopBits = (_liveRegister & loopMask) >> loopStart;

    bool feedbackBit = (loopBits >> (loopLength - 1)) & 1;
    if (_sequence->probability() > 0 && _rng.nextRange(100) < uint32_t(_sequence->probability())) {
        feedbackBit = !feedbackBit;
    }

    loopBits = (loopBits >> 1) | (feedbackBit ? (1u << (loopLength - 1)) : 0u);
    _liveRegister = (_liveRegister & ~loopMask) | (loopBits << loopStart);
    _sequence->setInitialRegister(_liveRegister);
    _initialRegister = _liveRegister;

    _currentStep = (_currentStep + 1) % loopLength;

    _gateOutput = evalGate(loopBits);
    _activity = _gateOutput;
    _cvOutputTarget = evalCv(loopBits);

    auto &midiOutputEngine = _engine.midiOutputEngine();
    bool outputGate = (!mute() || fill()) && _gateOutput;
    midiOutputEngine.sendGate(_track.trackIndex(), outputGate);
    if (outputGate || _sequence->gateMode() == 2) {
        midiOutputEngine.sendCv(_track.trackIndex(), _cvOutputTarget);
        midiOutputEngine.sendSlide(_track.trackIndex(), false);
    }

    return TickResult::GateUpdate | TickResult::CvUpdate;
}

void TuringMachineTrackEngine::update(float dt) {
    int slew = _sequence->slew();
    if (slew > 0) {
        _cvOutput = Slide::applySlide(_cvOutput, _cvOutputTarget, slew, dt);
    } else {
        _cvOutput = _cvOutputTarget;
    }
}

void TuringMachineTrackEngine::changePattern() {
    _sequence = &_track.turingMachineSequence();
}

void TuringMachineTrackEngine::syncRegisters() {
    uint16_t initialRegister = _sequence->initialRegister();
    if (initialRegister != _initialRegister) {
        _initialRegister = initialRegister;
        _liveRegister = initialRegister;
    }
}

bool TuringMachineTrackEngine::evalGate(uint16_t loopBits) {
    switch (_sequence->gateMode()) {
    case 0:
        return (loopBits & 0x1) != 0;
    case 1:
        return _rng.nextRange(100) < uint32_t(_sequence->probability());
    case 2:
        return true;
    default:
        return false;
    }
}

float TuringMachineTrackEngine::evalCv(uint16_t loopBits) const {
    int loopLength = _sequence->loopLength();
    uint8_t cvBits = tileBits(loopBits, loopLength);

    float cv = (float(cvBits) / 255.f) * float(_sequence->voltageRange());
    const auto &scale = _sequence->selectedScale();
    int root = _sequence->rootNote();
    if (scale.isChromatic()) {
        cv -= root * (1.f / 12.f);
    }
    float quantized = scale.noteToVolts(scale.noteFromVolts(cv));
    if (scale.isChromatic()) {
        quantized += root * (1.f / 12.f);
    }
    return quantized;
}
