#pragma once
#include "Numeric.h"

namespace Quasi::Bitwise {
    bool ContainsNullByte(u64 x);
    u32 PlaceOfNullByte(u64 x); // this uses big endian: 01234567 where 0 is the highest position, && assumes there is a null byte
}
