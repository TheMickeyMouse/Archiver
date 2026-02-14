#pragma once
#include "Comparison.h"
#include "Type.h"
#include "Option.h"
#include "Range.h"
#include "Tuple.h"

namespace Quasi {
    template <class T, class Super> struct IIterator;

    template <class C>             using   CollectionItem = typename C::Item;
    template <class I>             concept IteratorAny    = Extends<I, IIterator<CollectionItem<I>, I>>;
    template <class I, class Item> concept Iterator       = IteratorAny<I> && ConvTo<CollectionItem<I>, Item>;

    inline struct IteratorEndMarker {} IteratorEnd;

#pragma region Iter Declarations
    namespace Iter {
        template <class It>          struct EnumerateIter;
        template <class It, class F> struct MapIter;
        template <class Vw>          struct SplitIter;
    }
#pragma endregion

    template <class T, class Super>
    struct ICollection {
        using Item = T;
        friend Super;
    protected:
        Super& super() { return *static_cast<Super*>(this); }
        const Super& super() const { return *static_cast<const Super*>(this); }

        Iterator<const T&> auto IterImpl() const = delete;
        Iterator<T&> auto IterMutImpl() = delete;
    public:
        Iterator<const T&> auto Iter() const { return super().IterImpl(); }
        Iterator<T&> auto IterMut() requires IsMut<T> { return super().IterMutImpl(); }

        // Legacy methods:
        // CIter<Super> begin();
        // IteratorEndMarker end();
        Iterator<const T&> auto begin()  const { return Iter(); }
        Iterator<const T&> auto cbegin() const { return Iter(); }
        Iterator<T&> auto begin() requires IsMut<T> { return IterMut(); }
        IteratorEndMarker end()  const { return IteratorEnd; }
        IteratorEndMarker cend() const { return IteratorEnd; }
    public:


    };

    template <class C>             concept CollectionAny = Implements<C, ICollection, CollectionItem<C>>;
    template <class C, class Item> concept Collection    = CollectionAny<C> && ConvTo<CollectionItem<C>, Item>;

    template <class T, class Super>
    struct IIterator : ICollection<T, Super> {
        using Item = T;
        friend Super;
        friend ICollection<T, Super>;
    protected:
        Super& super() { return *static_cast<Super*>(this); }
        const Super& super() const { return *static_cast<const Super*>(this); }

        Super IterImpl() const { return super(); }
        Super IterMutImpl() requires IsMutRef<T> { return super(); }

        T CurrentImpl() const = delete;
        void AdvanceImpl() = delete;
        bool CanNextImpl() const = delete;
    public:
        Super IterMut() requires IsMutRef<T> { return super(); }

        T Current() const { return super().CurrentImpl(); }
        void Advance() { return super().AdvanceImpl(); }
        bool CanNext() const { return super().CanNextImpl(); }

        T operator*() const { return Current(); }
        Super& operator++() { Advance(); return super(); }
        friend bool operator==(const Super& it, const IteratorEndMarker&) { return !it.CanNext(); }
        friend bool operator!=(const Super& it, const IteratorEndMarker&) { return  it.CanNext(); }

        usize Count() const {
            usize count = 0;
            for (; CanNext(); Advance()) ++count;
            return count;
        }
        usize CountOccurs(Predicate<Item> auto&& pred) {
            usize count = 0;
            for (; CanNext(); Advance()) {
                if (pred(Current())) ++count;
            }
            return count;
        }
        usize CountNumberOf(const Item& i) { return CountOccurs(Cmp::Equals { i }); }

        usize AdvanceBy(usize n) {
            for (usize i = 0; i < n; ++i) {
                if (!CanNext()) return i;
                Advance();
            }
            return n;
        }

        Option<Item> Last() {
            Option<Item> last = nullptr;
            while (CanNext()) {
                last = Current();
                Advance();
            }
            return last;
        }

        Option<Item> Nth(usize n) {
            if (AdvanceBy(n) < n || !CanNext()) return nullptr;
            return Current();
        }

