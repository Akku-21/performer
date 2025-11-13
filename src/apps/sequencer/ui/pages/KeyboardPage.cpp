#include "KeyboardPage.h"

#include "ui/LedPainter.h"
#include "ui/painters/WindowPainter.h"
#include "ui/painters/ScreenPainter.h"

#include "core/utils/StringBuilder.h"
#include "core/midi/MidiMessage.h"
#include "model/Types.h"
#include "engine/NoteTrackEngine.h"

#include "os/os.h"

KeyboardPage::KeyboardPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
    for (int i = 0; i < 16; ++i) {
        _keyStates[i].pressed = false;
        _keyStates[i].note = -1;
    }
}

void KeyboardPage::enter() {
    _selectedTrackIndex = _project.selectedTrackIndex();
    _octave = 4;
    _frameOffset = 0;
    _rootOffset = 0;
    _lastNotePlayed = -1;  // Reset last note on page enter

    for (int i = 0; i < 16; ++i) {
        _keyStates[i].pressed = false;
        _keyStates[i].note = -1;
    }
}

void KeyboardPage::exit() {
    for (int i = 0; i < 16; ++i) {
        if (_keyStates[i].pressed) {
            releaseNote(i);
        }
    }
}

// Map hardware buttons to notes
// S9-S16 (step 8-15) = white keys only (natural notes: C, D, E, F, G, A, B, C)
// S1-S8 (step 0-7) = dynamically map to black keys (sharps/flats) within active white key range
int KeyboardPage::noteForKey(int step) const {
    int baseNote = _octave * 12;

    // Steps 8-15 (S9-S16): Map to 8 consecutive WHITE keys
    // rootOffset (0-6) determines which white key is first: C=0, D=1, E=2, F=3, G=4, A=5, B=6
    // White keys chromatic positions: C=0, D=2, E=4, F=5, G=7, A=9, B=11
    if (step >= 8 && step <= 15) {
        int whiteKeyIndex = (step - 8) + _rootOffset;  // 0-14 (can span up to 2 octaves)

        // Map white key index to chromatic position
        static const int whiteKeyChromaticPos[7] = {0, 2, 4, 5, 7, 9, 11};  // C D E F G A B

        int octaveOffset = (whiteKeyIndex / 7) * 12;  // Which octave (0 or 12)
        int keyInOctave = whiteKeyIndex % 7;          // Which white key in octave (0-6)

        return baseNote + whiteKeyChromaticPos[keyInOctave] + octaveOffset;
    }

    // Steps 0-7 (S1-S8): Map to black keys with dynamic piano keyboard spacing
    // Pattern shifts based on which white keys are active
    // S1 maps to the gap BEFORE the first white key, S2 to gap after first white key, etc.
    if (step >= 0 && step <= 7) {
        // Get the 9 active white key notes (need 9 to check gap after 8th white key)
        static const int whiteKeyChromaticPos[7] = {0, 2, 4, 5, 7, 9, 11};  // C D E F G A B
        int whiteKeyNotes[9];

        for (int i = 0; i < 9; ++i) {
            int whiteKeyIndex = _rootOffset + i;
            int octaveOffset = (whiteKeyIndex / 7) * 12;
            int keyInOctave = whiteKeyIndex % 7;
            whiteKeyNotes[i] = baseNote + whiteKeyChromaticPos[keyInOctave] + octaveOffset;
        }

        // Map S1-S8 to gaps, offset by one position
        // S1 = gap before white key 0 (between previous white key and white key 0)
        // S2 = gap after white key 0 (between white key 0 and 1)
        // S3 = gap after white key 1 (between white key 1 and 2)
        // ... etc.

        int lowerWhite, upperWhite;

        if (step == 0) {
            // S1: Gap before first white key
            // Find the white key before the first active white key
            int prevWhiteNote = whiteKeyNotes[0] - 1;  // Start one semitone below
            while (prevWhiteNote >= 0) {
                int chromPos = prevWhiteNote % 12;
                bool isWhite = (chromPos == 0 || chromPos == 2 || chromPos == 4 ||
                               chromPos == 5 || chromPos == 7 || chromPos == 9 || chromPos == 11);
                if (isWhite) {
                    lowerWhite = prevWhiteNote;
                    upperWhite = whiteKeyNotes[0];
                    break;
                }
                prevWhiteNote--;
            }
        } else {
            // S2-S8: Gaps after white keys 0-6
            lowerWhite = whiteKeyNotes[step - 1];
            upperWhite = whiteKeyNotes[step];
        }

        // Check if there's a black key between these two white keys
        if (upperWhite - lowerWhite == 2) {
            // There's exactly one semitone gap = black key exists
            return lowerWhite + 1;
        } else if (upperWhite - lowerWhite == 1) {
            // Adjacent white keys (E-F or B-C) = no black key
            return -1;
        }
    }

    return -1;  // Button doesn't map to anything
}

