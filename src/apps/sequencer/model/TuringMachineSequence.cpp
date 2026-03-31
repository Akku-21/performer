#include "TuringMachineSequence.h"

void TuringMachineSequence::clear() {
    setInitialRegister(0);
    setLoopLength(16);
    setLoopStart(0);
    setProbability(0);
    setScale(0);
    setRootNote(0);
    setVoltageRange(2);
    setGateMode(0);
    setSlew(0);
}

void TuringMachineSequence::write(VersionedSerializedWriter &writer) const {
    writer.write(_initialRegister);
    writer.write(_loopLength);
    writer.write(_loopStart);
    writer.write(_probability);
    writer.write(_scale);
    writer.write(_rootNote);
    writer.write(_voltageRange);
    writer.write(_gateMode);
    writer.write(_slew);
}

void TuringMachineSequence::read(VersionedSerializedReader &reader) {
    reader.read(_initialRegister);
    reader.read(_loopLength);
    reader.read(_loopStart);
    reader.read(_probability);
    reader.read(_scale);
    reader.read(_rootNote);
    reader.read(_voltageRange);
    reader.read(_gateMode);
    reader.read(_slew);
    setInitialRegister(_initialRegister);
    setLoopLength(_loopLength);
    setLoopStart(_loopStart);
    setProbability(_probability);
    setScale(_scale);
    setRootNote(_rootNote);
    setVoltageRange(_voltageRange);
    setGateMode(_gateMode);
    setSlew(_slew);
}