        void ForEach(Fn<void, const Item&> auto&& fn) {
            for (Item i : super())
                fn(i);
        }

        Option<Item> Reduce(Fn<Item, Item, const Item&> auto&& reducer) {
            if (!CanNext()) return nullptr;
            Item acc = Current();
            Advance();
            for (; CanNext(); Advance()) {
                acc = reducer(std::move(acc), Current());
            }
            return acc;
        }

        template <class R> R Reduce(Fn<R, R, const Item&> auto&& reducer, R starting) {
            for (; CanNext(); Advance()) {
                starting = reducer(std::move(starting), Current());
            }
            return starting;
        }
        Option<Item> Sum() { return Reduce(Operators::Add {}); }
        Item         Sum(Item begin) { return Reduce(Operators::Add {}, begin); }
        Option<Item> Min() { return Reduce(Qfn$(std::min)); }
        Option<Item> Max() { return Reduce(Qfn$(std::max)); }

        bool All(Predicate<Item> auto&& pred = Combinate::Identity {}) {
            for (; CanNext(); Advance())
                if (!pred(Current())) return false;
            return true;
        }
        bool Any(Predicate<Item> auto&& pred = Combinate::Identity {}) {
            for (; CanNext(); Advance())
                if (pred(Current())) return true;
            return false;
        }

        Option<Item> Find(Predicate<Item> auto&& pred) {
            for (; CanNext(); Advance()) {
                const Item i = Current();
                if (pred(i)) return i;
            }
            return nullptr;
        }

        template <Collection<RemQual<T>> C> C Collect() {
            C collection;
            for (auto&& x : *this) {
                collection.Push(x);
            }
            return collection;
        }

        Iter::EnumerateIter<Super> Enumerate() const&;
        Iter::EnumerateIter<Super> Enumerate() &&;
        template <FnArgs<T> F> Iter::MapIter<Super, F> Map(F&& fn) const&;
        template <FnArgs<T> F> Iter::MapIter<Super, F> Map(F&& fn) &&;
        // TODO:
        // StepBy
        // Chain
        // Zip
        // ForEach
        // Filter
        // FilterMap
        // SkipWhile
        // TakeWhile
        // MapWhile
        // Take
        // Scan
        // FlatMap
        // Flatten
        // MapWindows
        // CollectPartitioned
        // Acc
    };


    inline struct WrapMarker {} Wrap;
    struct WrappingIndex { isize index; usize operator()(usize len) const { return (index % (isize)len + (isize)len) % (isize)len; } };

    WrappingIndex operator%(Integer auto i, WrapMarker) { return { (isize)i }; }

#pragma region Continuous
    template <class T> struct Span;
    template <class T> struct Ref;
    template <class T> struct BufferIterator;
    struct Str;
    struct StrMut;

#define mut requires IsMut<T>
    template <class T, class Super>
    struct IContinuousCollection : ICollection<T, Super> {
        friend ICollection<T, Super>;
        using MutT = RemConst<T>;
    protected:
        Super& super() { return *static_cast<Super*>(this); }
        const Super& super() const { return *static_cast<const Super*>(this); }

        RemRef<T>*       DataImpl() mut   = delete;
        const RemRef<T>* DataImpl() const = delete;
        usize LengthImpl() const = delete;

        T& AtImpl(usize i) { return Data()[i]; }
        const T& AtImpl(usize i) const { return Data()[i]; }

        T& AtWrapImpl(WrappingIndex i) { return Data()[i(Length())]; }
        const T& AtWrapImpl(WrappingIndex i) const { return Data()[i(Length())]; }
    public:
        BufferIterator<T&>       IterMut()      { return super().AsSpan().IterMut(); }
        BufferIterator<const T&> Iter()   const { return super().AsSpan().Iter(); }

        BufferIterator<T&>       begin()  mut   { return super().IterMut(); }
        BufferIterator<const T&> begin()  const { return super().Iter(); }
        BufferIterator<const T&> cbegin() const { return super().Iter(); }

