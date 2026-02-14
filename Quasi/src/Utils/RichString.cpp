// ReSharper disable CppClangTidyCppcoreguidelinesAvoidGoto
#include "RichString.h"

#include <format>
#include <algorithm>

#include "Match.h"

namespace Quasi::Text {
    RichString RichString::ParseMarkdown(Str markdown) {
        // *           (N)-> IS SPECIAL CHAR ----IS WHITESPACE ------> [ {V} IS '*' (N)               (Y)TOGGLE BOLD
        // * START -> IS '\'                      (N)->START LINE      [       (Y)---+----> IS NEXT CHAR '*' |-----------v
        // *   ^       (Y)-> ADD NEXT CHAR --v          = TRUE         [ {V} IS '~,~'                 (N)TOGGLE ITALIC   |
        // *   ^-----------------------------|                         [       (Y)-------TOGGLE STRIKETHROUGH v          |
        // *   ^  [ { } REPLACE '---' -> HORIZ_LINE (ALT TEXT '-')   ] [ { } IS '>' && START LINE         |   |          |
        // *   ^  [ { } IF '-' && START LINE -> LIST (ALT TEXT '\t.' ] [       (Y)-------TOGGLE QUOTE UNTIL NEWLINE v    |
        // *   ^  [ { } IF '+' && START LINE -> LIST (NO ALT TEXT)   ] [ { } IS '```' && START LINE       |   |     |    |
        // *   ^  [ { } IF '- [( |x)]' -> CHECKBOX (ALT TEXT '#/v'   ] [       (Y)-------TOGGLE CODE BLOCK && SET LANG v |
        // *   ^  [ { } IF '<...>' -> PARSE TAG (UNSUCCESSFUL -> RAW)] [ { } IS '`' && NOT AFTER          |   |     |  | |
        // *   ^  [ { } IF '\n\n' -> NEW PARA (ALT TEXT '\n')        ] [      (Y)-------TOGGLE CODE BLOCK && LANG=0 v  | |
        // *   ^-------------------------------[GO BACK TO START]-----------------------------------------<---<-----<--<-|
        // *   
        // *   =============[ MARKDOWN PARSING CONTROL FLOW ]=====================================================
        
        enum State {
            NONE, LITERAL, SKIP
        };
        enum StyleFlags {
            NO_FLAGS = 0,
            BOLD_FLAG = 1 << 1,
            ITALIC_FLAG = 1 << 2,
            STRIKE_FLAG = 1 << 3,
            QUOTE_FLAG = 1 << 4, 
            MONOSPACE_FLAG = 1 << 5,
        };
        
        int state = NONE, styleFlags = NO_FLAGS;
        bool isStartLine = true;
        RichString str;
        for (; markdown; markdown.Advance(1)) {
            const char c = markdown.First();

            if (state == SKIP) {
                state = NONE;
                continue;
            }
            
            if (state == LITERAL) {
                state = NONE;
                if (c == '\n') {
                    str.Append('\\');
                    goto end;
                }
                goto rawChar;
            }

            if (state == NONE) {
                if (c == '\\') {
                    state = LITERAL;
                    goto end;
                }

                if (markdown.StartsWith("\n\n")) {
                    markdown.Advance(1);
                    goto newLine;
                }

                if (c == '`') {
                    const OptionUsize endTick = markdown.Tail().Find('`');
                    if (!endTick) {
                        state = LITERAL;
                        goto rawChar;
                    }
                    str.AddTag(Style::CODE_BLOCK, { 1 });
                    str.Append(markdown.Substr(1, *endTick));
                    str.AddTag(Style::CODE_BLOCK, { 0 });
                    markdown.Advance(*endTick + 1);
                    goto end;
                }

                if (c == '*') { // try bold or italic
                    if (markdown.Length() <= 1) goto rawChar;
                    const bool boldOrItalic = markdown[1] == '*';
                    const bool isSet = styleFlags & 2 << (1 - boldOrItalic);
                    if (!isSet) {
                        if (!markdown.Skip(1 + boldOrItalic).Contains(Str::Slice("**", 1 + boldOrItalic))) {
                            state = boldOrItalic ? SKIP : LITERAL;
                            goto rawChar;
                        }
                    }
                    state = boldOrItalic ? SKIP : NONE;
                    str.AddTag(boldOrItalic ? Style::BOLD : Style::ITALIC, !isSet);
                    styleFlags ^= 2 << (1 - boldOrItalic);
                    goto end;
                }

                if (markdown.StartsWith("~~")) {
                    const bool isSet = styleFlags & STRIKE_FLAG;
                    if (!isSet) {
                        if (!markdown.Skip(2).Contains("~~")) {
                            state = LITERAL;
                            goto rawChar;
                        }
                    }
                    state = SKIP;
                    str.AddTag(Style::STRIKETHROUGH, !isSet);
                    styleFlags ^= STRIKE_FLAG;
                    goto end;
                }

                if (c == '>' && isStartLine) {
                    str.AddTag(Style::BLOCKQUOTE, true);
                    styleFlags |= QUOTE_FLAG;
                }

                if (markdown.StartsWith("<br>")) {
                    str.Append('\n');
                    markdown.Advance(3);
                    goto end;
                }
            }

            rawChar:
            str.Append(c);
            goto end;

            newLine:
            isStartLine = true;
            str.AddTag(Style::ALT_TEXT, true);
            str.Append('\n');
            str.AddTag(Style::ALT_TEXT, false);
            goto end;

            end:
            if (!std::isspace(c)) {
                isStartLine = false;
                if (styleFlags & QUOTE_FLAG) {
                    styleFlags &= ~QUOTE_FLAG;
                    str.AddTag(Style::BLOCKQUOTE, false);
                }
            }
        }

        if (styleFlags & BOLD_FLAG)   str.AddTag(Style::BOLD,          false);
        if (styleFlags & ITALIC_FLAG) str.AddTag(Style::ITALIC,        false);
        if (styleFlags & STRIKE_FLAG) str.AddTag(Style::STRIKETHROUGH, false);
        if (styleFlags & QUOTE_FLAG)  str.AddTag(Style::BLOCKQUOTE,    false);

        return str;
    }

