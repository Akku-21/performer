#include "MatrixRouter.h"

namespace {
    bool isEmpty(const MatrixRouter::Connection &connection) {
        return connection.srcType == 0 &&
            connection.srcIndex == 0 &&
            connection.dstCC == 0 &&
            connection.dstCh == 0 &&
            connection.amount == 0;
    }
}

MatrixRouter::MatrixRouter() {
    clear();
}

void MatrixRouter::clear() {
    for (auto &connection : connections) {
        connection = {};
    }
}

int MatrixRouter::findConnection(SrcType srcType, int srcIndex, int dstCC, int dstCh) const {
    for (int i = 0; i < MaxConnections; ++i) {
        const auto &connection = connections[i];
        if (connection.srcType == static_cast<uint8_t>(srcType) &&
            connection.srcIndex == srcIndex &&
            connection.dstCC == dstCC &&
            connection.dstCh == dstCh) {
            return i;
        }
    }
    return -1;
}

bool MatrixRouter::addConnection(SrcType srcType, int srcIndex, int dstCC, int dstCh, int8_t amount) {
    int existingIndex = findConnection(srcType, srcIndex, dstCC, dstCh);
    if (existingIndex >= 0) {
        connections[existingIndex].amount = amount;
        return true;
    }

    for (int i = 0; i < MaxConnections; ++i) {
        if (isEmpty(connections[i])) {
            connections[i].srcType = static_cast<uint8_t>(srcType);
            connections[i].srcIndex = srcIndex;
            connections[i].dstCC = dstCC;
            connections[i].dstCh = dstCh;
            connections[i].amount = amount;
            return true;
        }
    }

    return false;
}

void MatrixRouter::removeConnection(int index) {
    if (index < 0 || index >= MaxConnections) {
        return;
    }
    connections[index] = {};
}

int MatrixRouter::connectionCount() const {
    int count = 0;
    for (const auto &connection : connections) {
        if (!isEmpty(connection)) {
            ++count;
        }
    }
    return count;
}