        RemRef<T>*       Data() mut   { return super().DataImpl(); }
        const RemRef<T>* Data() const { return super().DataImpl(); }
        RemRef<T>*       DataEnd() mut   { return super().DataImpl() + super().LengthImpl(); }
        const RemRef<T>* DataEnd() const { return super().DataImpl() + super().LengthImpl(); }
        usize Length() const { return super().LengthImpl(); }

        Hashing::Hash GetHashCode() const { return AsSpan().GetHashCode(); }

        Span<const T> AsSpan()  const { return Span<const T>::Slice(Data(), Length()); }
        Span<T>       AsSpan()    mut { return Span<MutT>   ::Slice(Data(), Length()); }
        Span<MutT>    AsSpanMut() mut { return Span<MutT>   ::Slice(Data(), Length()); }

        Str                       AsStr()      const requires SameAs<const char, const T>;
        StrMut                    AsStrMut()   requires SameAs<char, T>;
        Span<const byte>          AsBytes()    const;
        Span<byte>                AsBytesMut() mut;

        usize ByteSize()         const { return super().AsSpan().ByteSize(); }
        bool IsEmpty()           const { return super().AsSpan().IsEmpty(); }
        explicit operator bool() const { return (bool)super().AsSpan(); }

        MutT&    FirstMut() mut { return super().AsSpanMut().FirstMut(); }
        MutT&    LastMut()  mut { return super().AsSpanMut().LastMut(); }
        const T& First()  const { return super().AsSpan().First(); }
        const T& Last()   const { return super().AsSpan().Last(); }
        OptRef<MutT>    TryFirstMut() mut { return super().AsSpanMut().TryFirstMut(); }
        OptRef<MutT>    TryLastMut()  mut { return super().AsSpanMut().TryLastMut(); }
        OptRef<const T> TryFirst()  const { return super().AsSpan().TryFirst(); }
        OptRef<const T> TryLast()   const { return super().AsSpan().TryLast(); }

