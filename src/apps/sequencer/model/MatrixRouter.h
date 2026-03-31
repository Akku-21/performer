#pragma once

#include "Serialize.h"

#include <cstdint>

class MatrixRouter {
public:
    MatrixRouter();
    enum class SrcType : uint8_t { Mod = 0, CvIn = 1, Note = 2 };

    struct Connection {
        uint8_t srcType;
        uint8_t srcIndex;
        uint8_t dstCC;
        uint8_t dstCh;
        int8_t  amount;
    };

    static constexpr int MaxConnections = 48;

    Connection connections[MaxConnections] = {};

    void clear();

    int findConnection(SrcType srcType, int srcIndex, int dstCC, int dstCh) const;
    bool addConnection(SrcType srcType, int srcIndex, int dstCC, int dstCh, int8_t amount = 0);
    void removeConnection(int index);
    int connectionCount() const;

    void write(VersionedSerializedWriter &writer) const;
    void read(VersionedSerializedReader &reader);
};
