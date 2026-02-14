#pragma once
#include "ArrayBox.h"
#include "Memory.h"
#include "Iterator.h"
#include "Range.h"
#include "Ref.h"

namespace Quasi {
    namespace Algorithm {};
    template <class T> struct BufferIterator;
    template <class T, usize N> struct Array;

#define mut requires IsMut<T>
    template <class T>
    struct Span : IContinuousCollection<T, Span<T>> {
        friend ICollection<T, Span>;
        friend IContinuousCollection<T, Span>;
        template <class U> friend struct Span;
        using MutT = RemConst<T>;
    private:
        T* data;
        usize size;

        Span(T* dat, usize len) : data(dat), size(len) {}
    public:
        Span() : data(nullptr), size(0) {}
        template <usize N>
        Span(T (&arr)[N]) : data(arr), size(N) {}
        template <ContinuousCollection<RemConst<T>> Coll>
        Span(const Coll& collection) : data(collection.Data()), size(collection.Length()) {}
        template <ContinuousCollection<T> Coll>
        Span(Coll& collection) mut : data(collection.Data()), size(collection.Length()) {}

        template <usize N> static Span FromArray(T (&arr) [N]) { return { arr, N }; }
        static Span Slice(T* dat, usize len) { return { dat, len }; }
        static Span Only(T& obj) { return { &obj, 1 }; }
        static Span Empty() { return {}; }

        template <IsMut U> requires SameAs<T, byte>
        static Span BytesOf(U& memRead)       { return { (byte*)&memRead, sizeof(U) }; }
        template <class U> requires SameAs<T, const byte>
        static Span BytesOf(const U& memRead) { return { (const byte*)&memRead, sizeof(U) }; }

        Str              AsStr() const requires SameAs<const T, const char>;
        StrMut           AsStrMut() requires SameAs<T, char>;
        Span<byte>       AsBytes() mut   { return { Memory::AsBytePtr(data), size * sizeof(T) }; }
        Span<const byte> AsBytes() const { return { Memory::AsBytePtr(data), size * sizeof(T) }; }
        Span<const T>    AsSpan()  const { return *this; }
        Span             AsSpan()  mut   { return *this; }
    protected:
        usize LengthImpl() const { return size; }

        T* DataImpl() { return data; }
        const T* DataImpl() const { return data; }
    public:
        BufferIterator<T&>       IterMut() mut { return { data, data + size }; }
        BufferIterator<const T&> Iter()  const { return { data, data + size }; }
        BufferIterator<T>        IntoIter()    { return { data, data + size }; }

        usize ByteSize()   const { return size * sizeof(T); }
        bool IsEmpty()     const { return size == 0; }
        explicit operator bool() const { return !IsEmpty(); }

        MutT& FirstMut() mut   { return data[0]; }
        MutT& LastMut()  mut   { return data[size - 1]; }
        const T& First() const { return data[0]; }
        const T& Last()  const { return data[size - 1]; }
        OptRef<MutT> TryFirstMut() mut   { return size ? OptRefs::SomeRef(data[0])        : nullptr; }
        OptRef<MutT> TryLastMut()  mut   { return size ? OptRefs::SomeRef(data[size - 1]) : nullptr; }
        OptRef<const T> TryFirst() const { return size ? OptRefs::SomeRef(data[0])        : nullptr; }
        OptRef<const T> TryLast()  const { return size ? OptRefs::SomeRef(data[size - 1]) : nullptr; }