        Span<MutT> SubspanMut(usize start)              mut { return super().AsSpan().SubspanMut(start); }
        Span<MutT> SubspanMut(usize start, usize count) mut { return super().AsSpan().SubspanMut(start, count); }
        Span<MutT> SubspanMut(zRange range)             mut { return super().AsSpan().SubspanMut(range); }
        Span<MutT> FirstMut(usize num)                  mut { return super().AsSpan().FirstMut(num); }
        Span<MutT> SkipMut(usize len)                   mut { return super().AsSpan().SkipMut(len); }
        Span<MutT> TailMut()                            mut { return super().AsSpan().TailMut(); }
        Span<MutT> LastMut(usize num)                   mut { return super().AsSpan().LastMut(num); }
        Span<MutT> TruncMut(usize len)                  mut { return super().AsSpan().TruncMut(len); }
        Span<MutT> InitMut()                            mut { return super().AsSpan().InitMut(); }
        Tuple<MutT&,      Span<MutT>>        SplitFirstMut()                         mut { return super().AsSpan().SplitFirstMut(); }
        Tuple<Span<MutT>, MutT&>             SplitLastMut()                          mut { return super().AsSpan().SplitLastMut(); }
        Tuple<Span<MutT>, Span<MutT>>        CutAtMut(usize at)                      mut { return super().AsSpan().CutAtMut(at); }
        Tuple<Span<MutT>, Span<MutT>>        SplitAtMut(usize at)                    mut { return super().AsSpan().SplitAtMut(at); }
        Tuple<Span<MutT>, MutT&, Span<MutT>> PartitionAtMut(usize at)                mut { return super().AsSpan().PartitionAtMut(at); }
        Tuple<Span<MutT>, Span<MutT>>        SplitOnceOnMut(Predicate<T> auto&& pred)    mut { return super().AsSpan().SplitOnceOnMut(pred); }
        Tuple<Span<MutT>, Span<MutT>>        RevSplitOnceOnMut(Predicate<T> auto&& pred) mut { return super().AsSpan().RevSplitOnceOnMut(pred); }
        Tuple<Span<MutT>, Span<MutT>>        SplitOnceMut(const T& sep)                  mut { return super().AsSpan().SplitOnceMut(sep); }
        Span<const T> Subspan(usize start)              const { return super().AsSpan().Subspan(start); }
        Span<const T> Subspan(usize start, usize count) const { return super().AsSpan().Subspan(start, count); }
        Span<const T> Subspan(zRange range)             const { return super().AsSpan().Subspan(range); }
        Span<const T> First(usize num)                  const { return super().AsSpan().First(num); }
        Span<const T> Skip(usize len)                   const { return super().AsSpan().Skip(len); }
        Span<const T> Tail()                            const { return super().AsSpan().Tail(); }
        Span<const T> Last(usize num)                   const { return super().AsSpan().Last(num); }
        Span<const T> Trunc(usize len)                  const { return super().AsSpan().Trunc(len); }
        Span<const T> Init()                            const { return super().AsSpan().InitMut(); }
        Tuple<const T&,      Span<const T>>           SplitFirst()                         const { return super().AsSpan().SplitFirst(); }
        Tuple<Span<const T>, const T&>                SplitLast()                          const { return super().AsSpan().SplitLast(); }
        Tuple<Span<const T>, Span<const T>>           CutAt(usize at)                      const { return super().AsSpan().CutAt(at); }
        Tuple<Span<const T>, Span<const T>>           SplitAt(usize at)                    const { return super().AsSpan().SplitAt(at); }
        Tuple<Span<const T>, const T&, Span<const T>> PartitionAt(usize at)                const { return super().AsSpan().PartitionAt(at); }
        Tuple<Span<const T>, Span<const T>>           SplitOnceOn(Predicate<T> auto&& pred)    const { return super().AsSpan().SplitOnceOn(pred); }
        Tuple<Span<const T>, Span<const T>>           RevSplitOnceOn(Predicate<T> auto&& pred) const { return super().AsSpan().RevSplitOnceOn(pred); }
        Tuple<Span<const T>, Span<const T>>           SplitOnce(const T& sep)                  const { return super().AsSpan().SplitOnce(sep); }

        Iter::SplitIter<Span<const T>> Split(const T& sep) const {
            return Iter::SplitIter<Span<const T>>::New(*this, Only(sep));
        }
        Iter::SplitIter<Span<const T>> Split(Span<const T> sep) const {
            return Iter::SplitIter<Span<const T>>::New(*this, sep);
        }

        void Swap(usize i, usize j) mut { return super().AsSpanMut().Swap(i, j); }
        void Reverse()              mut { return super().AsSpanMut().Reverse(); }

        bool RefEquals     (Span<const T> other) const { return super().AsSpan().RefEquals(other); }
        bool ContainsBuffer(Span<const T> buf)   const { return super().AsSpan().ContainsBuffer(buf); }
        bool OverlapsBuffer(Span<const T> buf)   const { return super().AsSpan().OverlapsBuffer(buf); }
        bool Equals        (Span<const T> other) const { return super().AsSpan().Equals(other); }
        bool EqualsBy      (Span<const T> other, EqualPred<T> auto&& eq)       const { return super().AsSpan().EqualsByKey(other, eq); }
        bool EqualsByKey   (Span<const T> other, FnArgs<const T&> auto&& keyf) const { return super().AsSpan().EqualsByKey(other, keyf); }
        bool operator==    (Span<const T> other) const { return super().AsSpan() == other; }