    RichString RichString::ParseHtml(Str html) {
        // TODO: add plz
        (void)html;
        return {};
    }

    void RichString::AddTag(Style s, IList<byte> data) {
        rawString.Append(DELIMITER);
        stylings.Push((byte)s.Ord());
        for (byte byte : data) stylings.Push(byte);
    }

    void RichString::AddTag(Style s, bool state) {
        rawString.Append(DELIMITER);
        stylings.Push((byte)(s + !state).Ord()); // off is encoded as the next enum val
    }

    uint RichString::Lines() const {
        return rawString.CountLines();
    }

    RichString::Iter RichString::begin() const {
        return ++Iter { .str = *this, .iter = rawString.Data() - 1 };
    }
    RichString::Iter RichString::end() const {
        return { .str = *this, .iter = rawString.DataEnd() };
    }

    void RichString::StyleState::AddState(Style s, const byte* data) {
        switch (s->byteEncodingSize) {
            case 0: {
                const u32 i = s.Ord() / 2;
                styleFlags = (byte)((styleFlags & ~(1 << i)) | (s->isOn << i));
            }
            return;
            case 1: Qmatch$ (s, (
                case (Style::FONT_TYPE)  { font = *data; },
                case (Style::CODE_BLOCK) { codeLang = *data; },
                case (Style::ALIGNMENT)  { alignment = *data; },
                else;
            ))
            return;
            case 2:
                if (s == Style::FONT_SIZE) fontSizeP64 = (short)((data[0] << 8) + data[1]); break;
            case 4: Qmatch$ (s, (
                case (Style::TEXT_COLOR)      { std::copy_n(data, 4, color);     },
                case (Style::HIGHLIGHT_COLOR) { std::copy_n(data, 4, highlight); },
                else;
            ))
            default:;
        }
    }

    RichString::RichChar RichString::Iter::operator*() const {
        return { .c = *iter, .style = currentState };
    }

    RichString::Iter& RichString::Iter::operator++() {
        ++iter;
        for (; *iter == DELIMITER; ++iter) {
            const byte* begin = str->stylings.Data() + styleOffset;
            const Style s = Style::FromOrd(*begin);
            currentState.AddState(s, begin + 1);
            styleOffset += s->byteEncodingSize + 1;
        }
        return *this;
    }
}
