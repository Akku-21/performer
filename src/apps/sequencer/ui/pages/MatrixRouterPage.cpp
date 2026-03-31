#include "MatrixRouterPage.h"

#include "Pages.h"

#include "ui/painters/WindowPainter.h"

#include "core/utils/StringBuilder.h"
#include "core/utils/MathUtils.h"
#include "core/utils/Random.h"

#include "os/os.h"

namespace {
    constexpr int HeaderY = 0;
    constexpr int SourceY = 16;
    constexpr int ConnectionTopY = 26;
    constexpr int ConnectionMidY = 31;
    constexpr int ConnectionBottomY = 37;
    constexpr int DestinationY = 46;
    constexpr int FooterY = 54;

    constexpr int SourceHeight = 8;
    constexpr int DestinationHeight = 8;

    constexpr int DestCount = 16;
    constexpr int CvOutCount = 8;

    enum class ContextAction {
        ClearAll,
        Randomize,
        Last
    };

    const ContextMenuModel::Item contextMenuItems[] = {
        { "CLR ALL" },
        { "RNDM" },
    };
}

MatrixRouterPage::MatrixRouterPage(PageManager &manager, PageContext &context) :
    BasePage(manager, context)
{
}

void MatrixRouterPage::enter() {
    _selectedSource = 0;
    _selectedDest = 0;
    _destMask = 0;
    _amountDisplay = 0;
    _showAmount = false;
    _lastTappedDest = -1;
    _lastTapTicks = 0;
}

void MatrixRouterPage::draw(Canvas &canvas) {
    WindowPainter::clear(canvas);
    WindowPainter::drawHeader(canvas, _model, _engine, "MATRIX");

    const char *footerNames[] = { "SRCE", "", "", "BANK", "" };
    WindowPainter::drawFooter(canvas, footerNames, pageKeyState());

    canvas.setBlendMode(BlendMode::Set);

    const int srcCount = sourceCount();
    const int srcWidth = Width / srcCount;

    for (int i = 0; i < srcCount; ++i) {
        int x = i * srcWidth;
        canvas.setColor(isSrcSelected(i) ? Color::Bright : Color::Medium);
        if (isSrcSelected(i)) {
            canvas.fillRect(x + 1, SourceY, srcWidth - 2, SourceHeight);
        } else {
            canvas.drawRect(x + 1, SourceY, srcWidth - 2, SourceHeight);
        }
    }

    const int destCount = destinationCount();
    const int destWidth = Width / destCount;
    for (int i = 0; i < destCount; ++i) {
        int x = i * destWidth;
        bool selected = isDestSelected(i) || isDestMasked(i);
        canvas.setColor(selected ? Color::Bright : Color::Medium);
        if (selected) {
            canvas.fillRect(x + 1, DestinationY, destWidth - 2, DestinationHeight);
        } else {
            canvas.drawRect(x + 1, DestinationY, destWidth - 2, DestinationHeight);
        }
    }

    const auto &router = _model.matrixRouter();
    int baseCc = _ccBank * DestCount;
    MatrixRouter::SrcType srcType = sourceType();
    int srcBase = sourceIndex();

    if (_destinationTab == DestinationTab::MidiCc) {
        for (int i = 0; i < MatrixRouter::MaxConnections; ++i) {
            const auto &connection = router.connections[i];
            if (connection.amount == 0) {
                continue;
            }
            if (connection.srcType != static_cast<uint8_t>(srcType)) {
                continue;
            }
            if (connection.srcIndex < srcBase || connection.srcIndex >= srcBase + srcCount) {
                continue;
            }
            if (connection.dstCC < baseCc || connection.dstCC >= baseCc + destCount) {
                continue;
            }
            if (connection.dstCh != 0) {
                continue;
            }

            int srcIndex = connection.srcIndex - srcBase;
            int destIndex = connection.dstCC - baseCc;

            int srcCenter = srcIndex * srcWidth + srcWidth / 2;
            int destCenter = destIndex * destWidth + destWidth / 2;

            bool highlight = isSrcSelected(srcIndex) && (isDestSelected(destIndex) || isDestMasked(destIndex));
            canvas.setColor(highlight ? Color::Bright : Color::Medium);

            canvas.vline(srcCenter, SourceY + SourceHeight, ConnectionTopY - (SourceY + SourceHeight));
            canvas.vline(destCenter, ConnectionBottomY, DestinationY - ConnectionBottomY);

            int x1 = srcCenter;
            int x2 = destCenter;
            if (x1 > x2) {
                int tmp = x1;
                x1 = x2;
                x2 = tmp;
            }
            canvas.hline(x1, ConnectionMidY, x2 - x1);
        }
    } else {
        for (int i = 0; i < MatrixRouter::MaxCvConnections; ++i) {
            const auto &connection = router.cvConnections[i];
            if (connection.isEmpty() || connection.amount == 0) {
                continue;
            }
            if (connection.srcType != static_cast<uint8_t>(srcType)) {
                continue;
            }
            if (connection.srcIndex < srcBase || connection.srcIndex >= srcBase + srcCount) {
                continue;
            }
            if (connection.dstChannel >= destCount) {
                continue;
            }

            int srcIndex = connection.srcIndex - srcBase;
            int destIndex = connection.dstChannel;

            int srcCenter = srcIndex * srcWidth + srcWidth / 2;
            int destCenter = destIndex * destWidth + destWidth / 2;

            bool highlight = isSrcSelected(srcIndex) && (isDestSelected(destIndex) || isDestMasked(destIndex));
            canvas.setColor(highlight ? Color::Bright : Color::Medium);

            canvas.vline(srcCenter, SourceY + SourceHeight, ConnectionTopY - (SourceY + SourceHeight));
            canvas.vline(destCenter, ConnectionBottomY, DestinationY - ConnectionBottomY);

            int x1 = srcCenter;
            int x2 = destCenter;
            if (x1 > x2) {
                int tmp = x1;
                x1 = x2;
                x2 = tmp;
            }
            canvas.hline(x1, ConnectionMidY, x2 - x1);
        }
    }

    if (_showAmount) {
        canvas.setFont(Font::Small);
        canvas.setColor(Color::Bright);
        FixedStringBuilder<8> valueStr("%d", _amountDisplay);
        canvas.drawTextCentered(0, 32, Width, 10, valueStr);
        if (os::ticks() > _showAmountTicks + os::time::ms(800)) {
            _showAmount = false;
        }
    }
}