        bool All          (Predicate<T> auto&& pred = Combinate::Identity {}) const { return super().AsSpan().All(pred); }
        bool Any          (Predicate<T> auto&& pred = Combinate::Identity {}) const { return super().AsSpan().Any(pred); }
        Comparison Cmp          (Span<const T> other)                               const { return super().AsSpan().Cmp(other); }
        Comparison CmpBy        (Span<const T> other, Comparator<T> auto&& cmp)     const { return super().AsSpan().CmpBy(other, cmp); }
        Comparison CmpByKey     (Span<const T> other, FnArgs<const T&> auto&& keyf) const { return super().AsSpan().CmpByKey(other, keyf); }
        Comparison CmpSized     (Span<const T> other)                               const { return super().AsSpan().CmpSized(other); }
        Comparison CmpSizedBy   (Span<const T> other, Comparator<T> auto&& cmp)     const { return super().AsSpan().CmpSizedBy(other, cmp); }
        Comparison CmpSizedByKey(Span<const T> other, FnArgs<const T&> auto&& keyf) const { return super().AsSpan().CmpSizedByKey(other), keyf; }
        Comparison operator<=>  (Span<const T> other)                               const { return super().AsSpan() <=> other; }

        bool StartsWith(Span<const T> prefix) const { return super().AsSpan().StartsWith(prefix); }
        bool EndsWith  (Span<const T> suffix) const { return super().AsSpan().EndsWith(suffix); }
        Option<Span<MutT>> RemovePrefixMut(Span<const T> prefix) mut   { return super().AsSpan().RemovePrefixMut(prefix); }
        Option<Span<MutT>> RemoveSuffixMut(Span<const T> suffix) mut   { return super().AsSpan().RemoveSuffixMut(suffix); }
        Option<Span<const T>> RemovePrefix(Span<const T> prefix) const { return super().AsSpan().RemovePrefix(prefix); }
        Option<Span<const T>> RemoveSuffix(Span<const T> suffix) const { return super().AsSpan().RemoveSuffix(suffix); }

        OptionUsize Find     (const T& target)          const { return super().AsSpan().Find     (target); }
        OptionUsize RevFind  (const T& target)          const { return super().AsSpan().RevFind  (target); }
        OptionUsize FindIf   (Predicate<T> auto&& pred) const { return super().AsSpan().FindIf   (pred); }
        OptionUsize RevFindIf(Predicate<T> auto&& pred) const { return super().AsSpan().RevFindIf(pred); }
        bool    Contains     (const T& target)          const { return super().AsSpan().Contains     (target); }
        bool    RevContains  (const T& target)          const { return super().AsSpan().RevContains  (target); }
        bool    ContainsIf   (Predicate<T> auto&& pred) const { return super().AsSpan().ContainsIf   (pred); }
        bool    RevContainsIf(Predicate<T> auto&& pred) const { return super().AsSpan().RevContainsIf(pred); }
        OptionUsize   Find   (Span<const T> target)     const { return super().AsSpan().Find       (target); }
        OptionUsize   RevFind(Span<const T> target)     const { return super().AsSpan().RevFind    (target); }
        bool      Contains   (Span<const T> target)     const { return super().AsSpan().Contains   (target); }
        bool      RevContains(Span<const T> target)     const { return super().AsSpan().RevContains(target); }
        Tuple<OptionUsize, OptionUsize> FindOneOf   (Span<const T> anytarget) const { return super().AsSpan().FindOneOf       (anytarget); }
        Tuple<OptionUsize, OptionUsize> RevFindOneOf(Span<const T> anytarget) const { return super().AsSpan().RevFindOneOf    (anytarget); }
        OptionUsize                 ContainsOneOf   (Span<const T> anytarget) const { return super().AsSpan().ContainsOneOf   (anytarget); }
        OptionUsize                 RevContainsOneOf(Span<const T> anytarget) const { return super().AsSpan().RevContainsOneOf(anytarget); }
        Tuple<OptionUsize, OptionUsize> FindOneOf   (Span<const Span<const T>> anytarget) const { return super().AsSpan().FindOneOf       (anytarget); }
        Tuple<OptionUsize, OptionUsize> RevFindOneOf(Span<const Span<const T>> anytarget) const { return super().AsSpan().RevFindOneOf    (anytarget); }
        OptionUsize                 ContainsOneOf   (Span<const Span<const T>> anytarget) const { return super().AsSpan().ContainsOneOf   (anytarget); }
        OptionUsize                 RevContainsOneOf(Span<const Span<const T>> anytarget) const { return super().AsSpan().RevContainsOneOf(anytarget); }

