#include "TuringMachineEditPage.h"

#include "Pages.h"

#include "ui/LedPainter.h"
#include "ui/MatrixMap.h"
#include "ui/painters/SequencePainter.h"
#include "ui/painters/WindowPainter.h"

#include "engine/TuringMachineTrackEngine.h"

#include "model/Scale.h"

#include "os/os.h"

#include "core/utils/StringBuilder.h"

enum class ContextAction {
    Init,
    Copy,
    Paste,
    Last
};

static const ContextMenuModel::Item contextMenuItems[] = {
    { "INIT" },
    { "COPY" },
    { "PASTE" },
};

enum class Function {
    Bits   = 0,
    Prob   = 1,
    Loop   = 2,
    Scale  = 3,
    Output = 4,
};

static const char *functionNames[] = { "BITS", "PROB", "LOOP", "SCALE", "OUTPUT" };

static const char *gateModeNames[] = { "MSB", "PROB", "ALWAYS" };

TuringMachineEditPage::TuringMachineEditPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
}

void TuringMachineEditPage::enter() {
    _showDetail = false;
}

void TuringMachineEditPage::exit() {
}

void TuringMachineEditPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "TURING");

    const char *layerNames[] = {
        "BITS", "PROBABILITY", "LOOP LEN", "LOOP START",
        "SCALE", "ROOT NOTE", "V-RANGE", "GATE MODE", "SLEW"
    };
    WindowPainter::drawActiveFunction(canvas, layerNames[int(_layer)]);

    WindowPainter::drawFooter(canvas, functionNames, pageKeyState(), activeFunctionKey());

    const auto &trackEngine = _engine.selectedTrackEngine().as<TuringMachineTrackEngine>();
    const auto &sequence = _project.selectedTuringMachineSequence();

    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    uint16_t regValue = trackEngine.registerValue();

    const int stepWidth = Width / 16;
    const int loopLen = sequence.loopLength();
    const int loopStart = sequence.loopStart();

    // Loop window indicator bar (y=11..14)
    canvas.setBlendMode(BlendMode::Set);
    for (int i = 0; i < 16; ++i) {
        int x = i * stepWidth;
        bool inLoop = (i >= loopStart && i < loopStart + loopLen);
        canvas.setColor(inLoop ? Color::Bright : Color::Low);
        canvas.hline(x + 1, 12, stepWidth - 2);
    }

    // Step indices (y=16..19)
    for (int i = 0; i < 16; ++i) {
        int x = i * stepWidth;
        bool inLoop = (i >= loopStart && i < loopStart + loopLen);
        canvas.setColor(inLoop ? Color::Bright : Color::Low);
        FixedStringBuilder<4> str("%d", i + 1);
        canvas.drawText(x + (stepWidth - canvas.textWidth(str) + 1) / 2, 17, str);
    }

    // Bit boxes (y=20..35, 16x16 each)
    for (int i = 0; i < 16; ++i) {
        int x = i * stepWidth;
        int y = 20;
        bool inLoop = (i >= loopStart && i < loopStart + loopLen);
        bool bitSet = (regValue >> i) & 1;
        bool isPlayhead = (i == currentStep);
        bool isSelected = _stepSelection[i];

        canvas.setColor(isPlayhead ? Color::Bright : (inLoop ? Color::Medium : Color::Low));
        canvas.drawRect(x + 2, y + 2, stepWidth - 4, stepWidth - 4);

        if (bitSet && inLoop) {
            canvas.setColor(isSelected ? Color::Bright : (isPlayhead ? Color::Medium : Color::Bright));
            canvas.fillRect(x + 4, y + 4, stepWidth - 8, stepWidth - 8);
        }

        if (isSelected) {
            canvas.setColor(Color::Bright);
            canvas.drawRect(x + 1, y + 1, stepWidth - 2, stepWidth - 2);
        }

        if (inLoop) {
            switch (_layer) {
            case Layer::Probability:
                SequencePainter::drawProbability(
                    canvas, x + 2, y + 18, stepWidth - 4, 2,
                    sequence.probability(), TuringMachineSequence::ProbabilityMax
                );
                break;
            default:
                break;
            }
        }
    }

    // Global parameter display (y=40..53)
    {
        int y = 42;
        canvas.setColor(Color::Bright);
        FixedStringBuilder<32> str;

        switch (_layer) {
        case Layer::Register:
            // Show register as hex
            str("REG: %04X", regValue);
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        case Layer::Probability:
            str("PROB: %d%%", sequence.probability());
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        case Layer::LoopLength:
            str("LOOP: %d", sequence.loopLength());
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        case Layer::LoopStart:
            str("START: %d", sequence.loopStart() + 1);
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        case Layer::Scale:
            str("SCALE: %s", Scale::name(sequence.scale()));
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        case Layer::RootNote: {
            static const char *noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            str("ROOT: %s", noteNames[sequence.rootNote()]);
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        }
        case Layer::VoltageRange:
            str("RANGE: %dV", sequence.voltageRange());
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        case Layer::GateMode:
            str("GATE: %s", gateModeNames[sequence.gateMode()]);
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        case Layer::Slew:
            str("SLEW: %d%%", sequence.slew());
            canvas.drawText((Width - canvas.textWidth(str)) / 2, y, str);
            break;
        default:
            break;
        }
    }

    // Detail popup when step selected and encoder turning
    if (_showDetail) {
        if (_stepSelection.none()) {
            _showDetail = false;
        } else if (_stepSelection.isPersisted() && os::ticks() > _showDetailTicks + os::time::ms(500)) {
            _showDetail = false;
        }
    }

    if (_showDetail) {
        drawDetail(canvas);
    }
}