void MatrixRouterPage::updateLeds(Leds &leds) {
    for (int i = 0; i < 8; ++i) {
        bool selected = (i < sourceCount()) && isSrcSelected(i);
        leds.set(MatrixMap::fromTrack(i), false, selected);
    }

    for (int i = 0; i < destinationCount(); ++i) {
        bool selected = isDestSelected(i) || isDestMasked(i);
        leds.set(MatrixMap::fromStep(i), false, selected);
    }
}

void MatrixRouterPage::keyPress(KeyPressEvent &event) {
    const auto &key = event.key();

    if (key.isContextMenu()) {
        contextShow();
        event.consume();
        return;
    }

    if (key.isFunction()) {
        switch (key.function()) {
        case 0:
            cycleSourceTab();
            event.consume();
            return;
        case 3:
            cycleCcBank();
            event.consume();
            return;
        case 4:
            cycleDestinationTab();
            event.consume();
            return;
        default:
            break;
        }
    }

    if (key.isTrackSelect()) {
        selectSource(key.trackSelect());
        event.consume();
        return;
    }

    if (key.isStep()) {
        int destIndex = key.step();
        if (destIndex >= destinationCount()) {
            return;
        }
        if (key.shiftModifier()) {
            toggleDestinationMask(destIndex);
            event.consume();
            return;
        }

        bool sameDest = (destIndex == _selectedDest);
        uint32_t now = os::ticks();
        bool doubleTap = (sameDest && _lastTappedDest == destIndex && (now - _lastTapTicks) < os::time::ms(DoubleTapTimeout));

        selectDestination(destIndex);

        if (doubleTap) {
            removeConnection(destIndex);
            _lastTappedDest = -1;
            _lastTapTicks = 0;
        } else {
            _lastTappedDest = destIndex;
            _lastTapTicks = now;
            if (sameDest && sourceSelected() && destinationSelected()) {
                commitConnections();
            }
        }

        event.consume();
    }
}

