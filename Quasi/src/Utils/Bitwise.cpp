#include "Bitwise.h"

namespace Quasi::Bitwise {
    bool ContainsNullByte(u64 x) {
        // theres an explanation here: https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
        static constexpr u64 EXCEPT_FIRST = 0x7F7F7F7F'7F7F7F7F;
        return ~(((x & EXCEPT_FIRST) + EXCEPT_FIRST) | x | EXCEPT_FIRST);
    }

    u32 PlaceOfNullByte(u64 x) {
        static constexpr u64 EXCEPT_FIRST = 0x7F7F7F7F'7F7F7F7F;
        const u64 q = ~(((x & EXCEPT_FIRST) + EXCEPT_FIRST) | x | EXCEPT_FIRST);
        return q ? u64s::CountRightZeros(q) >> 3 : 8;
    }
}
