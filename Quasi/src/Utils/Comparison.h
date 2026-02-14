#pragma once
#include "Func.h"

namespace Quasi {
    namespace Cmp { enum Comparison : int; }

    template <class T>
    concept Comparable = requires (const T& lhs, const T& rhs) {
        { lhs.Cmp(rhs) } -> ConvTo<Cmp::Comparison>;
    } || requires (const T& lhs, const T& rhs) {
        { lhs <=> rhs };
    };

    template <class F, class T>
    concept Comparator = Fn<F, Cmp::Comparison, const T&, const T&>;
    template <class F, class T>
    concept EqualPred  = Fn<F, bool, const T&, const T&>;

    namespace Cmp {
        enum Comparison : int {
            LESS    = -1,
            EQUAL   = 0,
            GREATER = 1
        };

        inline bool operator< (Comparison cmp, int) { return cmp == LESS; }
        inline bool operator<=(Comparison cmp, int) { return cmp != GREATER; }
        inline bool operator==(Comparison cmp, int) { return cmp == EQUAL; }
        inline bool operator!=(Comparison cmp, int) { return cmp != EQUAL; }
        inline bool operator>=(Comparison cmp, int) { return cmp != LESS; }
        inline bool operator> (Comparison cmp, int) { return cmp == GREATER; }
        inline bool operator- (Comparison cmp)      { return (Comparison)(-(int)cmp); }
        Comparison IntoComparison(const auto& cmp) { return (Comparison)((cmp > 0) - (cmp < 0)); }

        template <Comparable T>
        Comparison Between(const T& left, const T& right) {
            if constexpr (requires { { left.Cmp(right) } -> ConvTo<Comparison>; }) {
                return left.Cmp(right);
            } else {
                return IntoComparison(left <=> right);
            }
        }
#pragma region Comparison Functors
        struct LessThan      { bool operator()(const auto& left, const auto& right) const { return left <  right; } };
        struct LessEquals    { bool operator()(const auto& left, const auto& right) const { return left <= right; } };
        struct GreaterEquals { bool operator()(const auto& left, const auto& right) const { return left >= right; } };
        struct GreaterThan   { bool operator()(const auto& left, const auto& right) const { return left <  right; } };

        template <class T>
        struct LessThanWith  { const T& x; bool operator()(const auto& other) const { return other < x; } };
        template <class F>
        struct LessThanKeyed {
            F& keyer;
            bool operator()(const auto& lhs, const auto& rhs) { return keyer(lhs) < keyer(rhs); }
        };
        template <class F, class T>
        struct LessThanWithKeyed {
            F& keyer;
            const T& target;
            bool operator()(const auto& x) { return keyer(x) < target; }
        };

        struct Equality      { bool operator()(const auto& left, const auto& right) const { return left == right; } };
        template <class T>
        struct Equals        { const T& x; auto operator()(const auto& other) const { return x == other; } };
        template <class F>
        struct EqualityKeyed {
            F& keyer;
            bool operator()(const auto& lhs, const auto& rhs) { return keyer(lhs) == keyer(rhs); }
        };

        struct Compare       { Comparison operator()(const auto& left, const auto& right) const { return IntoComparison(left <=> right); } };
        template <class T>
        struct ComparedTo    { const T& rhs; Comparison operator()(const auto& other) const { return IntoComparison(other <=> rhs); } };
        template <class F>
        struct CompareKeyed {
            F& keyer;
            Comparison operator()(const auto& lhs, const auto& rhs) { return IntoComparison(keyer(lhs) <=> keyer(rhs)); }
        };
        template <class F, class T>
        struct ComparedToKeyed {
            F& keyer;
            T target;
            Comparison operator()(const auto& x) { return IntoComparison(keyer(x) <=> target); }
        };
#pragma endregion
    }

    using Cmp::Comparison;
}