void KeyboardPage::playNote(int step) {
    int note = noteForKey(step);
    if (note < 0 || note > 127) return;

    // Send note through selected track's engine monitoring system (same as MIDI keyboard input)
    // This allows live playback and recording on the selected track
    _engine.trackEngine(_selectedTrackIndex).monitorMidi(_engine.tick(), MidiMessage::makeNoteOn(0, note, 100));

    _keyStates[step].pressed = true;
    _keyStates[step].note = note;
    _lastNotePlayed = note;  // Store last note for persistent display
}

void KeyboardPage::releaseNote(int step) {
    if (!_keyStates[step].pressed || _keyStates[step].note < 0) return;

    int note = _keyStates[step].note;

    // Send note off through selected track's engine monitoring system
    _engine.trackEngine(_selectedTrackIndex).monitorMidi(_engine.tick(), MidiMessage::makeNoteOff(0, note));

    _keyStates[step].pressed = false;
    _keyStates[step].note = -1;
}

void KeyboardPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);

    FixedStringBuilder<32> str;
    str("KEYBOARD");
    WindowPainter::drawHeader(canvas, _model, _engine, str);

    // Function buttons: F1=INIT, F2=DUPL, F3=ARP, F4=OCT-, F5=OCT+, F6
    const char *functionNames[] = { "INIT", "DUPL", "ARP", "OCT-", "OCT+", nullptr };
    WindowPainter::drawFooter(canvas, functionNames, pageKeyState());

    // Draw 14 white keys (2 octaves = 7 white keys per octave)
    const int whiteKeyWidth = 12;  // 12px per white key
    const int whiteKeyHeight = 28;  // Reduced from 34 to make room for step display
    const int blackKeyWidth = 9;   // 9px per black key
    const int blackKeyHeight = 16;  // Reduced proportionally
    const int totalWidth = 14 * whiteKeyWidth;  // 168px total
    const int startX = (256 - totalWidth) / 2;  // Center: 44px margin each side
    const int startY = 15;
    const int keyboardMidY = startY + (whiteKeyHeight / 2);  // Vertical center of keyboard

    const char *noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    canvas.setFont(Font::Small);
    canvas.setColor(Color::Bright);
    canvas.setBlendMode(BlendMode::Set);

    int leftMargin = startX;
    int keyboardEndX = startX + totalWidth;
    int rightMargin = 256 - keyboardEndX;

    // LEFT SIDE: Track number and Root note/octave - centered vertically with keyboard
    // Track number
    canvas.setFont(Font::Small);
    FixedStringBuilder<8> trackStr;
    trackStr("T%d", _selectedTrackIndex + 1);
    int trackTextWidth = canvas.textWidth(trackStr);
    int trackX = (leftMargin - trackTextWidth) / 2;
    canvas.drawText(trackX, keyboardMidY + 6, trackStr);

    // Root note/octave - show first active white key
    FixedStringBuilder<8> rootStr;
    const char *whiteKeyNames[] = {"C", "D", "E", "F", "G", "A", "B"};
    // Calculate the actual MIDI note for the root and convert to display octave (MIDI octave - 1)
    int rootMidiNote = (_octave * 12) + (whiteKeyNames[_rootOffset][0] == 'C' ? 0 :
                                          whiteKeyNames[_rootOffset][0] == 'D' ? 2 :
                                          whiteKeyNames[_rootOffset][0] == 'E' ? 4 :
                                          whiteKeyNames[_rootOffset][0] == 'F' ? 5 :
                                          whiteKeyNames[_rootOffset][0] == 'G' ? 7 :
                                          whiteKeyNames[_rootOffset][0] == 'A' ? 9 : 11);
    int displayOctave = (rootMidiNote / 12) - 1;
    rootStr("%s%d", whiteKeyNames[_rootOffset], displayOctave);
    int rootTextWidth = canvas.textWidth(rootStr);
    int rootX = (leftMargin - rootTextWidth) / 2;
    canvas.drawText(rootX, keyboardMidY - 6, rootStr);  // Above track number

    // RIGHT SIDE: Currently pressed note or last played note - centered vertically with keyboard
    // Priority: Show currently pressed note, otherwise show last played note
    int displayNote = -1;

    // Check if any key is currently pressed
    for (int i = 0; i < 16; ++i) {
        if (_keyStates[i].pressed && _keyStates[i].note >= 0) {
            displayNote = _keyStates[i].note;
            break;
        }
    }

    // If no key pressed, show last played note
    if (displayNote < 0 && _lastNotePlayed >= 0) {
        displayNote = _lastNotePlayed;
    }

    // Display the note
    if (displayNote >= 0) {
        canvas.setFont(Font::Small);
        int noteNum = displayNote % 12;
        int noteOct = (displayNote / 12) - 1;
        FixedStringBuilder<8> noteDisplayStr;
        noteDisplayStr("%s%d", noteNames[noteNum], noteOct);
        int noteDisplayWidth = canvas.textWidth(noteDisplayStr);
        int noteDisplayX = keyboardEndX + (rightMargin - noteDisplayWidth) / 2;
        canvas.drawText(noteDisplayX, keyboardMidY, noteDisplayStr);  // Centered vertically
    }

    // Draw all 14 white keys (2 octaves)
    // Calculate which white keys correspond to the 8 active buttons (S9-S16)
    int baseNote = _octave * 12;

    canvas.setBlendMode(BlendMode::Set);
    for (int i = 0; i < 14; ++i) {
        int x = startX + i * whiteKeyWidth;
        int y = startY;

        // Calculate the chromatic position of this white key
        // White keys on screen: C(0) D(1) E(2) F(3) G(4) A(5) B(6) C(7) D(8) E(9) F(10) G(11) A(12) B(13)
        // Chromatic positions:   0    2    4    5    7    9   11    0    2    4     5     7     9    11
        int whiteKeyChromaticPositions[] = {0, 2, 4, 5, 7, 9, 11, 0, 2, 4, 5, 7, 9, 11};
        int octaveOffset = (i >= 7) ? 12 : 0;  // Second octave starts at position 7
        int keyNote = baseNote + whiteKeyChromaticPositions[i] + octaveOffset;

        // Check if this white key is one of the 8 active keys (S9-S16)
        bool active = false;
        int activeStep = -1;
        for (int step = 8; step <= 15; ++step) {
            if (noteForKey(step) == keyNote) {
                active = true;
                activeStep = step;
                break;
            }
        }

        bool pressed = (active && activeStep >= 0 && _keyStates[activeStep].pressed);

        // Draw key outline
        canvas.setColor(active ? Color::Bright : Color::Low);
        canvas.drawRect(x, y, whiteKeyWidth - 1, whiteKeyHeight);

        // Fill key
        if (pressed) {
            canvas.setColor(Color::Medium);
        } else if (active) {
            canvas.setColor(Color::Low);
        } else {
            canvas.setColor(Color::None);
        }
        canvas.fillRect(x + 1, y + 1, whiteKeyWidth - 3, whiteKeyHeight - 2);
    }

    // Draw ALL black keys in standard piano pattern
    // 14 white keys (2 full octaves) require 10 black keys
    // Pattern per octave: C-C#-D-D#-E-F-F#-G-G#-A-A#-B (5 black keys per octave)
    // White keys: C(0) D(1) E(2) F(3) G(4) A(5) B(6) | C(7) D(8) E(9) F(10) G(11) A(12) B(13)
    // Black keys appear BETWEEN white keys at positions:
    // Octave 1: C#(1), D#(2), F#(4), G#(5), A#(6)
    // Octave 2: C#(8), D#(9), F#(11), G#(12), A#(13)
    // Chromatic values for black keys on screen:
    // Screen pos: 1=C#  2=D#  4=F#  5=G#  6=A#  8=C#  9=D#  11=F#  12=G#  13=A#
    int allBlackKeyScreenPos[] = {1, 2, 4, 5, 6, 8, 9, 11, 12, 13};
    int allBlackKeyChromaticPos[] = {1, 3, 6, 8, 10, 1, 3, 6, 8, 10};  // Chromatic positions
    int allBlackKeyOctaveOffset[] = {0, 0, 0, 0, 0, 12, 12, 12, 12, 12};  // Octave offset
    int numBlackKeys = 10;

    for (int i = 0; i < numBlackKeys; ++i) {
        int screenPos = allBlackKeyScreenPos[i];
        int chromaticPos = allBlackKeyChromaticPos[i];
        int octaveOff = allBlackKeyOctaveOffset[i];
        int keyNote = baseNote + chromaticPos + octaveOff;

        // Position black key between white keys on screen
        int x = startX + screenPos * whiteKeyWidth - (blackKeyWidth / 2);
        int y = startY;

        // Check if this black key is one of the active keys (S1-S8)
        bool active = false;
        int activeStep = -1;
        for (int step = 0; step < 8; ++step) {
            if (noteForKey(step) == keyNote) {
                active = true;
                activeStep = step;
                break;
            }
        }

        bool pressed = (active && activeStep >= 0 && _keyStates[activeStep].pressed);

        // Draw black key
        if (pressed) {
            canvas.setColor(Color::Bright);
            canvas.drawRect(x, y, blackKeyWidth, blackKeyHeight);
            canvas.setColor(Color::Medium);
            canvas.fillRect(x + 1, y + 1, blackKeyWidth - 2, blackKeyHeight - 2);
        } else {
            canvas.setColor(active ? Color::Medium : Color::Low);
            canvas.drawRect(x, y, blackKeyWidth, blackKeyHeight);
            canvas.setColor(Color::None);
            canvas.fillRect(x + 1, y + 1, blackKeyWidth - 2, blackKeyHeight - 2);
        }
    }

    // Draw step indicator below keyboard
    const auto &sequence = _project.selectedNoteSequence();
    const auto &trackEngine = _engine.selectedTrackEngine().as<NoteTrackEngine>();
    int stepsY = startY + whiteKeyHeight + 4;  // 4px gap below keyboard
    int stepWidth = 10;
    int stepHeight = 4;
    int stepsPerPage = 16;

    // Calculate which page of steps to show based on current playback position
    int currentStep = trackEngine.isActiveSequence(sequence) ? trackEngine.currentStep() : -1;
    int currentPage = (currentStep >= 0) ? (currentStep / stepsPerPage) : 0;
    int startStep = currentPage * stepsPerPage;
    int totalPages = (CONFIG_STEP_COUNT + stepsPerPage - 1) / stepsPerPage;

    // Draw 16 steps with spacing between groups
    for (int i = 0; i < stepsPerPage; ++i) {
        int stepIndex = startStep + i;
        if (stepIndex >= CONFIG_STEP_COUNT) break;

        int x = startX + (i * stepWidth) + (i / 8) * 8;  // Extra gap after 8 steps

        // Check if this step has a gate
        bool hasGate = sequence.step(stepIndex).gate();

        // Highlight current playing step
        bool isCurrent = (stepIndex == currentStep);

        canvas.setBlendMode(BlendMode::Set);
        if (isCurrent) {
            canvas.setColor(Color::Bright);
            canvas.fillRect(x, stepsY, stepWidth - 1, stepHeight);
        } else if (hasGate) {
            canvas.setColor(Color::Medium);
            canvas.fillRect(x, stepsY, stepWidth - 1, stepHeight);
        } else {
            canvas.setColor(Color::Low);
            canvas.drawRect(x, stepsY, stepWidth - 1, stepHeight);
        }
    }

    // Draw page indicator circles under the last note played area (right side)
    // Circle indicators: filled for current page, outline for others
    int circleRadius = 2;
    int circleSpacing = 8;
    int circleY = keyboardMidY + 10;  // Below the last note played text
    int totalCircleWidth = (totalPages * circleRadius * 2) + ((totalPages - 1) * circleSpacing);
    int circleStartX = keyboardEndX + (rightMargin - totalCircleWidth) / 2;

    canvas.setBlendMode(BlendMode::Set);
    for (int page = 0; page < totalPages; ++page) {
        int circleX = circleStartX + (page * (circleRadius * 2 + circleSpacing)) + circleRadius;

        if (page == currentPage) {
            // Filled circle for current page
            canvas.setColor(Color::Bright);
            canvas.fillRect(circleX - circleRadius, circleY - circleRadius, circleRadius * 2, circleRadius * 2);
        } else {
            // Outline circle for other pages
            canvas.setColor(Color::Medium);
            canvas.drawRect(circleX - circleRadius, circleY - circleRadius, circleRadius * 2, circleRadius * 2);
        }
    }
}

