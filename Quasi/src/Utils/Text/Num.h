#pragma once
#include "Utils/Numeric.h"
#include "Formatting.h"
#include "Parsing.h"

namespace Quasi::Text {
#pragma region Format
    template <> struct Formatter<bool> : Formatter<Str> {
        using FormatOptions = TextFormatOptions;
        static usize FormatTo(StringWriter sw, bool input, const FormatOptions& options);
    };

    struct IntFormatter {
        // see python's int options
        struct FormatOptions {
            u32 numLen = 0;
            u32 width = 0;

            TextFormatOptions::Alignment alignment = TextFormatOptions::LEFT;
            char pad = ' ';
            bool showSign = false, shouldPadZero = false, showPrefix = false;
            enum Base { DECIMAL, BINARY, OCTAL, HEX, CAP_HEX } base = DECIMAL;
        };
        static FormatOptions ConfigureOptions(Str opt);
        template <class N> static usize FormatTo(StringWriter sw, N num, const FormatOptions& options);
    };

    template <> struct Formatter<u16> : IntFormatter {};
    template <> struct Formatter<i16> : IntFormatter {};
    template <> struct Formatter<u32> : IntFormatter {};
    template <> struct Formatter<i32> : IntFormatter {};
    template <> struct Formatter<u64> : IntFormatter {};
    template <> struct Formatter<i64> : IntFormatter {};

    struct FloatFormatter {
        // ((?'pad'.?)(?'align'[<^>])(?'totalLen'[0-9]+)\,)?(?'showSign'\+?)(?'shouldPadZero'0?)(?'width'[0-9]*)\.(?'precision'[0-9]*)(?'mode'[feEgG%])
        struct FormatOptions {
            u32 width = 0, precision = ~0;

            TextFormatOptions::Alignment alignment = TextFormatOptions::LEFT;
            char pad = ' ';
            bool showSign = false, shouldPadZero = false;
            enum Mode { SCIENTIFIC, FIXED, GENERAL, SCI_CAP, GEN_CAP, PERCENTAGE } mode = FIXED;
        };
        static FormatOptions ConfigureOptions(Str opt);
        template <class N> static usize FormatTo(StringWriter sw, N num, const FormatOptions& options);
    };

    template <> struct Formatter<f32> : FloatFormatter {};
    template <> struct Formatter<f64> : FloatFormatter {};
#pragma endregion

#pragma region Parse
    struct BoolParser {
        struct ParseOptions {
            enum {
                ALLOW_NUMERIC = 1, USE_WORDS = 2, GENERAL = ALLOW_NUMERIC | USE_WORDS
            } format = GENERAL;
        };
        static OptionUsize ParseUntil(Str string, Out<bool&> out, ParseOptions options);
    };

    template <> struct Parser<bool> : BoolParser {};

    struct IntParser {
        struct ParseOptions {
            enum {
                ADAPTIVE = 0,
                BASE_N   = 0,
                BASE_2   = 2,  BINARY   = 2,
                BASE_3   = 3,  BASE_4   = 4,  BASE_5   = 5,  BASE_6   = 6,  BASE_7   = 7,
                BASE_8   = 8,  OCTAL    = 8,
                BASE_9   = 9,
                BASE_10  = 10, DECIMAL  = 10,
                BASE_11  = 11, BASE_12  = 12, BASE_13  = 13, BASE_14  = 14, BASE_15  = 15,
                BASE_16  = 16, HEX      = 16,
                BASE_17  = 17, BASE_18  = 18, BASE_19  = 19, BASE_20  = 20,
                BASE_21  = 21, BASE_22  = 22, BASE_23  = 23, BASE_24  = 24,
                BASE_25  = 25, BASE_26  = 26, BASE_27  = 27, BASE_28  = 28,
                BASE_29  = 29, BASE_30  = 30, BASE_31  = 31, BASE_32  = 32,
                BASE_33  = 33, BASE_34  = 34, BASE_35  = 35, BASE_36  = 36,
            } radix = DECIMAL;
        };
        template <class N> static OptionUsize ParseUntil(Str string, Out<N&> out, ParseOptions options);
    };

    template <> struct Parser<u16> : IntParser {};
    template <> struct Parser<i16> : IntParser {};
    template <> struct Parser<u32> : IntParser {};
    template <> struct Parser<i32> : IntParser {};
    template <> struct Parser<u64> : IntParser {};
    template <> struct Parser<i64> : IntParser {};

    struct FloatParser {
        struct ParseOptions {
            enum {
                SCIENTIFIC = 1, FIXED = 2, GENERAL = SCIENTIFIC | FIXED,
            } format = GENERAL;
        };
    };

    namespace NumberConversion {
        template <class N>
        struct FloatConv {
            static OptionUsize ParseUntil(Str string, Out<N&> out, FloatParser::ParseOptions options);
            static usize ParseInteger(Str string, Out<N&> out);
            static usize ParseDecimal(Str string, InOut<N&> out);
            static OptionUsize ParseExponent(Str string, InOut<N&> out);
        };
    }

    template <> struct Parser<f32> : FloatParser, NumberConversion::FloatConv<f32> {};
    template <> struct Parser<f64> : FloatParser, NumberConversion::FloatConv<f64> {};
#pragma endregion