void MatrixRouterPage::encoder(EncoderEvent &event) {
    if (!sourceSelected() || !destinationSelected()) {
        return;
    }

    int srcIndex = sourceIndex() + _selectedSource;
    int baseCc = _ccBank * DestCount;
    int8_t amountDelta = static_cast<int8_t>(event.value());

    auto &router = _model.matrixRouter();
    auto updateAmountForDest = [&](int destIndex) {
        if (_destinationTab == DestinationTab::MidiCc) {
            int dstCC = baseCc + destIndex;
            int connectionIndex = router.findConnection(sourceType(), srcIndex, dstCC, 0);
            int8_t amount = 0;
            if (connectionIndex >= 0) {
                amount = router.connections[connectionIndex].amount;
            }
            amount = utils::clamp<int>(amount + amountDelta, -100, 100);
            if (connectionIndex >= 0) {
                router.connections[connectionIndex].amount = amount;
            } else {
                router.addConnection(sourceType(), srcIndex, dstCC, 0, amount);
            }
            _amountDisplay = amount;
        } else {
            int dstChannel = destIndex;
            int connectionIndex = router.findCvConnection(sourceType(), srcIndex, dstChannel);
            int8_t amount = 0;
            if (connectionIndex >= 0) {
                amount = router.cvConnections[connectionIndex].amount;
            }
            amount = utils::clamp<int>(amount + amountDelta, -100, 100);
            if (connectionIndex >= 0) {
                router.cvConnections[connectionIndex].amount = amount;
            } else {
                router.addCvConnection(sourceType(), srcIndex, dstChannel, amount);
            }
            _amountDisplay = amount;
        }
    };

    if (_destMask) {
        int destCount = destinationCount();
        for (int i = 0; i < destCount; ++i) {
            if (_destMask & (1u << i)) {
                updateAmountForDest(i);
            }
        }
    } else {
        updateAmountForDest(_selectedDest);
    }

    _showAmount = true;
    _showAmountTicks = os::ticks();
    event.consume();
}

void MatrixRouterPage::cycleSourceTab() {
    _sourceTab = SourceTab((int(_sourceTab) + 1) % int(SourceTab::Last));
    _selectedSource = 0;
}

void MatrixRouterPage::cycleDestinationTab() {
    _destinationTab = DestinationTab((int(_destinationTab) + 1) % int(DestinationTab::Last));
    _selectedDest = 0;
    _destMask = 0;
}

void MatrixRouterPage::cycleCcBank() {
    _ccBank = (_ccBank + 1) % 8;
}

int MatrixRouterPage::sourceCount() const {
    switch (_sourceTab) {
    case SourceTab::Mod:
        return 8;
    case SourceTab::CvIn:
        return 4;
    case SourceTab::NoteLow:
    case SourceTab::NoteHigh:
        return 8;
    case SourceTab::Last:
        break;
    }
    return 8;
}

MatrixRouter::SrcType MatrixRouterPage::sourceType() const {
    switch (_sourceTab) {
    case SourceTab::Mod:
        return MatrixRouter::SrcType::Mod;
    case SourceTab::CvIn:
        return MatrixRouter::SrcType::CvIn;
    case SourceTab::NoteLow:
    case SourceTab::NoteHigh:
        return MatrixRouter::SrcType::Note;
    case SourceTab::Last:
        break;
    }
    return MatrixRouter::SrcType::Mod;
}

int MatrixRouterPage::sourceIndex() const {
    switch (_sourceTab) {
    case SourceTab::Mod:
    case SourceTab::CvIn:
        return 0;
    case SourceTab::NoteLow:
        return 0;
    case SourceTab::NoteHigh:
        return 8;
    case SourceTab::Last:
        break;
    }
    return 0;
}

int MatrixRouterPage::destinationCount() const {
    switch (_destinationTab) {
    case DestinationTab::MidiCc:
        return DestCount;
    case DestinationTab::CvOut:
        return CvOutCount;
    case DestinationTab::Last:
        break;
    }
    return DestCount;
}

int MatrixRouterPage::destCc(int destIndex) const {
    return _ccBank * DestCount + destIndex;
}

bool MatrixRouterPage::sourceSelected() const {
    return _selectedSource >= 0 && _selectedSource < sourceCount();
}

bool MatrixRouterPage::destinationSelected() const {
    return _selectedDest >= 0 && _selectedDest < destinationCount();
}

void MatrixRouterPage::selectSource(int index) {
    _selectedSource = utils::clamp(index, 0, sourceCount() - 1);
}

void MatrixRouterPage::selectDestination(int index) {
    _selectedDest = utils::clamp(index, 0, destinationCount() - 1);
}

void MatrixRouterPage::toggleDestinationMask(int index) {
    if (index < 0 || index >= destinationCount()) {
        return;
    }
    _destMask ^= (1u << index);
}