        Span<MutT> SubspanMut(usize start)              mut { return { data + start, size - start }; }
        Span<MutT> SubspanMut(usize start, usize count) mut { return { data + start, count }; }
        Span<MutT> SubspanMut(zRange range)             mut { return { data + range.min, range.max - range.min }; }
        Span<MutT> FirstMut(usize num)                  mut { return { data, num }; }
        Span<MutT> SkipMut(usize len)                   mut { return { data + len, size - len }; }
        Span<MutT> TailMut()                            mut { return { data + 1, size - 1 }; }
        Span<MutT> LastMut(usize num)                   mut { return { data + size - num, num }; }
        Span<MutT> TruncMut(usize len)                  mut { return { data, size - len }; }
        Span<MutT> InitMut()                            mut { return { data, size - 1 }; }
        Tuple<MutT&,      Span<MutT>> SplitFirstMut()       mut { return { data[0], TailMut() }; }
        Tuple<Span<MutT>, MutT&>      SplitLastMut()        mut { return { InitMut(), data[size - 1] }; }
        Tuple<Span<MutT>, Span<MutT>> CutAtMut(usize at)    mut { return { FirstMut(at), SkipMut(at) }; }
        Tuple<Span<MutT>, Span<MutT>> SplitAtMut(usize at)  mut { return { FirstMut(at), SkipMut(at + 1) }; }
        Tuple<Span<MutT>, MutT&, Span<MutT>> PartitionAtMut(usize at) mut { return { FirstMut(at), data[at], SkipMut(at + 1) }; }
        Tuple<Span<MutT>, Span<MutT>> SplitOnceOnMut(Predicate<T> auto&& pred) mut {
            const OptionUsize i = FindIf(pred);
            return i ? SplitAtMut(i) : Tuple { *this, Span<MutT>::Empty() };
        }
        Tuple<Span<MutT>, Span<MutT>> RevSplitOnceOnMut(Predicate<T> auto&& pred) mut {
            const OptionUsize i = RevFindIf(pred);
            return i ? SplitAtMut(i) : Tuple { Span<MutT>::Empty(), *this };
        }
        Tuple<Span<MutT>, Span<MutT>> SplitOnceMut(const T& sep) mut {
            const OptionUsize i = Find(sep);
            return i ? SplitAtMut(i) : Tuple { *this, Empty() };
        }
        Span<const T> Subspan(usize start)              const { return { data + start, size - start }; }
        Span<const T> Subspan(usize start, usize count) const { return { data + start, count }; }
        Span<const T> Subspan(zRange range)             const { return { data + range.min, range.max - range.min }; }
        Span<const T> First(usize num)                  const { return { data, num }; }
        Span<const T> Skip(usize len)                   const { return { data + len, size - len }; }
        Span<const T> Tail()                            const { return { data + 1, size - 1 }; }
        Span<const T> Last(usize num)                   const { return { data + size - num, num }; }
        Span<const T> Trunc(usize len)                  const { return { data, size - len }; }
        Span<const T> Init()                            const { return { data, size - 1 }; }
        Tuple<const T&,      Span<const T>> SplitFirst()       const { return { data[0], Tail() }; }
        Tuple<Span<const T>, const T&>      SplitLast()        const { return { Init(), data[size - 1] }; }
        Tuple<Span<const T>, Span<const T>> CutAt(usize at)    const { return { First(at), Skip(at) }; }
        Tuple<Span<const T>, Span<const T>> SplitAt(usize at)  const { return { First(at), Skip(at + 1) }; }
        Tuple<Span<const T>, const T&, Span<const T>> PartitionAt(usize at) const { return { First(at), data[at], Skip(at + 1) }; }
        Tuple<Span<const T>, Span<const T>> SplitOnceOn(Predicate<T> auto&& pred) const {
            const OptionUsize i = FindIf(pred);
            return i ? SplitAt(i) : Tuple { *this, Empty() };
        }
        Tuple<Span<const T>, Span<const T>> RevSplitOnceOn(Predicate<T> auto&& pred) const {
            const OptionUsize i = RevFindIf(pred);
            return i ? SplitAt(i) : Tuple { Empty(), *this };
        }
        Tuple<Span<const T>, Span<const T>> SplitOnce(const T& sep) const {
            const OptionUsize i = Find(sep);
            return i ? SplitAt(i) : Tuple { *this, Empty() };
        }

        // Option<FixedSpan<T, N>> FirstChunk<usize N>()
        // Tuple<FixedSpan<T, N>, Span> SplitFirstChunk<usize N>()
        // Option<FixedSpan<T, N>> LastChunk<usize N>()
        // Tuple<Span, FixedSpan<T, N>> SplitLastChunk<usize N>()