        usize Unaddress      (const T* addr) const { return super().AsSpan().Unaddress(addr); }
        bool  ContainsAddress(const T* addr) const { return super().AsSpan().ContainsAddress(addr); }

        Tuple<bool, usize> BinarySearch(const T& target) const { return super().AsSpan().BinarySearch(target); }
        // assumes ascending order
        Tuple<bool, usize> BinarySearchWith         (Fn<Comparison, const T&> auto&& cmp)           const { return super().AsSpan().BinarySearchWith(cmp); }
        Tuple<bool, usize> BinarySearchByKey        (const T& target, FnArgs<const T&> auto&& keyf) const { return super().AsSpan().BinarySearchByKey(target, keyf); }
        usize              BinaryPartitionPoint     (const T& target)                               const { return super().AsSpan().BinaryPartitionPoint(target); }
        usize              BinaryPartitionPointBy   (Predicate<T> auto&& left)                      const { return super().AsSpan().BinaryPartitionPointBy(left); }
        usize              BinaryPartitionPointByKey(const T& target, FnArgs<const T&> auto&& keyf) const { return super().AsSpan().BinaryPartitionPointByKey(target, keyf); }
        usize              LowerBound               (const T& target)                               const { return super().AsSpan().LowerBound(target); }
        usize              LowerBoundBy             (Fn<Comparison, const T&> auto&& cmp)           const { return super().AsSpan().LowerBoundBy(cmp); }
        usize              LowerBoundByKey          (const T& target, FnArgs<const T&> auto&& keyf) const { return super().AsSpan().LowerBoundByKey(target, keyf); }
        usize              UpperBound               (const T& target)                               const { return super().AsSpan().UpperBound(target); }
        usize              UpperBoundBy             (Fn<Comparison, const T&> auto&& cmp)           const { return super().AsSpan().UpperBoundBy(cmp); }
        usize              UpperBoundByKey          (const T& target, FnArgs<const T&> auto&& keyf) const { return super().AsSpan().UpperBoundByKey(target, keyf); }
        // Range<usize> EqualRange(const T& target) const { return LowerBoundBy(Operators::cmpWith { target }); }
        // Range<usize> EqualRangeBy(Predicate<T> auto&& left) const;
        // Range<usize> EqualRangeByKey(FnArgs<const T&> auto&& keyf) const;

        bool BinaryContains     (const T& target)                                const { return super().AsSpan().BinaryContains(target); }
        bool BinaryContainsKey  (FnArgs<const T&> auto&& keyf, auto&& targetKey) const { return super().AsSpan().BinaryContainsKey(keyf, targetKey); }
        bool BinaryContainsByKey(FnArgs<const T&> auto&& keyf, const T& target)  const { return super().AsSpan().BinaryContainsByKey(keyf, target); }

        void Sort     ()                             mut { return super().AsSpanMut().Sort(); }
        void SortBy   (Comparator<T> auto&& cmp)     mut { return super().AsSpanMut().SortBy(cmp); }
        void SortByKey(FnArgs<const T&> auto&& keyf) mut { return super().AsSpanMut().SortByKey(keyf); }

        // void SortStable() TODO i cant be bothered to do this
        // void SortStableBy(Fn<bool, const T&, const T&> auto&& cmp)
        // void SortStableByKey(auto&& keyf)
        bool IsSorted()                                  const { return super().AsSpan().IsSorted(); }
        bool IsSortedBy(Comparator<T> auto&& cmp)        const { return super().AsSpan().IsSortedBy(cmp); }
        bool IsSortedByKey(FnArgs<const T&> auto&& keyf) const { return super().AsSpan().IsSortedByKey(keyf); }

