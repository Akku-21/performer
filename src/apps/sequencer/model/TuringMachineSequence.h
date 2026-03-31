#pragma once

#include "Config.h"
#include "Serialize.h"
#include "Scale.h"

#include "core/math/Math.h"

#include <cstdint>

class TuringMachineSequence {
public:
    static constexpr int RegisterBits = 16;

    static constexpr int InitialRegisterMin = 0;
    static constexpr int InitialRegisterMax = 0xffff;

    static constexpr int LoopLengthMin = 2;
    static constexpr int LoopLengthMax = 16;
    static constexpr int LoopLengthRange = LoopLengthMax - LoopLengthMin + 1;

    static constexpr int LoopStartMin = 0;
    static constexpr int LoopStartMax = RegisterBits - 1;
    static constexpr int LoopStartRange = LoopStartMax - LoopStartMin + 1;

    static constexpr int ProbabilityMin = 0;
    static constexpr int ProbabilityMax = 100;
    static constexpr int ProbabilityRange = ProbabilityMax - ProbabilityMin + 1;

    static constexpr int RootNoteMin = 0;
    static constexpr int RootNoteMax = 11;
    static constexpr int RootNoteRange = RootNoteMax - RootNoteMin + 1;

    static constexpr int VoltageRangeMin = 1;
    static constexpr int VoltageRangeMax = 4;
    static constexpr int VoltageRangeRange = VoltageRangeMax - VoltageRangeMin + 1;

    static constexpr int GateModeMin = 0;
    static constexpr int GateModeMax = 2;
    static constexpr int GateModeRange = GateModeMax - GateModeMin + 1;

    static constexpr int SlewMin = 0;
    static constexpr int SlewMax = 100;
    static constexpr int SlewRange = SlewMax - SlewMin + 1;

    uint16_t initialRegister() const { return _initialRegister; }
    void setInitialRegister(int initialRegister) {
        _initialRegister = clamp(initialRegister, InitialRegisterMin, InitialRegisterMax);
    }

    void editInitialRegister(int value, bool shift) {
        int step = shift ? 16 : 1;
        setInitialRegister(int(initialRegister()) + value * step);
    }

    int loopLength() const { return _loopLength; }
    void setLoopLength(int loopLength) {
        _loopLength = clamp(loopLength, LoopLengthMin, LoopLengthMax);
        _loopStart = clamp(int(_loopStart), LoopStartMin, maxLoopStart());
    }

    void editLoopLength(int value, bool shift) {
        int step = shift ? 4 : 1;
        setLoopLength(loopLength() + value * step);
    }

    int loopStart() const { return _loopStart; }
    void setLoopStart(int loopStart) {
        _loopStart = clamp(loopStart, LoopStartMin, maxLoopStart());
    }

    void editLoopStart(int value, bool shift) {
        int step = shift ? 4 : 1;
        setLoopStart(loopStart() + value * step);
    }

    int probability() const { return _probability; }
    void setProbability(int probability) {
        _probability = clamp(probability, ProbabilityMin, ProbabilityMax);
    }

    void editProbability(int value, bool shift) {
        int step = shift ? 10 : 1;
        setProbability(probability() + value * step);
    }

    int scale() const { return _scale; }
    void setScale(int scale) {
        _scale = clamp(scale, 0, Scale::Count - 1);
    }

    void editScale(int value, bool shift) {
        (void) shift;
        setScale(scale() + value);
    }

    const Scale &selectedScale() const {
        return Scale::get(scale());
    }

    int rootNote() const { return _rootNote; }
    void setRootNote(int rootNote) {
        _rootNote = clamp(rootNote, RootNoteMin, RootNoteMax);
    }

    void editRootNote(int value, bool shift) {
        (void) shift;
        setRootNote(rootNote() + value);
    }

    int voltageRange() const { return _voltageRange; }
    void setVoltageRange(int voltageRange) {
        _voltageRange = clamp(voltageRange, VoltageRangeMin, VoltageRangeMax);
    }

    void editVoltageRange(int value, bool shift) {
        (void) shift;
        setVoltageRange(voltageRange() + value);
    }

    int gateMode() const { return _gateMode; }
    void setGateMode(int gateMode) {
        _gateMode = clamp(gateMode, GateModeMin, GateModeMax);
    }

    void editGateMode(int value, bool shift) {
        (void) shift;
        setGateMode(gateMode() + value);
    }

    int slew() const { return _slew; }
    void setSlew(int slew) {
        _slew = clamp(slew, SlewMin, SlewMax);
    }

    void editSlew(int value, bool shift) {
        int step = shift ? 10 : 1;
        setSlew(slew() + value * step);
    }

    TuringMachineSequence() { clear(); }

    void clear();

    void write(VersionedSerializedWriter &writer) const;
    void read(VersionedSerializedReader &reader);

private:
    int maxLoopStart() const {
        return 16 - loopLength();
    }

    uint16_t _initialRegister = 0;
    uint8_t _loopLength = 16;
    uint8_t _loopStart = 0;
    uint8_t _probability = 0;
    uint8_t _scale = 0;
    uint8_t _rootNote = 0;
    uint8_t _voltageRange = 2;
    uint8_t _gateMode = 0;
    uint8_t _slew = 0;
};