        // WindowIter                               Windows(usize len)
        // FixedWindowIter                          WindowsFixed(usize len)
        // ChunkIter                                Chunks(usize chunk)
        // RevChunkIter                             RevChunks(usize chunk)
        // ChunkExactIter                           ChunksExact(usize chunk)
        // RevChunkExactIter                        RevChunksExact(usize chunk)
        // FixedChunkExactIter                      FixedChunksExact<usize N>()
        // Span<Array<RemConst<T>, N>>              AsChunksExact<usize N>()
        // Tuple<Span<Array<RemConst<T>, N>>, Span> AsChunks<usize N>()
        // Tuple<Span, Span<Array<RemConst<T>, N>>> AsRevChunks<usize N>()
        //
        // ChunkByIter           ChunkBy(Fn<bool, T&, T&> pred)
        Iter::SplitIter<Span> Split(const T& sep) const {
            return Iter::SplitIter<Span>::New(*this, Only(sep));
        }
        Iter::SplitIter<Span> Split(Span<const T> sep) const {
            return Iter::SplitIter<Span>::New(*this, sep);
        }

        // SplitIfIter           SplitIf(Fn<bool, T&> pred)
        // SplitIfInclIter       SplitIfIncl(Fn<bool, T&> pred)
        // RevSplitIter          RevSplit(const T& sep)
        // RevSplitIfIter        RevSplitIf(Fn<bool, T&> pred)
        // SplitAtmostIfIter     SplitIfAtmost(usize maxLen, Fn<bool, T&, T&> pred)
        // RevSplitAtmostIfIter  RevSplitIfAtmost(usize maxLen, Fn<bool, T&, T&> pred)

        Span& Advance(usize num)    { data += num; size -= num; return *this; }
        Span& Shorten(usize amount) { size -= amount; return *this; }
        Span<MutT> TakeFirstMut(usize num) { Span<MutT> first    = FirstMut(num); data += num; size -= num; return first; }
        Span<MutT> TakeLastMut(usize num)  { Span<MutT> last     = LastMut(num);  size -= num;              return last; }
        MutT&      TakeFirstMut()          { MutT& first         = FirstMut();    ++data; --size;           return first; }
        MutT&      TakeLastMut()           { MutT& last          = LastMut();     --size;                   return last; }
        Span<MutT> TakeAfterMut(usize i)   { Span<MutT> after    = SkipMut(i);    size = i;                 return after; }
        Span<const T> TakeFirst(usize num) { Span<const T> first = First(num);    data += num; size -= num; return first; }
        Span<const T> TakeLast(usize num)  { Span<const T> last  = Last(num);     size -= num;              return last; }
        const T&      TakeFirst()          { const T& first      = First();       ++data; --size;           return first; }
        const T&      TakeLast()           { const T& last       = Last();        --size;                   return last; }
        Span<const T> TakeAfter(usize i)   { Span<const T> after = Skip(i);       size = i;                 return after; }

        template <usize N> Array<Ref<T>, N> GetMany(const Array<usize, N>& indices) {
            Array<Ref<T>, N> values;
            for (usize i = 0; i < N; ++i) { values[i].SetRef(data[indices[i]]); }
            return values;
        }

        void Swap(usize i, usize j) mut { std::swap(data[i], data[j]); }
        void Reverse() {
            for (usize i = 0, j = size - 1; i < j; ++i, --j) Swap(i, j);
        }

        bool RefEquals(Span other) const { return data == other.data && size == other.size; }
        bool ContainsBuffer(Span buf) const { return buf.data >= data && data + size >= buf.data + buf.size; }
        bool OverlapsBuffer(Span buf) const {
            const T* end = data + size, * bufEnd = buf.data + buf.size;
            return end >= buf.data && bufEnd >= data;
        }
        bool Equals(Span other) const { return EqualsBy(other, Cmp::Equality {}); }
        bool EqualsBy(Span other, EqualPred<T> auto&& eq) const {
            if (size != other.size) return false;
            for (usize i = 0; i < size; ++i) if (!eq(data[i], other[i])) return false; return true;
        }
        bool EqualsByKey(Span other, FnArgs<const T&> auto&& keyf) const;
        bool operator==(Span other) const { return Equals(other); }

