#include "Num.h"

#include "Utils/Debug/Logger.h"

namespace Quasi::Text {
    u32 NumberConversion::Add4Bytes(u32 a, u32 b) {
        static constexpr u32 EVEN_BYTES = 0xFF00FF00, ODD_BYTES = 0x00FF00FF;
        return (((a & EVEN_BYTES) + (b & EVEN_BYTES)) & EVEN_BYTES) |
               (((a &  ODD_BYTES) + (b &  ODD_BYTES)) &  ODD_BYTES);
    }

    bool NumberConversion::AreAllInCharRange8(u64 digits, uchar min, uchar max) {
        const u64 a = digits + 0x8080'8080'8080'8080 - max * 0x0101'0101'0101'0101;
        const u64 b = digits                         - min * 0x0101'0101'0101'0101;
        return ((a | b) & 0x8080'8080'8080'8080) == 0;
    }

    bool NumberConversion::AreAllInCharRange4(u32 digits, uchar min, uchar max) {
        const u32 a = digits + 0x8080'8080 - max * 0x0101'0101;
        const u32 b = digits               - min * 0x0101'0101;
        return ((a | b) & 0x8080'8080) == 0;
    }

    bool NumberConversion::AreAllDigits4(u32 digits) {
        return AreAllInCharRange4(digits, '0', '9' + 1);
    }

    bool NumberConversion::AreAllDigitsRadix4(u32 digits, u32 radix) {
        return AreAllInCharRange4(digits, '0', '0' + radix);
    }

    bool NumberConversion::AreAllDigits8(u64 digits) {
        return AreAllInCharRange8(digits, '0', '9' + 1);
    }

    bool NumberConversion::AreAllHexDigits4(u32 digits) {
        const u32 aLo = Add4Bytes(digits, 0x7F7F7F7F - "9999"_u32),
                  aHi = Add4Bytes(digits,            - "0000"_u32),
                  bLo = Add4Bytes(digits, 0x7F7F7F7F - "FFFF"_u32),
                  bHi = Add4Bytes(digits,            - "AAAA"_u32),
                  cLo = Add4Bytes(digits, 0x7F7F7F7F - "ffff"_u32),
                  cHi = Add4Bytes(digits,            - "aaaa"_u32);
        return (((aLo | aHi) & (bLo | bHi) & (cLo | cHi)) & 0x8080'8080) == 0;
    }

    bool NumberConversion::AreAllHexDigitsRadix4(u32 digits, u32 radix) {
        const u32 aLo = Add4Bytes(digits, 0x7F7F7F7F - "9999"_u32),
                  aHi = Add4Bytes(digits,            - "0000"_u32),
                  bLo = Add4Bytes(digits, 0x80808080 - "AAAA"_u32 - (radix - 10) * 0x0101'0101),
                  bHi = Add4Bytes(digits,            - "AAAA"_u32),
                  cLo = Add4Bytes(digits, 0x80808080 - "aaaa"_u32 - (radix - 10) * 0x0101'0101),
                  cHi = Add4Bytes(digits,            - "aaaa"_u32);
        return (((aLo | aHi) & (bLo | bHi) & (cLo | cHi)) & 0x8080'8080) == 0;
    }

    u32 NumberConversion::ConvertDigits4(u32 chars) {
        return chars - "0000"_u32;
    }

    u64 NumberConversion::ConvertDigits8(u64 chars) {
        return chars - "00000000"_u64;
    }

    u32 NumberConversion::ConvertHexDigits4(u32 chars) {
        // removes unnessecary information,
        // ranges '0'-'9' now become 0-9
        // ranges 'A'-'F'/'a'-'f' now both become 0x41-0x47
        chars &= 0x4F4F4F4F;
        // alphabet characters all have the 6th bit set
        const u64 isAlpha = chars & 0x40404040;
        // removes the extra bit for alphabets, now 0x41-0x47 -> 0x01-0x07
        // adds 9 if alpha is set, 0x01-0x07 -> 10-16
        chars = (isAlpha >> 6) * 9 + (chars ^ isAlpha);
        return chars;
    }

    // adapted from https://doc.rust-lang.org/src/core/num/dec2flt/parse.rs.html
    // which was further adapted from "Fast numeric string to
    // int", available here: <https://johnnylee-sde.github.io/Fast-numeric-string-to-int/>.
    // assumes digits are all in range of '0'-'9'
    u64 NumberConversion::ParseDigits8(u64 digits) {
        static constexpr u64 MASK = 0x0000'00FF'0000'00FF;
        static constexpr u64 MUL1 = 0x000F'4240'0000'0064; // 1'00'00'00 << 32 + 1'00
        static constexpr u64 MUL2 = 0x0000'2710'0000'0001; // 1'00'00    << 32 + 1
        // digits is now an array of 8 digits (0-9) for each byte (ABCDEFGH)
        digits = ConvertDigits8(digits);
        // consecutive digits are concat-ed (A0 BA CB DC ED FE GF HG)
        digits = digits * 10 + (digits >> 8);
        // retrives digits #4 and #8
        //   ________________[d4] ________________[d8] (each 32 bits)
        //   ____________[100*d4] ____________[100*d8] -> (* 100)
        //   ________[1000000*d8] ____________________ -> (<< 32 * 1000000)
        // + +++++++++++++++++++++++++++++++++++++++++
        //   _[1000000*d8+100*d4] ____________[100*d8]
        // v1 = [HG00DC00] [0000HG00]
        const u64 v1 = (digits & MASK) * MUL1;
        // retrieves digits #2 and #6
        //   ________________[d2] ________________[d6] (each 32 bits)
        //   __________[10000*d6] ____________________ -> (<< 32 * 10000)
        // + +++++++++++++++++++++++++++++++++++++++++
        //        [10000*d6 + d2] ________________[d6]
        // v2 = [00FE00BA] [000000FE]
        const u64 v2 = ((digits >> 16) & MASK) * MUL2;
        // v1      = [HG00DC00] [0000HG00]
        // v2      = [00FE00BA] [000000FE]
        // +         +++++++++++++++++++++
        // v1 + v2 = [HGFEDCBA] [0000HGFE] <- bottom 32 bits are discarded
        return (v1 + v2) >> 32;
    }

    u32 NumberConversion::ParseDigits4(u32 digits) {
        digits = ConvertDigits4(digits);
        digits = digits * 10 + (digits >> 8);
        return ((digits & 0x00FF00FF) * ((100 << 16) + 1)) >> 16;
    }

    u32 NumberConversion::ParseHexDigits4(u32 xdigits) {
        xdigits = ConvertHexDigits4(xdigits);
        xdigits = (xdigits * 0x1001) >> 8;
        return ((xdigits & 0x00FF00FF) * ((256 << 16) + 1)) >> 16;
    }

    u32 NumberConversion::ParseDigitsTinyRadix4(u32 dig, u32 radix) {
        static constexpr u32 TINY_LOOKUP[5] = { 0, 0, 0x08'04'02'01, 0x1B'09'03'01, 0x40'20'04'01 };
        return (dig * TINY_LOOKUP[radix]) >> 24;
    }

    u32 NumberConversion::ParseDigitsSmallRadix4(u32 dig, u32 radix) {
        dig = dig * radix + (dig >> 8);
        return ((dig & 0x00FF00FF) * ((radix * radix << 16) + 1)) >> 16;
    }

    u32 NumberConversion::ParseDigitsLargeRadix4(u32 dig, u32 radix) {
        const u32 d = (dig >> 24) & 0xFF,
                  c = (dig >> 16) & 0xFF,
                  b = (dig >> 8)  & 0xFF,
                  a = (dig >> 0)  & 0xFF;
        return ((a * radix + b) * radix + c) * radix + d;
    }

    OptionUsize NumberConversion::ParseBinaryInt(Str string, Out<u64&> out, u32 bits) {
        return ParseIntBy<8>(string, out,  [] (u64& n, const char* str) {
            u64 digs = Memory::ReadU64(str);
            digs = ConvertDigits8(digs);
            if (digs & 0x7E7E7E7E7E7E7E7E) return false;
            // magic multiplication to make array of 8 bits into a u8
            digs = (digs * 0x80'40'20'10'08'04'02'01) >> 56;
            n = (n << 8) + digs;
            return true;
        }, bits, 2);
    }

    OptionUsize NumberConversion::ParseDecimalInt(Str string, Out<u64&> out, u32 bits) {
        return ParseIntBy<4>(string, out, [] (u64& n, const char* str) {
            u32 digs = Memory::ReadU32(str);
            if (!AreAllDigits4(digs)) return false;
            digs = ParseDigits4(digs);
            n = n * 10'000 + digs;
            return true;
        }, bits * Math::INV_LOG10_2_MUL >> 16, 10);
    }

    OptionUsize NumberConversion::ParseHexInt(Str string, Out<u64&> out, u32 bits) {
        return ParseIntBy<4>(string, out, [] (u64& n, const char* str) {
            u32 digs = Memory::ReadU32(str);
            if (!AreAllHexDigits4(digs)) return false;
            digs = ParseHexDigits4(digs);
            n = (n << 16) + digs;
            return true;
        }, bits * 2, 16);
    }

    OptionUsize NumberConversion::ParseTinyRadixInt(Str string, Out<u64&> out, u32 bits, u32 radix) {
        const bool hasInexactDigit = radix & 1; // when radix is 3 instead of 2/4, the log is an irrational
        return ParseIntBy<4>(string, out, [&, r4 = radix * radix * radix * radix] (u64& n, const char* str) {
            u32 digs = Memory::ReadU32(str);
            if (!AreAllDigitsRadix4(digs, radix)) return false;
            digs = ConvertDigits4(digs);
            digs = ParseDigitsTinyRadix4(digs, radix);
            n = n * r4 + digs;
            return true;
        }, hasInexactDigit + (bits * Math::INV_LOG2_LOOKUP[radix] >> 16), radix);
    }

    OptionUsize NumberConversion::ParseSmallRadixInt(Str string, Out<u64&> out, u32 bits, u32 radix) {
        return ParseIntBy<4>(string, out, [&, r4 = radix * radix * radix * radix] (u64& n, const char* str) {
            u32 digs = Memory::ReadU32(str);
            if (!AreAllDigitsRadix4(digs, radix)) return false;
            digs = ConvertDigits4(digs);
            digs = ParseDigitsSmallRadix4(digs, radix);
            n = n * r4 + digs;
            return true;
        }, 1 + (bits * Math::INV_LOG2_LOOKUP[radix] >> 16), radix);
    }

    OptionUsize NumberConversion::ParseAsciiRadixInt(Str string, Out<u64&> out, u32 bits, u32 radix) {
        return ParseIntBy<4>(string, out, [&, r4 = radix * radix * radix * radix] (u64& n, const char* str) {
            u32 digs = Memory::ReadU32(str);
            if (!AreAllHexDigitsRadix4(digs, radix)) return false;
            digs = ConvertHexDigits4(digs);
            digs = ParseDigitsSmallRadix4(digs, radix);
            n = n * r4 + digs;
            return true;
        }, 1 + (bits * Math::INV_LOG2_LOOKUP[radix] >> 16), radix);
    }

    OptionUsize NumberConversion::ParseLargeRadixInt(Str string, Out<u64&> out, u32 bits, u32 radix) {
        return ParseIntBy<4>(string, out, [&, r4 = radix * radix * radix * radix] (u64& n, const char* str) {
            u32 digs = Memory::ReadU32(str);
            if (!AreAllHexDigitsRadix4(digs, radix)) return false;
            digs = ConvertHexDigits4(digs);
            digs = ParseDigitsLargeRadix4(digs, radix);
            n = n * r4 + digs;
            return true;
        }, 1 + (bits * Math::INV_LOG2_LOOKUP[radix] >> 16), radix);
    }

    template <Integer I>
    OptionUsize NumberConversion::ParseInt(Str string, Out<I&> out, IntParser::ParseOptions options) {
        static constexpr usize BITS = sizeof(I) * 8;
        bool negative = false;
        if (string.StartsWith('+'))
            string.Advance(1);
        else {
            if constexpr (Signed<I>)
                if (string.StartsWith('-')) { negative = true; string.Advance(1); }
        }

        if (string.IsEmpty()) return nullptr;

        if (options.radix == IntParser::ParseOptions::ADAPTIVE) {
            if (string.StartsWith('0')) {
                u64 n = 0;
                OptionUsize result;
                switch (string[2]) {
                    case 'b': result = ParseBinaryInt(string.Skip(2), n, BITS); break;
                    case 'x': result = ParseHexInt   (string.Skip(2), n, BITS); break;
                    default:  result = ParseSmallRadixInt(string, n, BITS, 8);  break;
                }
                if (!result) return result;
                if (negative ? (n < NumInfo<I>::MIN) : (n > NumInfo<I>::MAX)) return nullptr;

                *result += negative;
                out = negative ? (I)-n : (I)n;
                return result;
            } else {
                u64 n = 0;
                OptionUsize result = ParseDecimalInt(string, n, BITS);
                if (!result) return result;
                if (negative ? (n < NumInfo<I>::MIN) : (n > NumInfo<I>::MAX)) return nullptr;

                *result += negative;
                out = negative ? (I)-n : (I)n;
                return result;
            }
        }

        u64 n = 0;
        OptionUsize result;
        if (options.radix == 2)
            result = ParseBinaryInt(string, n, BITS);
        else if (options.radix == 10)
            result = ParseDecimalInt(string, n, BITS);
        else if (options.radix == 16)
            result = ParseHexInt(string, n, BITS);
        else if (options.radix <= 4)
            result = ParseTinyRadixInt(string, n, BITS, options.radix);
        else if (options.radix < 10)
            result = ParseSmallRadixInt(string, n, BITS, options.radix);
        else if (options.radix < 16)
            result = ParseAsciiRadixInt(string, n, BITS, options.radix);
        else
            result = ParseLargeRadixInt(string, n, BITS, options.radix);

        if (negative ? (n < NumInfo<I>::MIN) : (n > NumInfo<I>::MAX)) return nullptr;

        out = negative ? (I)-n : (I)n;
        return result;
    }

    template OptionUsize NumberConversion::ParseInt<u16>(Str string, Out<u16&> out, IntParser::ParseOptions options);
    template OptionUsize NumberConversion::ParseInt<i16>(Str string, Out<i16&> out, IntParser::ParseOptions options);
    template OptionUsize NumberConversion::ParseInt<u32>(Str string, Out<u32&> out, IntParser::ParseOptions options);
    template OptionUsize NumberConversion::ParseInt<i32>(Str string, Out<i32&> out, IntParser::ParseOptions options);
    template OptionUsize NumberConversion::ParseInt<u64>(Str string, Out<u64&> out, IntParser::ParseOptions options);
    template OptionUsize NumberConversion::ParseInt<i64>(Str string, Out<i64&> out, IntParser::ParseOptions options);

    template <Floating F>
    OptionUsize NumberConversion::ParseNanOrInf(Str string, Out<F&> out) {
        // utilizes u64s to optimize string comparisons
        constexpr u64 CLEAR_CASE = 0xDFDFDFDFDFDFDFDF;
        if (string.Length() < 3) return nullptr;
        if (string.Length() < 8) {
            u64 first3 = string[0] << 16 | string[1] << 8 | string[0];
            first3 &= CLEAR_CASE;
            if (first3 == "INF"_u64) {
                out = Math::Infinity;
            } else if (first3 == "NAN"_u64) {
                out = Math::NaN;
            } else return nullptr;
            return 3;
        } else {
            u64 first8 = Memory::ReadU64Big(string.Data());
            first8 &= CLEAR_CASE;
            if (first8 == "INFINITY"_u64) {
                out = Math::Infinity;
                return 8;
            }
        }
        return nullptr;
    }

    template OptionUsize NumberConversion::ParseNanOrInf<f32>(Str string, Out<f32&> out);
    template OptionUsize NumberConversion::ParseNanOrInf<f64>(Str string, Out<f64&> out);

    u64 NumberConversion::U64ToBCD2(u64 x) {
        // x          = [000] [0AB]
        // tens       = [000] [00A]
        // tens * 256 = [00A] [000]
        // tens * -10 = [000] [-A0]
        const u32 tenCarry = (x * 6554 >> 16) * (256 - 10); // shifting by 16 = /65536, 6554/65536 ~ 1/10
        x += tenCarry;
        return x;
    }

    u64 NumberConversion::U64ToBCD4(u64 x) {
        // x   = [0000ABCD]
        // top = [000000AB]
        //             vvvvvvvvvvv ~1/100
        u64 top = ((x * 5243) >> 19) & 0xFF;
        // x   = [00AB00CD]
        x += top * (65536 - 100);
        // top = [000A000C]
        top = ((x * 103) >> 10) & 0xF000F;
        // x   = [0A0B0C0D]
        x += top * (256 - 10);
        return x;
    }

    u64 NumberConversion::U64ToBCD8(u64 x) {
        // x   = [00000000ABCDEFGH]
        // top = [000000000000ABCD]
        u64 top = (x * 109951163) >> 40; // 1/10000
        // x   = [0000ABCD0000EFGH]
        x += top * ((1_u64 << 32) - 10000);
        // top = [000000AB000000EF]
        top = ((x * 5243) >> 19) & 0xFF000000FF;
        // x   = [00AB00CD00EF00GH]
        x += top * (65536 - 100);
        // top = [000A000C000E000G]
        top = ((x * 103) >> 10) & 0x000F000F000F000F;
        // x   = [0A0B0C0D0E0F0G0H]
        x += top * (256 - 10);
        return x;
    }

    u32 NumberConversion::SmallU64ToString(u64 x, char* out) {
        if (x <= 9) {
            out[0] = (char)(x | 0x30);
            return 1;
        } else {
            x = U64ToBCD2(x);
            Memory::WriteU16Big(x | 0x3030, out);
            return 2;
        }
    }

    u32 NumberConversion::U64ToString(u64 x, char* out) {
        if (x <= 99)
            return SmallU64ToString(x, out);

        const u32 len = (x > 999) ? 4 : 3;
        x = U64ToBCD4(x);
        x |= 0x30303030;

        if (len == 4) {
            Memory::WriteU32Big(x, out);
        } else {
            Memory::WriteU16Big(x >> 8, out);
            out[2] = (char)(x & 0xFF);
        }
        return len;
    }

    u32 NumberConversion::U64FullToString(u64 x, char* out) {
        u32 len;
        if (x <= 9999)
            return U64ToString(x, out);
        if (x < 100000000) { // 8 digits or less, can fit in 64 bits
            if (x > 999999)
                len = x > 9999999 ? 8 : 7;
            else
                len = x > 99999 ? 6 : 5;
        } else {
            // max num digits is 20

            u64 skip8 = x / 100'000'000;
            x -= (skip8 * 100'000'000);

            if (skip8 >= 100'000'000) {
                const u64 skip16 = skip8 / 100'000'000;
                skip8 -= (skip16 * 100'000'000);
                len = U64ToString(skip16, out);
                out += len;
                skip8 = U64ToBCD8(skip8);
                skip8 |= 0x3030303030303030;
                Memory::WriteU64Big(skip8, out);
                len += 8;
                out += 8;
            } else {
                len = u32s::Log10(skip8) + 1;
                skip8 = U64ToBCD8(skip8);
                skip8 |= 0x3030303030303030;
                Memory::MemCopyNoOverlap(out, ((const char*)&skip8) + 8 - len, len);
                out += len;
            }

            x = U64ToBCD8(x);
            x |= 0x3030303030303030;

            Memory::WriteU64Big(x, out);
            return len + 8;
        }

        x = U64ToBCD8(x);
        x |= 0x3030303030303030;

        Memory::WriteU64Big(x << ((8 - len) * 8), out);
        return len;
    }

    usize NumberConversion::FormatU64(StringWriter sw, u64 num, const IntFormatter::FormatOptions& options, char sign) {
        if (num == 0) {
            const u32 padLen = std::max(options.width, options.numLen) - options.numLen;
            const u32 right = padLen * (usize)options.alignment / 2;
            sw.WriteRepeat(options.pad, padLen - right);
            sw.WriteRepeat(options.shouldPadZero ? '0' : ' ', std::max(options.numLen, 1u) - 1);
            sw.Write('0');
            sw.WriteRepeat(options.pad, padLen - right);
            return options.width;
        }

        u32 nlen = 0;
        switch (options.base) {
            case IntFormatter::FormatOptions::DECIMAL: nlen = 1 + u64s::Log10(num); break;
            case IntFormatter::FormatOptions::BINARY:  nlen = u64s::BitWidth(num);  break;
            case IntFormatter::FormatOptions::OCTAL:   nlen = (u64s::BitWidth(num) + 2) / 3; break;
            case IntFormatter::FormatOptions::HEX:
            case IntFormatter::FormatOptions::CAP_HEX: nlen = (u64s::BitWidth(num) + 3) / 4; break;
            default:;
        }

        const u32 targetnLen = std::max(nlen, options.numLen) + (sign != '\0');
        u32 padLen = options.width - std::min(options.width, targetnLen);
        const u32 right = padLen * (usize)options.alignment / 2;

        if (options.showPrefix && options.base != IntFormatter::FormatOptions::DECIMAL) {
            sw.Write(Str::Slice(&"0b0o0x0X"[options.base * 2 - 2], 2));
            padLen = std::max(2u, padLen) - 2;
        }

        sw.WriteRepeat(options.pad, padLen - right);
        sw.WriteRepeat(options.shouldPadZero ? '0' : ' ', targetnLen - nlen);

        if (sign)
            sw.Write(sign);

        switch (options.base) {
            case IntFormatter::FormatOptions::DECIMAL: WriteU64Decimal(sw, num);          break;
            case IntFormatter::FormatOptions::BINARY:  WriteU64Binary(sw, num, nlen);     break;
            case IntFormatter::FormatOptions::OCTAL:   WriteU64Octal(sw, num, nlen);      break;
            case IntFormatter::FormatOptions::HEX:     WriteU64Hex(sw, num, nlen, false); break;
            case IntFormatter::FormatOptions::CAP_HEX: WriteU64Hex(sw, num, nlen, true);  break;
            default:;
        }

        sw.WriteRepeat(options.pad, right);

        return std::max(options.width, targetnLen);
    }

    usize NumberConversion::FormatI64(StringWriter sw, i64 num, const IntFormatter::FormatOptions& options) {
        const bool negative = num < 0;
        return FormatU64(sw, negative ? (u64)-num : (u64)num, options, negative ? '-' : options.showSign ? '+' : '\0');
    }

    void NumberConversion::WriteU64Decimal(StringWriter sw, u64 num) {
        char strbuf[u64s::DIGITS] = {};
        sw.Write(Str::Slice(strbuf, U64FullToString(num, strbuf)));
    }

    void NumberConversion::WriteU64Binary(StringWriter sw, u64 num, u32 bitwidth) {
        char strbuf[u64s::BITS] = {};
        u32 i = u64s::BITS - 8;
        while (num) {
            u64 x = ((num & 0x55) * 0x02040810204081) | ((num & 0xAA) * 0x02040810204081);
            x &= 0x0101010101010101;
            x |= 0x3030303030303030;
            Memory::WriteU64Big(x, &strbuf[i]);
            i -= 8;
            num >>= 8;
        }
        sw.Write(Str::Slice(strbuf + u64s::BITS - bitwidth, bitwidth));
    }

    void NumberConversion::WriteU64Octal(StringWriter sw, u64 num, u32 octalDigits) {
        char strbuf[24] = {};
        u32 i = sizeof(strbuf) - 4;
        while (num) {
            u32 x = (num & 7) |
                    (num & 070) << 5 |
                    (num & 0700) << 10 |
                    (num & 07000) << 15;
            x |= 0x30303030;
            Memory::WriteU32Big(x, &strbuf[i]);
            i -= 4;
            num >>= 4 * 3;
        }
        sw.Write(Str::Slice(strbuf + 24 - octalDigits, octalDigits));
    }

    void NumberConversion::WriteU64Hex(StringWriter sw, u64 num, u32 hexDigits, bool upperCase) {
        char strbuf[u64s::HEX_DIGITS] = {};
        for (u32 i = 0; i < 2; ++i) { // repeat twice
            u64 x = num & 0xFFFF'FFFF;
            // x = 00 00 00 00 ab cd ef gh
            x |= x << 16;
            // x = 00 00 ab cd ?? ?? ef gh
            x = ((x << 8) & 0x00FF0000'00FF0000) | // 00 ab 00 00 00 ef 00 00
                 (x       & 0x000000FF'000000FF);  // 00 00 00 cd 00 00 00 gh
            // 00 ab 00 cd 00 ef 00 gh
            x |= x << 4;
            // 0a ?b 0c ?d 0e ?f 0g ?h
            x &= 0x0F0F0F0F'0F0F0F0F;

            // if each hex digit of x is represented by a letter (aka >= 10)
            x += 0x06060606'06060606;
            x += ((x >> 4) & 0x01010101'01010101) * (upperCase ? 7 : 39);
            x += 0x2A2A2A2A'2A2A2A2A;
            Memory::WriteU64Big(x, &strbuf[(1 - i) * 8]);

            x >>= 32;
            if (!x) break;
        }
        sw.Write(Str::Slice(strbuf + (u64s::BITS / 4) - hexDigits, hexDigits));
    }

    char* NumberConversion::AddSign(f64 f, char* out, bool alwaysShowSign) {
        if (f < 0) {
            *out = '-';
            return ++out;
        }
        if (alwaysShowSign) {
            *out = '+';
            return ++out;
        }
        return out;
    }

    char* NumberConversion::WriteFltDecimal(f64 f, char* out, u32 precision) {
        if (precision == ~0) {
            // builtin 3-digit max precision
            u32 dig3 = f64s::FastToIntUnsigned(f * 1000);
            if (dig3 == 0) {
                return out;
            } else {
                dig3 = U64ToBCD4(dig3);
                const u32 empty = u32s::CountRightZeros(dig3) / 8;
                dig3 <<= 8;
                dig3 |= 0x30'30'30'00;
                *out++ = '.';
                Memory::WriteU32Big(dig3, out);
                out += 3 - empty;
            }
        } else {
            *out++ = '.';
            u32 p = 0;
            for (; p < precision; p += 2) {
                f64 dig2;
                f64s::SeparateDecimal(f * 100, dig2, f);

                u32 i = (u32)f64s::FloorToIntUnsigned(f);
                i = U64ToBCD2(i);
                Memory::WriteU16Big(i | 0x3030, out);
                out += 2;
            }
            if (p < precision) {
                *out++ = (char)(0x30 | f64s::FloorToIntUnsigned(f * 10));
            }
        }
        return out;
    }

    char* NumberConversion::WriteFltSci(f64 f, char* out, u32 precision, char e, int log10) {
        if (f == 0) {
            if (precision == ~0) {
                *out++ = '0';
                return out;
            }
            Memory::WriteU16(".0"_u16, out);
            out += 2;
            while (precision --> 0)
                *out++ = '0';
            return out;
        }

        f64 mant, mantDec;

        if (log10 == i32s::MIN) {
            f64 ilog;
            f64s::SeparateDecimal(f64s::Log10(f), ilog, mant);
            mant = f64s::Exp10(mant);
        } else {
            mant = f * f64s::Exp10(-log10);
        }
        f64s::SeparateDecimal(mant, mant, mantDec);

        *out = (char)('0' | f64s::FastToIntUnsigned(mant));
        out = WriteFltDecimal(mantDec, ++out, precision);
        // ...E+... or ...E-...
        Memory::WriteU16(e << 8 | (log10 < 0 ? '-' : '+'), out);
        out += 2;
        log10 = std::abs(log10);

        {
            u32 eStr = U64ToBCD4(log10);
            const u32 empty = u32s::CountLeftZeros(eStr) / 8;
            eStr <<= empty * 8;
            eStr |= 0x30303030;
            Memory::MemCopyNoOverlap(out, (const char*)&eStr, 4 - empty);
            out += 4 - empty;
        }

        return out;
    }

    char* NumberConversion::WriteFltFxd(f64 f, char* out, u32 width, u32 precision, int log10, char pad) {
        if (f == 0) {
            const u32 w = width > 1 ? width - 1 : 0;
            Memory::MemSet(out, pad, w);
            out += w;
            *out++ = '0';

            if (precision == ~0) return out;

            *out++ = '.';
            Memory::MemSet(out, pad, precision);

            return out + precision;
        } else if (f < 1) { // log will be negative
            const u32 w = width > 1 ? width - 1 : 0;
            Memory::MemSet(out, pad, w);
            out += w;
            *out++ = '0';
        } else {
            log10 = (log10 == i32s::MIN ? (int)f64s::FloorToIntUnsigned(f64s::Log10(f)) : log10) + 1;

            if (log10 < width) {
                Memory::MemSet(out, pad, width - log10);
                out += width - log10;
            }

            if (log10 > 4) {
                f64 frac;
                f *= f64s::Exp10(-f64s::FastToFloatUnsigned(log10 - 4));

                for (; log10 > 4; log10 -= 4) {
                    f64s::SeparateDecimal(f, f, frac);

                    u32 topDigits = f64s::FastToIntUnsigned(f);
                    topDigits = U64ToBCD4(topDigits);
                    topDigits |= 0x30303030;
                    Memory::WriteU32Big(topDigits, out);
                    out += 4;

                    f = frac * 10000;
                }
            }
            u32 d = f64s::FloorToIntUnsigned(f);
            d = U64ToBCD4(d);
            d |= 0x30303030;
            d <<= (-8 * log10) & 31;
            d = Memory::ByteSwap32(d);
            Memory::MemCopyNoOverlap(out, (const char*)&d, log10);
            out += log10;

            [[maybe_unused]] f64 ignore;
            f64s::SeparateDecimal(f, ignore, f);
        }

        out = WriteFltDecimal(f, out, precision);

        return out;
    }

    char* NumberConversion::WriteFltGen(f64 f, char* out, u32 width, u32 precision, char e) {
        const int log = (int)f64s::Log10(f);
        if (log > (int)width) {
            return WriteFltSci(f, out, precision, e);
        } else {
            return WriteFltFxd(f, out, width, precision, log);
        }
    }

    usize NumberConversion::FormatFltSci(StringWriter sw, f64 f, const FloatFormatter::FormatOptions& options) {
        // -_.___E+____
        // break down:
        // sign (1)
        // first digit (1)
        // decimal point (1)
        // decimals (options.precision or 3 if none specified (~0)) -> add 4 for ~0
        // E+ (2)
        // exponent (max 4)
        char* strbuf = Memory::QAlloca$(char, 1 + 1 + 1 + (options.precision + 4) + 2 + 4);
        const char* end = WriteFltSci(
            std::abs(f),
            AddSign(f, strbuf, options.showSign),
            options.precision,
            options.mode == FloatFormatter::FormatOptions::SCI_CAP ? 'E' : 'e'
        );
        return Formatter<Str>::FormatTo(sw,
            Str::Slice(strbuf, end - strbuf),
            { options.width, options.alignment, options.pad, false }
        );
    }

    usize NumberConversion::FormatFltFxd(StringWriter sw, f64 f, const FloatFormatter::FormatOptions& options) {
        // -___.___
        // sign (1)
        // digits (max(log10, width))
        // decimal point (1)
        // decimals (~0 -> 3, anything else -> itself)
        const int log10 = (int)f64s::Log10(std::abs(f));
        const u32 w = log10 <= 0 ? 1 : (log10 + 1);
        char* strbuf = Memory::QAlloca$(char, 1 + std::max(w, options.width) + 1 + (options.precision + 4));
        const char* end = WriteFltFxd(
            std::abs(f),
            AddSign(f, strbuf, options.showSign),
            options.width,
            options.precision,
            log10,
            options.pad
        );
        return Formatter<Str>::FormatTo(sw,
            Str::Slice(strbuf, end - strbuf),
            { options.width, options.alignment, options.pad, false }
        );
    }

    usize NumberConversion::FormatFloating(StringWriter sw, f64 f, const FloatFormatter::FormatOptions& options) {
        using enum FloatFormatter::FormatOptions::Mode;

        static constexpr char NaNString[] = "NaN%",
                              InfString[] = "-Infinity%";

        if (f == Math::NaN) {
            return Formatter<Str>::FormatTo(sw,
                Str::Slice(NaNString, 3 + options.mode == PERCENTAGE),
                { options.width, options.alignment, options.pad, false }
            );
        }
        if (f64s::IsInf(f)) {
            const bool isPositive = f > 0;
            return Formatter<Str>::FormatTo(sw,
                Str::Slice(InfString + isPositive, 9 - isPositive + options.mode == PERCENTAGE),
                { options.width, options.alignment, options.pad, false }
            );
        }

        switch (options.mode) {
            case SCIENTIFIC:
            case SCI_CAP:
                return FormatFltSci(sw, f, options);
            case FIXED: {
                return FormatFltFxd(sw, f, options);
            }
            case GENERAL:
            case GEN_CAP: {
                if (std::abs(f) > f64s::Exp10(options.width) ||
                    std::abs(f) < f64s::Exp10(-options.precision)) {
                    FloatFormatter::FormatOptions fopt = options;
                    fopt.mode = options.mode == GEN_CAP ? SCI_CAP : SCIENTIFIC;
                    return FormatFltSci(sw, f, fopt);
                } else {
                    return FormatFltFxd(sw, f, options);
                }
            }
            case PERCENTAGE: {
                return FormatFltFxd(sw, f * 100, options);
            }
            default: return 0;
        }
    }

    usize Formatter<bool>::FormatTo(StringWriter sw, bool input, const FormatOptions& options) {
        return Formatter<Str>::FormatTo(sw, input ? "true"_str : "false"_str, options);
    }

    IntFormatter::FormatOptions IntFormatter::ConfigureOptions(Str opt) {
        // see https://github.com/fmtlib/fmt/blob/0e078f6ed0624be8babc43bd145371d9f3a08aab/include/fmt/base.h#L1473

        enum State {
            BEGIN, ALIGN, SIGN, PREFIX, ZERO, WIDTH
        } state = BEGIN;

#define ENTER(S) Debug::QAssert$(S > state, "bad format spec"); state = S

        using Align = TextFormatOptions::Alignment;

        FormatOptions options;
        if (opt.IsEmpty()) { return options; }

        char c = 0;
        if (opt.Length() == 1) { c = opt[0]; }
        else {
            c = "<^>"_str.Contains(opt[1]) ? '\0' : opt[0];
        }

        while (true) {
            switch (c) {
                case '<': case '>': case '^':
                    ENTER(ALIGN);
                    options.alignment = c == '<' ? Align::LEFT : c == '^' ? Align::CENTER : Align::RIGHT;
                    opt.Advance(1);
                    if (!opt.IsEmpty() && Chr::IsDigit(opt[0])) {
                        const auto [n, w] = ParsePartial<u32>(opt);
                        options.width = w.Assert();
                        opt.Advance(*n);
                    }
                    break;
                case '+': case ' ': case '-':
                    ENTER(SIGN);
                    // TODO: add the 'space' showSign option
                    options.showSign = c == '+';
                    opt.Advance(1);
                    break;
                case '#':
                    ENTER(PREFIX);
                    options.showPrefix = true;
                    opt.Advance(1);
                    break;
                case '0':
                    ENTER(ZERO);
                    options.shouldPadZero = true;
                    opt.Advance(1);
                    break;
                case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9': {
                    ENTER(WIDTH);
                    const auto [n, w] = ParsePartial<u32>(opt);
                    options.numLen = w.Assert();
                    opt.Advance(*n);
                    break;
                }
                // case '.':
                //     ENTER(PRECISION);
                //
                //     const auto [] = parse_precision(begin, end, specs, specs.precision_ref, ctx);
                //     break;
                case 'd': return options;
                case 'X': options.base = FormatOptions::CAP_HEX; return options;
                case 'x': options.base = FormatOptions::HEX;     return options;
                case 'o': options.base = FormatOptions::OCTAL;   return options;
                case 'b': options.base = FormatOptions::BINARY;  return options;
                // case 'E': specs.set_upper(); FMT_FALLTHROUGH;
                // case 'e': return parse_presentation_type(pres::exp, float_set);
                // case 'F': specs.set_upper(); FMT_FALLTHROUGH;
                // case 'f': return parse_presentation_type(pres::fixed, float_set);
                // case 'G': specs.set_upper(); FMT_FALLTHROUGH;
                // case 'g': return parse_presentation_type(pres::general, float_set);
                default: {
                    if (opt.IsEmpty()) return options;
                    // Parse fill and alignment.
                    const char align = opt[1];
                    Debug::Assert("<^>"_str.Contains(align), "no align spec");
                    ENTER(ALIGN);
                    options.pad = c;
                    options.alignment = align == '<' ? Align::LEFT : align == '^' ? Align::CENTER : Align::RIGHT;
                    opt.Advance(2);
                }
            }
            if (opt.IsEmpty()) return options;
            c = opt[0];
        }

        return options;
    }

    template <class N>
    usize IntFormatter::FormatTo(StringWriter sw, N num, const FormatOptions& options) {
        if constexpr (Unsigned<N>)
            return NumberConversion::FormatU64(sw, (u64)num, options, options.showSign ? '+' : '\0');
        else
            return NumberConversion::FormatI64(sw, (i64)num, options);
    }

    template usize IntFormatter::FormatTo<u16>(StringWriter sw, u16 num, const FormatOptions& options);
    template usize IntFormatter::FormatTo<i16>(StringWriter sw, i16 num, const FormatOptions& options);
    template usize IntFormatter::FormatTo<u32>(StringWriter sw, u32 num, const FormatOptions& options);
    template usize IntFormatter::FormatTo<i32>(StringWriter sw, i32 num, const FormatOptions& options);
    template usize IntFormatter::FormatTo<u64>(StringWriter sw, u64 num, const FormatOptions& options);
    template usize IntFormatter::FormatTo<i64>(StringWriter sw, i64 num, const FormatOptions& options);

    FloatFormatter::FormatOptions FloatFormatter::ConfigureOptions(Str opt) {
        // see https://github.com/fmtlib/fmt/blob/0e078f6ed0624be8babc43bd145371d9f3a08aab/include/fmt/base.h#L1473

        enum State {
            BEGIN, ALIGN, SIGN, ZERO, WIDTH, PRECISION
        } state = BEGIN;

#define ENTER(S) Debug::QAssert$(S > state, "bad format spec"); state = S

        using Align = TextFormatOptions::Alignment;

        FormatOptions options;
        if (opt.IsEmpty()) { return options; }

        char c = 0;
        if (opt.Length() == 1) { c = opt[0]; }
        else {
            c = "<^>"_str.Contains(opt[1]) ? '\0' : opt[0];
        }

        while (true) {
            switch (c) {
                case '<': case '>': case '^':
                    ENTER(ALIGN);
                    options.alignment = c == '<' ? Align::LEFT : c == '^' ? Align::CENTER : Align::RIGHT;
                    opt.Advance(1);
                    break;
                case '+': case ' ': case '-':
                    ENTER(SIGN);
                    // TODO: add the 'space' showSign option
                    options.showSign = c == '+';
                    opt.Advance(1);
                    break;
                case '0':
                    ENTER(ZERO);
                    options.shouldPadZero = true;
                    opt.Advance(1);
                    break;
                case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9': {
                    ENTER(WIDTH);
                    const auto [n, w] = ParsePartial<u32>(opt);
                    options.width = w.Assert();
                    opt.Advance(*n);
                    break;
                }
                case '.': {
                    ENTER(PRECISION);
                    opt.Advance(1);
                    const auto [n, p] = ParsePartial<u32>(opt);
                    options.precision = p.Assert();
                    opt.Advance(*n);
                    break;
                }
                case 'E': options.mode = FormatOptions::SCI_CAP;    return options;
                case 'e': options.mode = FormatOptions::SCIENTIFIC; return options;
                case 'F': [[fallthrough]];
                case 'f': options.mode = FormatOptions::FIXED;      return options;
                case 'G': options.mode = FormatOptions::GEN_CAP;    return options;
                case 'g': options.mode = FormatOptions::GENERAL;    return options;
                default: {
                    if (opt.IsEmpty()) return options;
                    // Parse fill and alignment.
                    const char align = opt[1];
                    Debug::Assert("<^>"_str.Contains(align), "no align spec");
                    ENTER(ALIGN);
                    options.pad = c;
                    options.alignment = align == '<' ? Align::LEFT : align == '^' ? Align::CENTER : Align::RIGHT;
                    opt.Advance(2);
                }
                if (opt.IsEmpty()) return options;
                c = opt[0];
            }
        }

        return options;
    }

    OptionUsize BoolParser::ParseUntil(Str string, Out<bool&> out, ParseOptions options) {
        if (options.format & ParseOptions::ALLOW_NUMERIC) {
            if (!string) return nullptr;
            const char first = string.First();
            if (first == '0' || first == '1') {
                out = first == '1';
                return 1;
            }
            return nullptr;
        }
        if (options.format & ParseOptions::USE_WORDS) {
            if (string.StartsWith("true"))  { out = true;  return 4; }
            if (string.StartsWith("false")) { out = false; return 5; }
        }
        return nullptr;
    }

    template <class N>
    OptionUsize NumberConversion::FloatConv<N>::ParseUntil(Str string, Out<N&> out, FloatParser::ParseOptions options) {
        const bool isFixed = (options.format & FloatParser::ParseOptions::FIXED),
                   isSci   = (options.format & FloatParser::ParseOptions::SCIENTIFIC);

        bool negative = false, hasSign = true;
        if (string.StartsWith('+')) string.Advance(1);
        else if (string.StartsWith('-')) { negative = true; string.Advance(1); }
        else hasSign = false;

        N num;
        const usize integerPart = ParseInteger(string, num);

        if (string.Length() == integerPart) {
            if (isSci)
                return nullptr;
            out = negative ? -num : num;
            return integerPart + hasSign;
        }

        usize decimalPart = 0;
        if (string[integerPart] == '.') {
            N decimal = 0;
            decimalPart = ParseDecimal(string.Skip(integerPart + 1), decimal);
            num += decimal;
        }

        const usize fixedPart = integerPart + decimalPart + 1;

        if (fixedPart == string.Length()) {
            out = negative ? -num : num;
            return string.Length() + hasSign;
        }

        if (Chr::ToUpper(string[fixedPart]) == 'E' && isSci) {
            if (decimalPart == 0) return nullptr;
            const Str expStr = string.Skip(fixedPart + 1);
            const OptionUsize expPart = ParseExponent(expStr, num);
            if (!expPart) return nullptr;

            out = negative ? -num : num;
            return hasSign + fixedPart + 1 + *expPart;
        }

        if (isFixed) return nullptr;

        out = negative ? -num : num;
        return hasSign + fixedPart;
    }

    template <class N>
    usize NumberConversion::FloatConv<N>::ParseInteger(Str string, Out<N&> out) {
        string = string.First(string.FindIf([] (Str x) { return !Chr::IsDigit(x[0]); }).UnwrapOr(string.Length()));
        const usize totalLen = string.Length();
        string = string.TrimStart('0');

        if (string.Length() > NumInfo<N>::MAX_EXP10) {
            out = Math::Infinity;
            return totalLen;
        }

        N num = 0;
        usize i = 0;
        for (; i < (string.Length() & ~3); i += 4) {
            u32 dig = Memory::ReadU32(string.Data() + i);
            dig = ParseDigits4(dig);
            num = num * N { 10'000 } + dig;
        }
        for (; i < string.Length(); ++i) {
            num = num * N { 10 } + Chr::ToDigit(string[i]);
        }
        out = num;
        return totalLen;
    }

    template <class N>
    usize NumberConversion::FloatConv<N>::ParseDecimal(Str string, InOut<N&> out) {
        string = string.First(string.FindIf([] (Str x) { return !Chr::IsDigit(x[0]); }).UnwrapOr(string.Length()));
        const usize totalLen = string.Length();
        string = string.TrimEnd('0');

        N decimal = 1.0f;
        usize i = 0;
        for (; i < (string.Length() & ~3); i += 4) {
            u32 dig = Memory::ReadU32(string.Data() + i);
            if (dig == "0000"_u32) continue;
            dig = ParseDigits4(dig);
            decimal *= N { 0.0001 };
            const N newOut = out + decimal * dig;
            if (newOut == out) return totalLen;
            out = newOut;
        }
        for (; i < string.Length(); ++i) {
            decimal *= N { 0.1 };
            if (string[i] == '0') continue;
            const N newOut = out + decimal * Chr::ToDigit(string[i]);
            if (newOut == out) return totalLen;
            out = newOut;
        }
        return totalLen;
    }

    template <class N>
    OptionUsize NumberConversion::FloatConv<N>::ParseExponent(Str string, InOut<N&> out) {
        if (!string) return nullptr;

        bool negExp = false;
        if (string.StartsWith('+')) string.Advance(1);
        if (string.StartsWith('-')) { negExp = true; string.Advance(1); }
        if (!string) return nullptr;

        i32 exp = 0;
        usize i = 0;

        for (; i < string.Length(); ++i) {
            if (const Option d = Chr::TryToDigit(string[i])) {
                if (exp < 10000) exp = exp * 10 + (i32)*d;
            } else { out *= std::exp2(Math::LOG10_2 * (negExp ? -(float)exp : (float)exp)); return i; }
        }

        out *= std::exp2(Math::LOG10_2 * (negExp ? -(float)exp : (float)exp));
        return i;
    }

    template struct NumberConversion::FloatConv<float>;
    template struct NumberConversion::FloatConv<double>;
}