void KeyboardPage::updateLeds(Leds &leds) {
    // Clear all LEDs first
    for (int i = 0; i < 16; ++i) {
        leds.set(i, false, false);
    }

    // White keys (S9-S16): Solid GREEN LEDs always on
    for (int i = 8; i < 16; ++i) {
        int ledIndex = 15 - i;  // Inverted: S9=LED6, S10=LED5, ..., S16=LED0
        leds.set(ledIndex, false, true);  // Green
    }

    // Black keys (S1-S8): RED LEDs when button maps to a valid black key
    for (int step = 0; step < 8; ++step) {
        // Check if this button maps to a valid black key
        bool hasBlackKey = (noteForKey(step) >= 0);

        int ledIndex = step + 8;  // Direct: S1=LED8, S2=LED9, ..., S8=LED15
        leds.set(ledIndex, hasBlackKey, false);  // Red when button maps to black key
    }

    // Pagination LEDs: Use Global4 (left) and Global5 (right) - always on (wraps around)
    leds.set(Key::Global4, false, true);   // Left: always green (wraps around)
    leds.set(Key::Global5, false, true);  // Right: always green (wraps around)
}

void KeyboardPage::keyDown(KeyEvent &event) {
    const auto &key = event.key();

    if (key.isStep() && !key.pageModifier()) {
        int step = key.step();
        if (noteForKey(step) >= 0) {
            playNote(step);
        }
        // Consume step events only when not navigating to prevent writing to underlying sequencer
        event.consume();
        return;
    }
}

