#pragma once
#include <chrono>
#include <source_location>

#include "Utils/Text.h"
#include "Utils/Text/Num.h"
#include "Utils/Enum.h"
#include "Timer.h"

namespace Quasi::Debug {
    void DebugBreak();

    struct SeverityData {
        Str name;
        Text::ConsoleColor color;

        Text::ColoredStr ColoredName() const { return { color, name }; }

        QDefineEnum$(Severity,
            (OFF,      ("OFF",      Text::RESET))
            (TRACE,    ("TRACE",    FgColor(Text::GREEN)))
            (DEBUG,    ("DEBUG",    FgColor(Text::CYAN)))
            (INFO,     ("INFO",     Text::RESET))
            (WARN,     ("WARN",     FgColor(Text::YELLOW)))
            (ERROR,    ("ERROR",    FgColor(Text::RED)))
            (CRITICAL, ("CRITICAL", Bold(Text::FG_RED))),
        COMPARABLE, ("NEVER", Text::GRAY))
    };

    using SourceLoc = std::source_location;

    // captures source location
    struct FmtStr {
        Str fmt;
        SourceLoc loc;
        FmtStr(const char* f, const SourceLoc& l = SourceLoc::current()) : fmt(f), loc(l) {}
        FmtStr(Str f, const SourceLoc& l = SourceLoc::current()) : fmt(f), loc(l) {}
        // consteval FmtStr(const char* f, const SourceLoc& l = SourceLoc::current()) : fmt(f), loc(l) {}
    };

    struct LogEntry {
        String log;
        Severity severity;
        DateTime time;
        SourceLoc fileLoc;
    };

    class Logger {
        Text::StringWriter logOut;
        Text::ColoredStr name = { Text::RESET, "LOG" };
        Severity filterLevel = Severity::OFF, breakLevel = Severity::ERROR;
        Vec<LogEntry> logs;

        bool shortenFileNames : 1 = true;
        bool includeFunction : 1 = true;
        bool recordLogs : 1 = false;
        u32 lPad = 50;

    public:
        static Logger InternalLog;
#ifdef NDEBUG
        static constexpr bool DEBUG = false;
#else
        static constexpr bool DEBUG = true;
#endif
        explicit Logger(Text::StringWriter out = Text::StringWriter::WriteToConsole()) : logOut(out) {}

        static bool Overrides(Severity filter, Severity log) { return log >= filter; }
        bool Overrides(Severity s) const { return Overrides(filterLevel, s); }
        void SetFilter(Severity s) { filterLevel = s; }
        void SetBreakLevel(Severity s) { breakLevel = s; }

        void SetName(Str newName) { name.text = newName; }
        void SetNameColor(const Text::ConsoleColor col) { name.color = col; }
        void SetShortenFile(const bool flag) { shortenFileNames = flag; }
        void SetIncludeFunc(const bool flag) { includeFunction = flag; }
        void SetRecordLogs(const bool flag) { recordLogs = flag; }
        void SetLocPad(const u32 pad) { lPad = pad; }

        void FmtLog(Text::StringWriter output, const LogEntry& log) const;
        void FmtLog(Text::StringWriter output, Str log, Severity severity, DateTime time, const SourceLoc& fileLoc) const;
        Str FmtFile(Str fullname) const;
        void FmtSourceLoc(Text::StringWriter output, const SourceLoc& loc) const;
        void LogNoOut  (Severity sv, Str s, const SourceLoc& loc = SourceLoc::current());
        void ConsoleLog(Severity sv, Str s, const SourceLoc& loc = SourceLoc::current());
        void Log       (Severity sv, Str s, const SourceLoc& loc = SourceLoc::current());

        void AssertMsg(bool assert, Str msg, const SourceLoc& loc = SourceLoc::current());

        void WriteAllLogs(Text::StringWriter out, Severity filter = Severity::NONE);

        template <class ...Ts> void LogFmt(Severity s, const FmtStr& fmt, const Ts&... args) {
            this->Log(s, Text::Format(fmt.fmt, args...), fmt.loc);
        }

        template <class ...Ts> void Assert(bool assert, const FmtStr& fmt, const Ts&... args) {
            if (assert) return;
            this->AssertMsg(false, Text::Format(fmt.fmt, args...), fmt.loc);
        }

        template <class T>
        void AssertEq(const T& val, const T& cmp, const SourceLoc& loc = SourceLoc::current()) {
            this->Assert(val == cmp,
                { "Left operand {0}({1}) is not equal to Right Operand {0}({2})", loc },
                Text::TypeName<T>(), val, cmp
            );
        }

        template <class T>
        void AssertNeq(const T& val, const T& cmp, const SourceLoc& loc = SourceLoc::current()) {
            this->Assert(val != cmp,
                { "Left operand {0}({1}) is equal to Right Operand {0}({2})", loc },
                Text::TypeName<T>(), val, cmp
            );
        }

