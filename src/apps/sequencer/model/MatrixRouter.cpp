#include "MatrixRouter.h"

#include "ProjectVersion.h"

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
    for (auto &connection : cvConnections) {
        connection = {};
        connection.srcType = 0xFF;
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

int MatrixRouter::findCvConnection(SrcType srcType, int srcIndex, int dstChannel) const {
    for (int i = 0; i < MaxCvConnections; ++i) {
        const auto &connection = cvConnections[i];
        if (connection.isEmpty()) {
            continue;
        }
        if (connection.srcType == static_cast<uint8_t>(srcType) &&
            connection.srcIndex == srcIndex &&
            connection.dstChannel == dstChannel) {
            return i;
        }
    }
    return -1;
}

bool MatrixRouter::addCvConnection(SrcType srcType, int srcIndex, int dstChannel, int8_t amount) {
    int existingIndex = findCvConnection(srcType, srcIndex, dstChannel);
    if (existingIndex >= 0) {
        cvConnections[existingIndex].amount = amount;
        return true;
    }

    for (int i = 0; i < MaxCvConnections; ++i) {
        if (cvConnections[i].isEmpty()) {
            cvConnections[i].srcType = static_cast<uint8_t>(srcType);
            cvConnections[i].srcIndex = srcIndex;
            cvConnections[i].dstChannel = dstChannel;
            cvConnections[i].amount = amount;
            return true;
        }
    }

    return false;
}

void MatrixRouter::removeCvConnection(int index) {
    if (index < 0 || index >= MaxCvConnections) {
        return;
    }
    cvConnections[index] = {};
    cvConnections[index].srcType = 0xFF;
}

int MatrixRouter::cvConnectionCount() const {
    int count = 0;
    for (const auto &connection : cvConnections) {
        if (!connection.isEmpty()) {
            ++count;
        }
    }
    return count;
}

void MatrixRouter::write(VersionedSerializedWriter &writer) const {
    for (const auto &connection : connections) {
        writer.write(connection.srcType);
        writer.write(connection.srcIndex);
        writer.write(connection.dstCC);
        writer.write(connection.dstCh);
        writer.write(connection.amount);
    }

    if (writer.writerVersion() >= ProjectVersion::Version34) {
        for (const auto &connection : cvConnections) {
            writer.write(connection.srcType);
            writer.write(connection.srcIndex);
            writer.write(connection.dstChannel);
            writer.write(connection.amount);
        }
    }
}

void MatrixRouter::read(VersionedSerializedReader &reader) {
    clear();
    if (reader.dataVersion() < ProjectVersion::Version33) {
        return;
    }
    for (auto &connection : connections) {
        reader.read(connection.srcType);
        reader.read(connection.srcIndex);
        reader.read(connection.dstCC);
        reader.read(connection.dstCh);
        reader.read(connection.amount);
    }

    if (reader.dataVersion() >= ProjectVersion::Version34) {
        for (auto &connection : cvConnections) {
            reader.read(connection.srcType);
            reader.read(connection.srcIndex);
            reader.read(connection.dstChannel);
            reader.read(connection.amount);
        }
    }
}