void TuringMachineEditPage::drawDetail(Canvas &canvas) {
    auto &sequence = _project.selectedTuringMachineSequence();
    uint16_t regValue = _engine.selectedTrackEngine().as<TuringMachineTrackEngine>().registerValue();

    int step = _stepSelection.first();
    bool bitSet = (regValue >> step) & 1;

    // Detail popup frame
    WindowPainter::drawFrame(canvas, 64, 16, 128, 32);

    canvas.setBlendMode(BlendMode::Set);
    canvas.setColor(Color::Bright);

    FixedStringBuilder<16> str;
    str("BIT %d: %s", step + 1, bitSet ? "1" : "0");
    canvas.drawText(80, 28, str);

    str.reset();
    str("PROB: %d%%", sequence.probability());
    canvas.drawText(80, 38, str);
}

void TuringMachineEditPage::updateLeds(Leds &leds) {
    const auto &trackEngine = _engine.selectedTrackEngine().as<TuringMachineTrackEngine>();
    const auto &sequence = _project.selectedTuringMachineSequence();

    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    uint16_t regValue = trackEngine.registerValue();

    int loopLen = sequence.loopLength();
    int loopStart = sequence.loopStart();

    for (int i = 0; i < 16; ++i) {
        bool inLoop = (i >= loopStart && i < loopStart + loopLen);
        bool bitSet = (regValue >> i) & 1;
        bool isPlayhead = (i == currentStep);
        bool isSelected = _stepSelection[i];

        bool red   = isPlayhead || isSelected;
        bool green = !isPlayhead && bitSet && inLoop && !isSelected;

        leds.set(MatrixMap::fromStep(i), red, green);
    }

    // Quick-edit hints when PAGE held
    if (globalKeyState()[Key::Page] && !globalKeyState()[Key::Shift]) {
        static const bool quickEditValid[8] = { true, true, true, true, true, true, true, true };
        for (int i = 0; i < 8; ++i) {
            int index = MatrixMap::fromStep(i + 8);
            leds.unmask(index);
            leds.set(index, false, quickEditValid[i]);
            leds.mask(index);
        }
    }
}

void TuringMachineEditPage::keyDown(KeyEvent &event) {
    _stepSelection.keyDown(event, 0);
    event.consume();
}

void TuringMachineEditPage::keyUp(KeyEvent &event) {
    _stepSelection.keyUp(event, 0);
    event.consume();
}

void TuringMachineEditPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();
    auto &sequence = _project.selectedTuringMachineSequence();
    auto &trackEngine = _engine.selectedTrackEngine().as<TuringMachineTrackEngine>();

    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return;
    }

    if (key.pageModifier() && key.isStep() && key.step() >= 8) {
        quickEdit(key.step() - 8);
        event.consume();
        return;
    }

    // SHIFT+Left/Right: rotate register
    if (key.shiftModifier() && key.isLeft()) {
        uint16_t reg = trackEngine.registerValue();
        uint16_t rotated = ((reg >> 1) | (reg << 15)) & 0xFFFF;
        trackEngine.setRegister(rotated);
        event.consume();
        return;
    }
    if (key.shiftModifier() && key.isRight()) {
        uint16_t reg = trackEngine.registerValue();
        uint16_t rotated = ((reg << 1) | (reg >> 15)) & 0xFFFF;
        trackEngine.setRegister(rotated);
        event.consume();
        return;
    }

    if (key.isFunction()) {
        switchLayer(key.function(), key.shiftModifier());
        event.consume();
        return;
    }

    // Step buttons: toggle bit (in Register layer) or select
    if (key.isStep()) {
        int stepIndex = key.step();
        if (_layer == Layer::Register && !key.shiftModifier()) {
                uint16_t reg = trackEngine.registerValue();
            reg ^= (1 << stepIndex);
            trackEngine.setRegister(reg);
            sequence.setInitialRegister(reg);
        } else {
            _stepSelection.keyPress(event, 0);
        }
        event.consume();
        return;
    }

    event.consume();
}