        bool All(Predicate<T> auto&& pred = Combinate::Identity {}) const {
            for (const T& x : *this) { if (!pred(x)) return false; } return true;
        }
        bool Any(Predicate<T> auto&& pred = Combinate::Identity {}) const {
            for (const T& x : *this) { if (pred(x)) return true; } return false;
        }
        Comparison Cmp(Span other) const { return CmpBy(other, Cmp::Compare {}); }
        Comparison CmpBy(Span other, Comparator<T> auto&& cmp) const {
            for (usize i = 0; i < std::min(size, other.size); ++i) {
                const Comparison c = cmp(data[i], other[i]);
                if (c != Cmp::EQUAL) return c;
            }
            return Cmp::Between(size, other.size);
        }
        Comparison CmpByKey(Span other, FnArgs<const T&> auto&& keyf) const;
        Comparison CmpSized(Span other) const { return CmpSizedBy(other, Cmp::Compare {}); }
        Comparison CmpSizedBy(Span other, Comparator<T> auto&& cmp) const {
            return other.size == size ? CmpBy(other, cmp) : Cmp::Between(size, other.size);
        }
        Comparison CmpSizedByKey(Span other, FnArgs<const T&> auto&& keyf) const;
        Comparison operator<=>(Span other) const { return Cmp(other); }

        bool StartsWith(Span prefix) const { return size >= prefix.size && First(prefix.size) == prefix; }
        bool EndsWith  (Span suffix) const { return size >= suffix.size && Last(suffix.size)  == suffix; }
        Option<Span> RemovePrefix(Span prefix) const { return StartsWith(prefix) ? Options::Some(Skip(prefix.size)) : nullptr; }
        Option<Span> RemoveSuffix(Span suffix) const { return EndsWith(suffix)   ? Options::Some(Trunc(suffix.size)) : nullptr; }


        OptionUsize Find   (const T& target) const { return FindIf   (Cmp::Equals { target }); }
        OptionUsize RevFind(const T& target) const { return RevFindIf(Cmp::Equals { target }); }
        OptionUsize FindIf(Predicate<T> auto&& pred) const {
            for (usize i = 0; i < size; ++i) if (pred(data[i])) return i; return nullptr;
        }
        OptionUsize RevFindIf(Predicate<T> auto&& pred) const {
            for (usize i = size; i --> 0; )  if (pred(data[i])) return i; return nullptr;
        }
        bool Contains     (const T& target) const { return Find(target).HasValue(); }
        bool RevContains  (const T& target) const { return RevFind(target).HasValue(); }
        bool ContainsIf   (Predicate<T> auto&& pred) const { return FindIf(pred).HasValue(); }
        bool RevContainsIf(Predicate<T> auto&& pred) const { return RevFindIf(pred).HasValue(); }
        OptionUsize Find   (Span<const T> target) const {
            for (usize i = 0; i <= size - target.size; ++i)
                if (Subspan(i, target.size) == target) return i;
            return nullptr;
        }
        OptionUsize RevFind(Span<const T> target) const {
            for (usize i = size - target.size; i --> 0; )
                if (Subspan(i, target.size) == target) return i;
            return nullptr;
        }
        bool  Contains   (Span<const T> target) const { return Find   (target).HasValue(); }
        bool  RevContains(Span<const T> target) const { return RevFind(target).HasValue(); }
        Tuple<OptionUsize, OptionUsize> FindOneOf   (Span<const T> anytarget) const {
            for (usize i = 0; i < size; ++i)
                if (OptionUsize j = anytarget.Find(data[i])) return { i, j };
            return { nullptr, nullptr };
        }
        Tuple<OptionUsize, OptionUsize> RevFindOneOf(Span<const T> anytarget) const {
            for (usize i = size; i --> 0; )
                if (OptionUsize j = anytarget.Find(data[i])) return { i, j };
            return { nullptr, nullptr };
        }
        OptionUsize ContainsOneOf   (Span<const T> anytarget) const { const auto [i, j] = FindOneOf   (anytarget); return i.And(j); }
        OptionUsize RevContainsOneOf(Span<const T> anytarget) const { const auto [i, j] = RevFindOneOf(anytarget); return i.And(j); }
        Tuple<OptionUsize, OptionUsize> FindOneOf   (Span<const Span<const T>> anytarget) const {
            for (usize i = 0; i < size; ++i)
                for (usize j = 0; j < anytarget.Length(); ++j)
                    if (Subspan(i).StartsWith(anytarget[j])) return { i, j };
            return { nullptr, nullptr };
        }
        Tuple<OptionUsize, OptionUsize> RevFindOneOf(Span<const Span<const T>> anytarget) const {
            for (usize i = size; i --> 0; )
                for (usize j = 0; j < anytarget.Length(); ++j)
                    if (Subspan(i).EndsWith(anytarget[j])) return { i, j };
            return { nullptr, nullptr };
        }
        OptionUsize ContainsOneOf   (Span<const Span<const T>> anytarget) const { const auto [i, j] = FindOneOf   (anytarget); return i.And(j); }
        OptionUsize RevContainsOneOf(Span<const Span<const T>> anytarget) const { const auto [i, j] = RevFindOneOf(anytarget); return i.And(j); }