        void NoOp() const {}

        static Logger& GetInternalLog();

        static void WinEnableANSI();
    };

    inline void SetFilter(Severity s) { Logger::GetInternalLog().SetFilter(s); }
    inline void SetBreakLevel(Severity s) { Logger::GetInternalLog().SetBreakLevel(s); }

    inline void SetName(Str newName) { Logger::GetInternalLog().SetName(newName); }
    inline void SetNameColor(const Text::ConsoleColor col) { Logger::GetInternalLog().SetNameColor(col); }
    inline void SetShortenFile(const bool flag) { Logger::GetInternalLog().SetShortenFile(flag); }
    inline void SetIncludeFunc(const bool flag) { Logger::GetInternalLog().SetIncludeFunc(flag); }
    inline void SetLocPad(const int pad) { Logger::GetInternalLog().SetLocPad(pad); }

    inline void Log(Severity sv, Str s, const SourceLoc& loc = SourceLoc::current()) { Logger::GetInternalLog().Log(sv, s, loc); }

    inline void AssertMsg(bool assert, Str msg, const SourceLoc& loc = SourceLoc::current()) { Logger::GetInternalLog().AssertMsg(assert, msg, loc); }

    inline void Write(Text::StringWriter out, Severity filter = Severity::NONE) { Logger::GetInternalLog().WriteAllLogs(out, filter); }

    template <class ...Ts> void LogFmt(Severity s, const FmtStr& fmt, const Ts&... args) {
        Logger::GetInternalLog().LogFmt(s, fmt, args...);
    }

    template <class ...Ts> void Assert(bool assert, const FmtStr& fmt, const Ts&... args) {
        Logger::GetInternalLog().Assert(assert, fmt.fmt, args...);
    }

    template <class T>
    void AssertEq(const T& val, const T& cmp, const SourceLoc& loc = SourceLoc::current()) { Logger::GetInternalLog().AssertEq(val, cmp, loc); }

    template <class T>
    void AssertNeq(const T& val, const T& cmp, const SourceLoc& loc = SourceLoc::current()) { Logger::GetInternalLog().AssertNeq(val, cmp, loc); }

    inline void NoOp() {}

#ifdef NDEBUG
    #define QTrace$     NoOp() Q_EAT
    #define QDebug$     NoOp() Q_EAT
    #define QInfo$      NoOp() Q_EAT
    #define QWarn$      NoOp() Q_EAT
    #define QError$     NoOp() Q_EAT
    #define QCritical$  NoOp() Q_EAT
    #define QAssert$    NoOp() Q_EAT
    #define QAssertEq$  NoOp() Q_EAT
    #define QAssertNeq$ NoOp() Q_EAT
    #define QAssertMsg$ NoOp() Q_EAT
#else
    #define QTrace$(...)    LogFmt(Quasi::Debug::Severity::TRACE,    __VA_ARGS__)
    #define QDebug$(...)    LogFmt(Quasi::Debug::Severity::DEBUG,    __VA_ARGS__)
    #define QInfo$(...)     LogFmt(Quasi::Debug::Severity::INFO,     __VA_ARGS__)
    #define QWarn$(...)     LogFmt(Quasi::Debug::Severity::WARN,     __VA_ARGS__)
    #define QError$(...)    LogFmt(Quasi::Debug::Severity::ERROR,    __VA_ARGS__)
    #define QCritical$(...) LogFmt(Quasi::Debug::Severity::CRITICAL, __VA_ARGS__)
    #define QAssert$        Assert
    #define QAssertEq$      AssertEq
    #define QAssertNeq$     AssertNeq
    #define QAssertMsg$     AssertMsg
#endif
}

namespace Quasi {
    template <class T, class Super> T& INullable<T, Super>::Assert() { return QGetterMut$(Assert); }
    template <class T, class Super> const T& INullable<T, Super>::Assert() const {
        Debug::Assert(HasValue(), "{} doesn't have a value", Text::TypeName<Super>());
        return Unwrap();
    }

    template <class T, class Super> T& INullable<T, Super>::Assert(Str msg) { return QGetterMut$(Assert, msg); }
    template <class T, class Super> const T& INullable<T, Super>::Assert(Str msg) const {
        Debug::Assert(HasValue(), msg);
        return Unwrap();
    }

    template <class T, class Super> T& INullable<T, Super>::Assert(auto&& assertfn) { return QGetterMut$(Assert, assertfn); }
    template <class T, class Super> const T& INullable<T, Super>::Assert(auto&& assertfn) const {
        if (IsNull()) assertfn();
        return Unwrap();
    }
}

namespace Quasi::Text {
    template <>
    struct Formatter<Debug::DateTime> {
        static usize FormatTo(StringWriter out, const Debug::DateTime& time, Str fmt);
    };
}