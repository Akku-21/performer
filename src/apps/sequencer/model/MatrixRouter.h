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

    struct CvConnection {
        uint8_t srcType = 0xFF;
        uint8_t srcIndex = 0;
        uint8_t dstChannel = 0;
        int8_t  amount = 0;

        uint8_t isEmpty() const { return srcType == 0xFF; }
    };

    static constexpr int MaxConnections = 48;
    static constexpr int MaxCvConnections = 16;

    Connection connections[MaxConnections] = {};
    CvConnection cvConnections[MaxCvConnections] = {};

    void clear();

    int findConnection(SrcType srcType, int srcIndex, int dstCC, int dstCh) const;
    bool addConnection(SrcType srcType, int srcIndex, int dstCC, int dstCh, int8_t amount = 0);
    void removeConnection(int index);
    int connectionCount() const;

    int findCvConnection(SrcType srcType, int srcIndex, int dstChannel) const;
    bool addCvConnection(SrcType srcType, int srcIndex, int dstChannel, int8_t amount = 0);
    void removeCvConnection(int index);
    int cvConnectionCount() const;

    void write(VersionedSerializedWriter &writer) const;
    void read(VersionedSerializedReader &reader);
};