        usize SortedPartitionPoint     (usize idx)                               mut { return super().AsSpanMut().SortedPartitionPoint(idx); }
        usize SortedPartitionPointBy   (usize idx, Comparator<T> auto&& cmp)     mut { return super().AsSpanMut().SortedPartitionPointBy(idx, cmp); }
        usize SortedPartitionPointByKey(usize idx, FnArgs<const T&> auto&& keyf) mut { return super().AsSpanMut().SortedPartitionPointByKey(idx, keyf); }

        Tuple<Span<MutT>, MutT&, Span<MutT>> SortedPartition     (usize idx)                               mut { return super().AsSpanMut().SortedPartition(idx); }
        Tuple<Span<MutT>, MutT&, Span<MutT>> SortedPartitionBy   (usize idx, Comparator<T> auto&& cmp)     mut { return super().AsSpanMut().SortedPartitionBy(idx, cmp); }
        Tuple<Span<MutT>, MutT&, Span<MutT>> SortedPartitionByKey(usize idx, FnArgs<const T&> auto&& keyf) mut { return super().AsSpanMut().SortedPartitionByKey(idx, keyf); }

        usize PartitionDupPoint  ()                         mut { return super().AsSpanMut().PartitionDupPoint(); }
        usize PartitionDupPointBy(Comparator<T> auto&& cmp) mut { return super().AsSpanMut().PartitionDupPointBy(cmp); }
        Tuple<Span<MutT>, Span<MutT>> PartitionDup  ()                         mut { return super().AsSpanMut().PartitionDup(); }
        Tuple<Span<MutT>, Span<MutT>> PartitionDupBy(Comparator<T> auto&& cmp) mut { return super().AsSpanMut().PartitionDupBy(cmp); }

        void RotateLeft  (usize num) mut { return super().AsSpanMut().RotateLeft(num); }
        void RotateRight (usize num) mut { return super().AsSpanMut().RotateRight(num); }
        void RotateSigned(isize off) mut { return super().AsSpanMut().RotateSigned(off); }
        void RotateSigned(WrappingIndex off) mut { return super().AsSpanMut().RotateSigned(off); }
        void RotateSignedWrapped(isize off)  mut { return super().AsSpanMut().RotateSignedWrapped(off); }

        void Fill     (const T& value)       mut { return super().AsSpanMut().Fill(value); }
        void FillWith (Fn<T> auto&& factory = Combinate::Constructor<T> {}) mut { return super().AsSpanMut().FillWith(factory); }
        void FillDefault() { return super().AsSpan().FillDefault(); }
        void CloneFrom(Span<const T> span)   mut { return super().AsSpanMut().CloneFrom(span); }
        void MoveFrom (Span<MutT> span)      mut { return super().AsSpanMut().MoveFrom(span); }
        // void CopyFromSelf(IntegerRange, usize dest)
        void SwapWith (Span<MutT> span)      mut { return super().AsSpanMut().SwapWith(span); }

        MutT& GetMut(usize i) mut   { return super().AtImpl(i); }
        const T& Get(usize i) const { return super().AtImpl(i); }

        MutT& GetWrapMut(WrappingIndex i) mut   { return Data()[i(Length())]; }
        const T& GetWrap(WrappingIndex i) const { return Data()[i(Length())]; }

        T& operator[](usize i)         { if constexpr (IsMut<T>) return GetMut(i);         else return Get(i); }
        T& operator[](WrappingIndex i) { if constexpr (IsMut<T>) return GetWrapMut(i);     else return GetWrap(i); }
        T& operator[](zRange range)    { if constexpr (IsMut<T>) return SubspanMut(range); else return Subspan(range); }
        const T& operator[](usize i)         const { return Get(i); }
        const T& operator[](WrappingIndex i) const { return GetWrap(i); }
        const T& operator[](zRange range)    const { return Subspan(range); }
    };

    template <class T, class Super>
    struct IContinuousIterator : IIterator<T, Super> {

    };
#undef mut

    template <class C, class Item> concept ContinuousCollection = Implements<C, IContinuousCollection, Item>;
    template <class C> concept ContinuousCollectionAny = Implements<C, IContinuousCollection, CollectionItem<C>>;
#pragma endregion
}