void MatrixRouterPage::commitConnections() {
    if (!sourceSelected() || !destinationSelected()) {
        return;
    }

    int srcIndex = sourceIndex() + _selectedSource;
    int baseCc = _ccBank * DestCount;
    auto &router = _model.matrixRouter();

    auto commitDest = [&](int destIndex) {
        if (_destinationTab == DestinationTab::MidiCc) {
            int dstCC = baseCc + destIndex;
            int existingIndex = router.findConnection(sourceType(), srcIndex, dstCC, 0);
            if (existingIndex >= 0) {
                int8_t amount = router.connections[existingIndex].amount;
                if (amount == 0) {
                    amount = 50;
                }
                router.connections[existingIndex].amount = amount;
            } else {
                router.addConnection(sourceType(), srcIndex, dstCC, 0, 50);
            }
        } else {
            int dstChannel = destIndex;
            int existingIndex = router.findCvConnection(sourceType(), srcIndex, dstChannel);
            if (existingIndex >= 0) {
                int8_t amount = router.cvConnections[existingIndex].amount;
                if (amount == 0) {
                    amount = 50;
                }
                router.cvConnections[existingIndex].amount = amount;
            } else {
                router.addCvConnection(sourceType(), srcIndex, dstChannel, 50);
            }
        }
    };

    if (_destMask) {
        int destCount = destinationCount();
        for (int i = 0; i < destCount; ++i) {
            if (_destMask & (1u << i)) {
                commitDest(i);
            }
        }
    } else {
        commitDest(_selectedDest);
    }
}

void MatrixRouterPage::removeConnection(int destIndex) {
    if (!sourceSelected()) {
        return;
    }

    int srcIndex = sourceIndex() + _selectedSource;
    auto &router = _model.matrixRouter();
    if (_destinationTab == DestinationTab::MidiCc) {
        int dstCC = destCc(destIndex);
        int connectionIndex = router.findConnection(sourceType(), srcIndex, dstCC, 0);
        if (connectionIndex >= 0) {
            router.removeConnection(connectionIndex);
        }
    } else {
        int dstChannel = destIndex;
        int connectionIndex = router.findCvConnection(sourceType(), srcIndex, dstChannel);
        if (connectionIndex >= 0) {
            router.removeCvConnection(connectionIndex);
        }
    }
}

void MatrixRouterPage::contextShow() {
    showContextMenu(ContextMenu(
        contextMenuItems,
        int(ContextAction::Last),
        [&] (int index) { contextAction(index); }
    ));
}

void MatrixRouterPage::contextAction(int index) {
    switch (ContextAction(index)) {
    case ContextAction::ClearAll:
        _model.matrixRouter().clear();
        showMessage("MATRIX CLEARED");
        break;
    case ContextAction::Randomize:
        randomizeConnections();
        break;
    case ContextAction::Last:
        break;
    }
}

void MatrixRouterPage::randomizeConnections() {
    auto &router = _model.matrixRouter();
    Random random(os::ticks());
    int count = 3 + int(random.nextRange(3));

    for (int i = 0; i < count; ++i) {
        MatrixRouter::SrcType type = MatrixRouter::SrcType(random.nextRange(3));
        int srcIndex = 0;
        switch (type) {
        case MatrixRouter::SrcType::Mod:
            srcIndex = int(random.nextRange(8));
            break;
        case MatrixRouter::SrcType::CvIn:
            srcIndex = int(random.nextRange(4));
            break;
        case MatrixRouter::SrcType::Note:
            srcIndex = int(random.nextRange(16));
            break;
        }

        int8_t amount = int8_t(random.nextRange(201)) - 100;
        if (amount == 0) {
            amount = 50;
        }
        if (random.nextRange(2) == 0) {
            int dstCC = int(random.nextRange(128));
            int dstCh = int(random.nextRange(1));
            router.addConnection(type, srcIndex, dstCC, dstCh, amount);
        } else {
            int dstChannel = int(random.nextRange(CvOutCount));
            router.addCvConnection(type, srcIndex, dstChannel, amount);
        }
    }

    showMessage("RANDOMIZED");
}

bool MatrixRouterPage::isSrcSelected(int index) const {
    return sourceSelected() && _selectedSource == index;
}

bool MatrixRouterPage::isDestSelected(int index) const {
    return destinationSelected() && _selectedDest == index;
}

bool MatrixRouterPage::isDestMasked(int index) const {
    return (_destMask & (1u << index)) != 0;
}