        usize  Unaddress      (const T* addr) const { return addr - data; }
        bool   ContainsAddress(const T* addr) const { return data <= addr && addr < data + size; }
        zRange UnaddressSpan      (Span addr) const { return zRange::FromSize(addr.Data() - data, addr.Size()); }
        bool   ContainsAddressSpan(Span addr) const { return data <= addr.Data() && addr.DataEnd() <= this->DataEnd(); }

        // assumes ascending order
        Tuple<bool, usize> BinarySearch(const T& target) const { return BinarySearchWith(Cmp::ComparedTo { target }); }
        // assumes ascending order
        Tuple<bool, usize> BinarySearchWith(Fn<Comparison, const T&> auto&& cmp) const;
        Tuple<bool, usize> BinarySearchByKey(const T& target, FnArgs<const T&> auto&& keyf) const;
        usize BinaryPartitionPoint(const T& target) const { return BinaryPartitionPointBy(Cmp::LessThanWith { target }); }
        usize BinaryPartitionPointBy(Predicate<T> auto&& left) const;
        usize BinaryPartitionPointByKey(const T& target, FnArgs<const T&> auto&& keyf) const;
        usize LowerBound(const T& target) const { return LowerBoundBy(Cmp::ComparedTo { target }); }
        usize LowerBoundBy(Fn<Comparison, const T&> auto&& cmp) const { const auto [found, x] = BinarySearchWith(cmp); return found ? x : std::min<usize>(x, 1) - 1; }
        usize LowerBoundByKey(const T& target, FnArgs<const T&> auto&& keyf) const;
        usize UpperBound(const T& target) const { return UpperBoundBy(Cmp::LessThanWith { target }); }
        usize UpperBoundBy(Fn<Comparison, const T&> auto&& cmp) const;
        usize UpperBoundByKey(const T& target, FnArgs<const T&> auto&& keyf) const;
        // zRange EqualRange(const T& target) const { return LowerBoundBy(Operators::LessThanWith { target }); }
        // zRange EqualRangeBy(Predicate<T> auto&& left) const;
        // zRange EqualRangeByKey(FnArgs<const T&> auto&& keyf) const;

        bool BinaryContains(const T& target) const { const auto [found, _] = BinarySearch(target); return found; }
        bool BinaryContainsKey  (FnArgs<const T&> auto&& keyf, auto&& targetKey) const { const auto [found, _] = BinarySearchByKey(keyf(targetKey)); return found; }
        bool BinaryContainsByKey(FnArgs<const T&> auto&& keyf, const T& target)  const { const auto [found, _] = BinarySearchByKey(keyf(target));    return found; }

        void Sort() mut { return SortBy(Cmp::LessThan {}); }
        void SortBy(Comparator<T> auto&& cmp) mut;
        void SortByKey(FnArgs<const T&> auto&& keyf) mut;

