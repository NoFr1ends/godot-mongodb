#pragma once

#include "core/dictionary.h"

class Bson {
public:
    static int serialize(Dictionary dict, Vector<uint8_t> *data, int offset = 0);
    static int deserialize(Vector<uint8_t> *data, Dictionary &dict, int offset = 0);
};