    namespace NumberConversion {
        // different from plain addition, bytes do not overflow to other bytes
        u32 Add4Bytes(u32 a, u32 b);

        // checks if all 8 bytes in digits is a valid digit character (0-9)
        bool AreAllInCharRange8(u64 digits, uchar min, uchar max);
        bool AreAllInCharRange4(u32 digits, uchar min, uchar max);
        bool AreAllDigits4(u32 digits);
        bool AreAllDigitsRadix4(u32 digits, u32 radix);
        bool AreAllDigits8(u64 digits);
        bool AreAllHexDigits4(u32 digits);
        bool AreAllHexDigitsRadix4(u32 digits, u32 radix);

        u32 ConvertDigits4(u32 chars);
        u64 ConvertDigits8(u64 chars);
        u32 ConvertHexDigits4(u32 chars);
        u64 ParseDigits8(u64 digits);
        u32 ParseDigits4(u32 digits);
        u32 ParseHexDigits4(u32 xdigits);

        // base 2-4, 4 digits can fit in 1 byte
        u32 ParseDigitsTinyRadix4(u32 dig, u32 radix);
        // base 5-16, 4 digits can fit in 2 bytes
        u32 ParseDigitsSmallRadix4(u32 dig, u32 radix);
        // base 17+, 4 digits can fit in a u32
        u32 ParseDigitsLargeRadix4(u32 dig, u32 radix);

        template <usize ByteParallelCount>
        OptionUsize ParseIntBy(Str string, Out<u64&> out, const Fn<bool, u64&, const char*> auto& acc, u32 maxDigs, u32 radix) {
            const usize originalSize = string.Length();
            string = string.TrimStart('0');

            usize i = 0;
            u64 n = 0;
            for (; i <= maxDigs; i += ByteParallelCount) {
                if (!acc(n, &string[i])) { // cannot read chunks of digits
                    const char* rest = &string[i];
                    for (u32 j = 0; j < ByteParallelCount; ++j) {
                        if (const auto d = Chr::TryToDigitRadix(rest[j], radix)) {
                            if (i + j > maxDigs) return nullptr;
                            n *= radix;
                            n += *d;
                        } else {
                            out = n;
                            return originalSize - string.Length() + i + j;
                        }
                    }
                }
            }
            return nullptr;
        }

        OptionUsize ParseBinaryInt    (Str string, Out<u64&> out, u32 bits);
        OptionUsize ParseDecimalInt   (Str string, Out<u64&> out, u32 bits);
        OptionUsize ParseHexInt       (Str string, Out<u64&> out, u32 bits);
        OptionUsize ParseTinyRadixInt (Str string, Out<u64&> out, u32 bits, u32 radix);
        OptionUsize ParseSmallRadixInt(Str string, Out<u64&> out, u32 bits, u32 radix);
        OptionUsize ParseAsciiRadixInt(Str string, Out<u64&> out, u32 bits, u32 radix);
        OptionUsize ParseLargeRadixInt(Str string, Out<u64&> out, u32 bits, u32 radix);
        template <Integer I>
        OptionUsize ParseInt(Str string, Out<I&> out, IntParser::ParseOptions options);

        template <Floating F>
        OptionUsize ParseNanOrInf(Str string, Out<F&> out);

        // formatting functions
        u64 U64ToBCD2(u64 x);
        u64 U64ToBCD4(u64 x);
        u64 U64ToBCD8(u64 x);

        u32 SmallU64ToString(u64 x, char* out);
		u32 U64ToString     (u64 x, char* out);
		u32 U64FullToString (u64 x, char* out);
        usize FormatU64(StringWriter sw, u64 num, const IntFormatter::FormatOptions& options, char sign = '\0');
        usize FormatI64(StringWriter sw, i64 num, const IntFormatter::FormatOptions& options);

        void WriteU64Decimal(StringWriter sw, u64 num);
        void WriteU64Binary (StringWriter sw, u64 num, u32 bitwidth);
        void WriteU64Octal  (StringWriter sw, u64 num, u32 octalDigits);
        void WriteU64Hex    (StringWriter sw, u64 num, u32 hexDigits, bool upperCase);

        char* AddSign(f64 f, char* out, bool alwaysShowSign = false);
        char* WriteFltDecimal(f64 f, char* out, u32 precision = ~0);
        char* WriteFltSci(f64 f, char* out, u32 precision = ~0, char e = 'e', int log10 = i32s::MIN);
        char* WriteFltFxd(f64 f, char* out, u32 width = 0, u32 precision = ~0, int log10 = i32s::MIN, char pad = ' ');
        char* WriteFltGen(f64 f, char* out, u32 width = 0, u32 precision = ~0, char e = 'e');

        usize FormatFltSci  (StringWriter sw, f64 f, const FloatFormatter::FormatOptions& options);
        usize FormatFltFxd  (StringWriter sw, f64 f, const FloatFormatter::FormatOptions& options);
        usize FormatFloating(StringWriter sw, f64 f, const FloatFormatter::FormatOptions& options);
    }

    template <class N> usize FloatFormatter::FormatTo(StringWriter sw, N num, const FormatOptions& options) {
        return NumberConversion::FormatFloating(sw, num, options);
    }

    template <class N> OptionUsize IntParser::ParseUntil(Str string, Out<N&> out, ParseOptions options) {
        return NumberConversion::ParseInt(string, out, options);
    }
}