        // void SortStable() TODO i cant be bothered to do this
        // void SortStableBy(Fn<bool, const T&, const T&> auto&& cmp)
        // void SortStableByKey(auto&& keyf)

        bool IsSorted() const { return IsSortedBy(Cmp::LessThan {}); }
        bool IsSortedBy(Comparator<T> auto&& cmp) const;
        bool IsSortedByKey(FnArgs<const T&> auto&& keyf) const;

        usize SortedPartitionPoint(usize idx) mut { return SortedPartitionPointBy(idx, Cmp::LessThan {}); }
        usize SortedPartitionPointBy(usize idx, Comparator<T> auto&& cmp) mut;
        usize SortedPartitionPointByKey(usize idx, FnArgs<const T&> auto&& keyf) mut;

        Tuple<Span, T&, Span> SortedPartition(usize idx) mut { return SortedPartitionBy(idx, Cmp::LessThan {}); }
        Tuple<Span, T&, Span> SortedPartitionBy(usize idx, Comparator<T> auto&& cmp) mut {
            const usize i = SortedPartitionPointBy(idx, cmp);
            return PartitionAt(i);
        }
        Tuple<Span, T&, Span> SortedPartitionByKey(usize idx, FnArgs<const T&> auto&& keyf) mut;

        usize PartitionDupPoint() mut { return PartitionDupPointBy(Cmp::Equality {}); }
        usize PartitionDupPointBy(EqualPred<T> auto&& eq) mut;
        Tuple<Span, Span> PartitionDup() mut { return PartitionDupBy(Cmp::Equality {}); }
        Tuple<Span, Span> PartitionDupBy(EqualPred<T> auto&& eq) mut {
            const usize i = PartitionDupPointBy(eq);
            return SplitAt(i);
        }

        void RotateLeft(usize num) mut;
        void RotateRight(usize num) mut;
        void RotateSigned(isize off)  mut { return off == 0 ? void() : off < 0 ? RotateLeft(-off) : RotateRight(off); }
        void RotateSigned(WrappingIndex off)  mut { return RotateRight(off(size)); }
        void RotateSignedWrapped(isize off)  mut { return RotateRight((off % (isize)size + (isize)size) % (isize)size); }

        void Fill(const T& value) mut { for (T& t : *this) t = value; }
        void FillWith(Fn<T> auto&& factory) mut { for (T& t : *this) t = factory(); }
        void FillDefault() { return FillWith(Combinate::Constructor<T> {}); }
        void CloneFrom(Span<const T> span) mut { Memory::RangeCopy(data, span.data, span.count); }
        void MoveFrom(Span span) mut { Memory::RangeMove(data, span.data, span.count); }
        // void CopyFromSelf(IntegerRange, usize dest)
        void SwapWith(Span span) mut { Memory::RangeSwap(data, span.data, span.count); }

        // Tuple<Span, Span<AddConstIf<SimdT, T>>, Span> AsSimd() const;

        /* Span<AddConstIf<ArrayElement<T>, T>> */ auto AsFlattened() const requires IsRawArray<T> {
            return Span { (AddConstIf<ArrayElement<T>, T>*)data, size * ArrayLength<T>() };
        }

        ArrayBox<MutT> CollectToBox() const {
            T* arr = Memory::AllocateArrayUninit<T>(size);
            for (usize i = 0; i < size; ++i) Memory::ConstructCopyAt(&arr[i], data[i]);
            return ArrayBox<MutT>::Own(arr, size);
        }
        ArrayBox<MutT> MoveToBox() {
            T* arr = Memory::AllocateArrayUninit<T>(size);
            for (usize i = 0; i < size; ++i) Memory::ConstructMoveAt(&arr[i], std::move(data[i]));
            return ArrayBox<MutT>::Own(arr, size);
        }
        Vec<MutT> CollectToVec() const;
        Vec<MutT> MoveToVec() mut;
        Vec<MutT> Repeat(usize num) const;
        // Vec<ConcatResult<T>> Concat() const requires CanConcat<T>;
        // Vec<ConcatResult<T>> Join(const auto& sep) const requires CanConcat<T> && CanConcat<T, decltype(sep)>;

