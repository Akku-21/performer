#pragma once

#include "BasePage.h"

class KeyboardPage : public BasePage {
public:
    KeyboardPage(PageManager &manager, PageContext &context);

    virtual void enter() override;
    virtual void exit() override;

    virtual void draw(Canvas &canvas) override;
    virtual void updateLeds(Leds &leds) override;

    virtual bool isModal() const override { return true; }  // Modal overlay that closes on page navigation

    virtual void keyDown(KeyEvent &event) override;
    virtual void keyUp(KeyEvent &event) override;
    virtual void keyPress(KeyPressEvent &event) override;
    virtual void encoder(EncoderEvent &event) override;

private:
    struct KeyState {
        bool pressed = false;
        int note = -1;
    };

    int noteForKey(int step) const;
    void playNote(int step);
    void releaseNote(int step);
    void drawKey(Canvas &canvas, int keyIndex, bool isBlack, bool pressed, bool active);

    // Sequence operations
    void initSequence();
    void duplicateSequence();
    void arpeggiator();  // Placeholder for future arpeggiator

    int _octave = 4;  // Middle octave (C4 = MIDI note 60)
    int _selectedTrackIndex = 0;
    KeyState _keyStates[16];
    uint32_t _lastPerformerPressTicks = 0;

    // Track switching with double-tap
    int _trackBank = 0;  // 0 = tracks 0-7, 1 = tracks 8-15
    uint32_t _lastTrackPressTime[8] = {0};
    int _lastTrackPressed = -1;

    // Pagination: 14 white keys (2 octaves) shown with 8-key active window
    int _frameOffset = 0;  // Not used
    int _rootOffset = 0;   // 0-6 (which white key is first: C=0, D=1, E=2, F=3, G=4, A=5, B=6)

    // Last note played (for persistent display)
    int _lastNotePlayed = -1;  // MIDI note number, -1 if none
};
