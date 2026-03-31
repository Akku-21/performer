#pragma once

#include "TrackEngine.h"
#include "SequenceState.h"

#include "model/TuringMachineSequence.h"

#include "core/utils/Random.h"

class TuringMachineTrackEngine : public TrackEngine {
public:
    TuringMachineTrackEngine(Engine &engine, const Model &model, Track &track, const TrackEngine *linkedTrackEngine) :
        TrackEngine(engine, model, track, linkedTrackEngine),
        _sequence(&track.turingMachineSequence())
    {
        reset();
    }

    virtual Track::TrackMode trackMode() const override { return Track::TrackMode::TuringMachine; }

    virtual void reset() override;
    virtual void restart() override;
    virtual TickResult tick(uint32_t tick) override;
    virtual void update(float dt) override;

    virtual void changePattern() override;

    virtual const TrackLinkData *linkData() const override { return &_linkData; }

    virtual bool activity() const override { return _activity; }
    virtual bool gateOutput(int index) const override { return index == 0 ? _gateOutput : false; }
    virtual float cvOutput(int index) const override { return index == 0 ? _cvOutput : 0.f; }

    virtual float sequenceProgress() const override {
        return _currentStep < 0 ? 0.f : float(_currentStep) / float(_sequence->loopLength());
    }

    const TuringMachineSequence &sequence() const { return *_sequence; }
    bool isActiveSequence(const TuringMachineSequence &sequence) const { return &sequence == _sequence; }

    int currentStep() const { return _currentStep; }
    uint16_t registerValue() const { return _liveRegister; }
    void setRegister(uint16_t reg) {
        _liveRegister = reg;
        _initialRegister = reg;
    }

private:
    bool evalGate(uint16_t loopBits);
    float evalCv(uint16_t loopBits) const;
    void syncRegisters();

    TrackLinkData _linkData;

    TuringMachineSequence *_sequence;

    uint16_t _liveRegister = 0;
    uint16_t _initialRegister = 0;
    int _currentStep = -1;

    bool _activity = false;
    bool _gateOutput = false;
    float _cvOutput = 0.f;
    float _cvOutputTarget = 0.f;

    Random _rng;
};