        Hashing::Hash GetHashCode() const {
            usize seed = LengthImpl();
            for (const T& value : *this) {
                seed ^= HashObject(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return Hashing::AsHash(seed);
        }

        template <class U>
        Tuple<Span, Span<AddConstIf<U, T>>, Span> TransmuteAligned() const requires SameAs<const T, const byte> {
            const usize p = (usize)data;
            const usize prefix = -p & (alignof(U) - 1), mid = (size - prefix) / sizeof(U), suffix = size - prefix - mid * sizeof(U);
            return Tuple {
                Span { (T*)p,                               prefix }, // super cool magic, ex: 0b010 + 0b110 to sum to alignof(T)
                Span { (AddConstIf<U, T>*)(p + prefix),     mid },
                Span { (T*)(p + size * sizeof(U) - suffix), suffix }
            };
        }

        template <class U> requires (sizeof(T) % sizeof(U) == 0) || (sizeof(U) % sizeof(T) == 0)
        Span<AddConstIf<U, T>> Transmute() const { return { Memory::TransmutePtr<AddConstIf<U, T>>(data), size * sizeof(T) / sizeof(U) }; }
        template <class U> requires (sizeof(U) % sizeof(T) == 0)
        AddConstIf<U, T>& ReadFirst() const { return *Memory::TransmutePtr<AddConstIf<U, T>>(data); }
        template <class U> requires (sizeof(U) % sizeof(T) == 0)
        AddConstIf<U, T>& Read() { auto& x = *Memory::TransmutePtr<AddConstIf<U, T>>(data); data += sizeof(U) / sizeof(T); size -= sizeof(U) / sizeof(T); return x; }

        Span<MutT>      AsMut()   const { return { Memory::AsMutPtr(data),   size }; }
        Span<const T>   AsConst() const { return { Memory::AsConstPtr(data), size }; }

        operator Span<const T>() const { return AsConst(); }
    };

    template <class T> requires ContinuousCollectionAny<T>
    Span(T& collection) -> Span<AddConstIf<CollectionItem<T>, T>>;

    using Bytes = Span<const byte>;
    using BytesMut = Span<byte>;

    namespace Spans {
        template <class T>
        Span<T> Only(T& unit) { return Span<T>::Only(unit); }
        template <class T>
        Span<T> Slice(T* data, usize size) { return Span<T>::Slice(data, size); }
        template <class T>
        Span<const T> FromIList(IList<T> ilist) { return Span<const T>::Slice(ilist.begin(), ilist.size()); }
        template <class T, usize N>
        Span<T> Vals(T (&arr)[N])  { return Span<T>::FromArray(arr); }
        template <class T, usize N>
        Span<T> Vals(T (&&arr)[N]) { return Span<T>::FromArray(arr); }
    }

    template <class T>
    struct BufferIterator : IContinuousIterator<T, BufferIterator<T>> {
        friend IIterator<T, BufferIterator>;
        friend IContinuousIterator<T, BufferIterator>;

        RemRef<T>* iter;
        RemRef<T>* end;

        BufferIterator(RemRef<T>* beg, RemRef<T>* end) : iter(beg), end(end) {}

    protected:
        T CurrentImpl() const {
            if constexpr (!IsRef<T>) return std::move(*iter);
            else return *iter;
        }
        void AdvanceImpl() { ++iter; }
        bool CanNextImpl() const { return iter != end; }
    public:
        bool operator==(const BufferIterator& it) const { return iter == it.iter; }
    };

    template <class T, class Super>
    Span<const byte> IContinuousCollection<T, Super>::AsBytes()  const { return AsSpan().AsBytes(); }
    template <class T, class Super>
    Span<byte>       IContinuousCollection<T, Super>::AsBytesMut() mut { return AsSpan().AsBytesMut(); }

    template <class T, class A> Span<const T> Box<T, A>::AsSpan() const { return data ? Span<const T>::Only(*data) : nullptr; }
    template <class T, class A> Span<T>       Box<T, A>::AsSpanMut()    { return data ? Span<T>::Only(*data)       : nullptr; }

#undef mut
}