#pragma once

#include "BasePage.h"

#include "model/MatrixRouter.h"

#include <cstdint>

class MatrixRouterPage : public BasePage {
public:
    MatrixRouterPage(PageManager &manager, PageContext &context);

    void enter() override;

    void draw(Canvas &canvas) override;
    void updateLeds(Leds &leds) override;

    void keyPress(KeyPressEvent &event) override;
    void encoder(EncoderEvent &event) override;

private:
    enum class SourceTab : uint8_t {
        Mod,
        CvIn,
        NoteLow,
        NoteHigh,
        Last,
    };

    enum class DestinationTab : uint8_t {
        MidiCc,
        CvOut,
        Last,
    };

    void cycleSourceTab();
    void cycleDestinationTab();
    void cycleCcBank();

    int sourceCount() const;
    MatrixRouter::SrcType sourceType() const;
    int sourceIndex() const;
    int destinationCount() const;
    int destCc(int destIndex) const;

    bool sourceSelected() const;
    bool destinationSelected() const;

    void selectSource(int index);
    void selectDestination(int index);
    void toggleDestinationMask(int index);

    void commitConnections();
    void removeConnection(int destIndex);

    void contextShow();
    void contextAction(int index);
    void randomizeConnections();

    bool isSrcSelected(int index) const;
    bool isDestSelected(int index) const;
    bool isDestMasked(int index) const;

    SourceTab _sourceTab = SourceTab::Mod;
    DestinationTab _destinationTab = DestinationTab::MidiCc;
    int _selectedSource = 0;
    int _selectedDest = 0;
    uint16_t _destMask = 0;
    int _ccBank = 0;

    int _amountDisplay = 0;
    bool _showAmount = false;
    uint32_t _showAmountTicks = 0;

    int _lastTappedDest = -1;
    uint32_t _lastTapTicks = 0;
    static constexpr uint32_t DoubleTapTimeout = 500;
};
