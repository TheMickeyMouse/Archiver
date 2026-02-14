#pragma once
#include "Macros.h"

#include "Enum.h"
#include "Ref.h"
#include "String.h"
#include "Vec.h"

namespace Quasi::Text {
#define STYLE_SWITCH(N, S) (N, (0, "<" S ">", true, true))(N##_OFF, (0, "</" S ">", true, false))
    struct StyleData { // each 16 values represents 1 byte of memeory taken
        u32 byteEncodingSize;
        Str debugStr;
        bool isSwitch : 1 = false, isOn : 1 = false;

        QDefineEnum$(Style,
            STYLE_SWITCH(BOLD,          "b")
            STYLE_SWITCH(ITALIC,        "i")
            STYLE_SWITCH(UNDERLINE,     "u")
            STYLE_SWITCH(STRIKETHROUGH, "s")
            STYLE_SWITCH(SUBSCRIPT,     "sub")
            STYLE_SWITCH(SUPERSCRIPT,   "sup")
            STYLE_SWITCH(BLOCKQUOTE,    "bq")
            STYLE_SWITCH(ALT_TEXT,      "*alt")
            (FONT_TYPE,       (1, "<*font=\"{}\">",      false, false))
            (CODE_BLOCK,      (1, "<code *lang=\"{}\">", false, false))
            (ALIGNMENT,       (1, "<*align={}>",         false, false))
            (FONT_SIZE,       (2, "<*size={}>",          false, false))
            (TEXT_COLOR,      (4, "<*color={}>",         false, false))
            (HIGHLIGHT_COLOR, (4, "<*hcolor={}>",        false, false)),
        ARITH)
    };
#undef STYLE_SWITCH

    struct RichString {
        static constexpr char DELIMITER = (char)128;
        
        String rawString;
        Vec<byte> stylings;

        static RichString ParseMarkdown(Str markdown);
        static RichString ParseHtml(Str html);

        String& RawString() { return rawString; }
        const String& RawString() const { return rawString; }

        void Append(const char c) { rawString += c; }
        void Append(Str s) { rawString += s; }
        void AddTag(Style s, IList<byte> data);
        void AddTag(Style s, bool state);

        uint Lines() const;

        struct Iter;
        struct RichChar;
        struct StyleState;
        
        Iter begin() const;
        Iter end() const;
    };

    struct RichString::StyleState {
        byte styleFlags = 0, alignment = 0;
        byte codeLang = 0;
        byte color[4] = { 0, 0, 0, 255 }, highlight[4] = { 0, 0, 0, 0 };
        byte font = 0;
        i16 fontSizeP64 = 0; // big endian, aka 0x12'34 and not 0x34'12

        void AddState(Style s, const byte* data);
    };

    struct RichString::RichChar {
        char c;
        StyleState style;
    };

    struct RichString::Iter {
        Ref<const RichString> str;
        const char* iter;
        StyleState currentState;
        u32 styleOffset = 0;

        bool operator==(const Iter& other) const { return iter == other.iter; }
        RichChar operator*() const;
        Iter& operator++();
    };

    inline RichString operator ""_md(const char* markdown, usize size) {
        return RichString::ParseMarkdown(Str::Slice(markdown, size));
    }

    inline RichString operator ""_html(const char* html, usize size) {
        return RichString::ParseHtml(Str::Slice(html, size));
    }
}
