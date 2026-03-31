#pragma once

#include "BasePage.h"

#include "ui/StepSelection.h"

class TuringMachineEditPage : public BasePage {
public:
    TuringMachineEditPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

private:
    enum class Layer {
        Register    = 0,
        Probability = 1,
        LoopLength  = 2,
        LoopStart   = 3,
        Scale       = 4,
        RootNote    = 5,
        VoltageRange= 6,
        GateMode    = 7,
        Slew        = 8,
        Last
    };

    enum class Function {
        Bits   = 0,
        Prob   = 1,
        Loop   = 2,
        Scale  = 3,
        Output = 4,
    };

    void switchLayer(int functionKey, bool shift);
    int activeFunctionKey();

    void drawDetail(Canvas &canvas);

    void contextShow();
    void contextAction(int index);
    bool contextActionEnabled(int index) const;

    void quickEdit(int index);

    Layer _layer = Layer::Register;
    bool _showDetail = false;
    uint32_t _showDetailTicks = 0;

    StepSelection<CONFIG_STEP_COUNT> _stepSelection;
};