void KeyboardPage::keyUp(KeyEvent &event) {
    const auto &key = event.key();

    if (key.isStep() && !key.pageModifier()) {
        int step = key.step();
        if (noteForKey(step) >= 0) {
            releaseNote(step);
        }
        // Consume step events only when not navigating to prevent interaction with underlying sequencer
        event.consume();
        return;
    }
}

void KeyboardPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    // Handle transport controls (Play/Stop/Record) - forward to engine
    if (key.is(Key::Play)) {
        if (key.pageModifier()) {
            _engine.toggleRecording();
        } else {
            _engine.togglePlay(key.shiftModifier());
        }
        event.consume();
        return;
    }

    // Handle Performer button - pass through to TopPage for double-tap toggle
    if (key.isPerformer() && !key.pageModifier()) {
        // Don't consume - let TopPage handle the double-tap toggle
        return;
    }

    // Handle page navigation when PAGE is held
    // Check if this is a page navigation key (not step/track selection without page modifier)
    if (key.pageModifier()) {
        // Close keyboard for any page navigation
        close();
        // Don't consume - let TopPage handle the navigation
        return;
    }

    // Only handle keyboard-specific actions when PAGE is NOT held
    if (!key.pageModifier() && key.isTrackSelect()) {
        int trackButton = key.trackSelect();
        int targetTrack = (_trackBank * 8) + trackButton;

        for (int i = 0; i < 16; ++i) {
            if (_keyStates[i].pressed) {
                releaseNote(i);
            }
        }

        uint32_t now = os::ticks();
        uint32_t doubleTapWindow = os::time::ms(400);

        if (_lastTrackPressed == trackButton &&
            (now - _lastTrackPressTime[trackButton]) < doubleTapWindow) {
            targetTrack = ((1 - _trackBank) * 8) + trackButton;
            _trackBank = 1 - _trackBank;
            _lastTrackPressed = -1;
        } else {
            _lastTrackPressed = trackButton;
            _lastTrackPressTime[trackButton] = now;
        }

        _selectedTrackIndex = targetTrack;
        _project.setSelectedTrackIndex(targetTrack);
        event.consume();
        return;
    }

    // F1 = INIT (clear sequence)
    if (key.is(Key::F0)) {
        initSequence();
        event.consume();
        return;
    }

    // F2 = DUPL (duplicate sequence)
    if (key.is(Key::F1)) {
        duplicateSequence();
        event.consume();
        return;
    }

    // F3 = ARP (arpeggiator placeholder)
    if (key.is(Key::F2)) {
        arpeggiator();
        event.consume();
        return;
    }

    // F4 = Octave down (jump by 12 semitones)
    if (key.is(Key::F3)) {
        if (_octave > 0) {
            for (int i = 0; i < 16; ++i) {
                if (_keyStates[i].pressed) {
                    releaseNote(i);
                }
            }
            --_octave;
        }
        event.consume();
        return;
    }

    // F5 = Octave up (jump by 12 semitones)
    if (key.is(Key::F4)) {
        if (_octave < 9) {
            for (int i = 0; i < 16; ++i) {
                if (_keyStates[i].pressed) {
                    releaseNote(i);
                }
            }
            ++_octave;
        }
        event.consume();
        return;
    }

    // Left/Right shift semitone (same as encoder)
    if (key.is(Key::Left)) {
        if (_rootOffset > 0) {
            for (int i = 0; i < 16; ++i) {
                if (_keyStates[i].pressed) {
                    releaseNote(i);
                }
            }
            --_rootOffset;
        }
        event.consume();
        return;
    }

    if (key.is(Key::Right)) {
        if (_rootOffset < 11) {
            for (int i = 0; i < 16; ++i) {
                if (_keyStates[i].pressed) {
                    releaseNote(i);
                }
            }
            ++_rootOffset;
        }
        event.consume();
        return;
    }
}

void KeyboardPage::encoder(EncoderEvent &event) {
    // Encoder shifts active white key window by 1 white key (0-6 range: C D E F G A B)
    int oldOffset = _rootOffset;
    _rootOffset = _rootOffset + event.value();

    // Wrap around: 0-6 range (7 white keys per octave)
    while (_rootOffset < 0) _rootOffset += 7;
    while (_rootOffset >= 7) _rootOffset -= 7;

    if (_rootOffset != oldOffset) {
        for (int i = 0; i < 16; ++i) {
            if (_keyStates[i].pressed) {
                releaseNote(i);
            }
        }
    }
}

void KeyboardPage::initSequence() {
    // Clear the currently selected sequence
    _project.selectedNoteSequence().clearSteps();
    showMessage("STEPS INITIALIZED");
}

void KeyboardPage::duplicateSequence() {
    // Duplicate the currently selected sequence
    _project.selectedNoteSequence().duplicateSteps();
    showMessage("STEPS DUPLICATED");
}

void KeyboardPage::arpeggiator() {
    // Placeholder for future arpeggiator functionality
    showMessage("ARP NOT YET IMPLEMENTED");
}