void TuringMachineEditPage::encoder(EncoderEvent &event) {
    auto &sequence = _project.selectedTuringMachineSequence();
    auto &trackEngine = _engine.selectedTrackEngine().as<TuringMachineTrackEngine>();

    bool shift = globalKeyState()[Key::Shift];

    switch (_layer) {
    case Layer::Register:
        if (_stepSelection.any()) {
            uint16_t reg = trackEngine.registerValue();
            for (int i = 0; i < 16; ++i) {
                if (_stepSelection[i]) {
                    if (event.value() > 0) {
                        reg |= (1 << i);
                    } else {
                        reg &= ~(1 << i);
                    }
                }
            }
            trackEngine.setRegister(reg);
            sequence.setInitialRegister(reg);
        }
        break;
    case Layer::Probability:
        sequence.editProbability(event.value(), shift);
        break;
    case Layer::LoopLength:
        sequence.editLoopLength(event.value(), shift);
        break;
    case Layer::LoopStart:
        sequence.editLoopStart(event.value(), shift);
        break;
    case Layer::Scale:
        sequence.editScale(event.value(), shift);
        break;
    case Layer::RootNote:
        sequence.editRootNote(event.value(), shift);
        break;
    case Layer::VoltageRange:
        sequence.editVoltageRange(event.value(), shift);
        break;
    case Layer::GateMode:
        sequence.editGateMode(event.value(), shift);
        break;
    case Layer::Slew:
        sequence.editSlew(event.value(), shift);
        break;
    default:
        break;
    }

    _showDetail = true;
    _showDetailTicks = os::ticks();

    event.consume();
}

void TuringMachineEditPage::switchLayer(int functionKey, bool shift) {
    if (shift) {
        switch (functionKey) {
        case int(Function::Bits):   _layer = Layer::Register;     break;
        case int(Function::Prob):   _layer = Layer::Probability;  break;
        case int(Function::Loop):   _layer = Layer::LoopLength;   break;
        case int(Function::Scale):  _layer = Layer::Scale;        break;
        case int(Function::Output): _layer = Layer::VoltageRange; break;
        }
        return;
    }

    switch (functionKey) {
    case int(Function::Bits):
        _layer = Layer::Register;
        break;
    case int(Function::Prob):
        _layer = Layer::Probability;
        break;
    case int(Function::Loop):
        switch (_layer) {
        case Layer::LoopLength: _layer = Layer::LoopStart;  break;
        default:                _layer = Layer::LoopLength; break;
        }
        break;
    case int(Function::Scale):
        switch (_layer) {
        case Layer::Scale:   _layer = Layer::RootNote; break;
        default:             _layer = Layer::Scale;    break;
        }
        break;
    case int(Function::Output):
        switch (_layer) {
        case Layer::VoltageRange: _layer = Layer::GateMode;     break;
        case Layer::GateMode:     _layer = Layer::Slew;         break;
        default:                  _layer = Layer::VoltageRange; break;
        }
        break;
    }
}

int TuringMachineEditPage::activeFunctionKey() {
    switch (_layer) {
    case Layer::Register:     return int(Function::Bits);
    case Layer::Probability:  return int(Function::Prob);
    case Layer::LoopLength:
    case Layer::LoopStart:    return int(Function::Loop);
    case Layer::Scale:
    case Layer::RootNote:     return int(Function::Scale);
    case Layer::VoltageRange:
    case Layer::GateMode:
    case Layer::Slew:         return int(Function::Output);
    default:                  return -1;
    }
}

void TuringMachineEditPage::contextShow() {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&](int index) { contextAction(index); },
        [&](int index) { return contextActionEnabled(index); }
    ));
}

void TuringMachineEditPage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::Init:
        _project.selectedTuringMachineSequence().clear();
        _engine.selectedTrackEngine().as<TuringMachineTrackEngine>().reset();
        break;
    case ContextAction::Copy:
        _model.clipBoard().copyTuringMachineSequence(_project.selectedTuringMachineSequence());
        break;
    case ContextAction::Paste:
        _model.clipBoard().pasteTuringMachineSequence(_project.selectedTuringMachineSequence());
        break;
    default:
        break;
    }
}

bool TuringMachineEditPage::contextActionEnabled(int index) const {
    switch (ContextAction(index)) {
    case ContextAction::Paste:
        return _model.clipBoard().canPasteTuringMachineSequence();
    default:
        return true;
    }
}

void TuringMachineEditPage::quickEdit(int index) {
    auto &sequence = _project.selectedTuringMachineSequence();
    switch (index) {
    case 0: _layer = Layer::LoopLength;   break;
    case 1: _layer = Layer::LoopStart;    break;
    case 2: _layer = Layer::Probability;  break;
    case 3: _layer = Layer::Scale;        break;
    case 4: _layer = Layer::RootNote;     break;
    case 5: _layer = Layer::VoltageRange; break;
    case 6: _layer = Layer::GateMode;     break;
    case 7: _layer = Layer::Slew;         break;
    default: break;
    }
    (void)sequence;
}
