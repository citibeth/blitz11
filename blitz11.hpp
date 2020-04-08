/**


Necessary Features
==================

1. Arrays are fully C++ compliant, can be held in standard C++ collections.

2. Array unnderlying memory is either: (a) a shared ptr, or (b) borrowed.  Both variants have the same top-level type.  When new arrays are constructed from an old array, the new array shares or borrows, according to how the old arrya did it.

3. Handles const correctly.

4. Array layout is fully configurable: any base, any stride (even negative), any dimension order.

5. Full complement of array slicing, reshaping, etc.


Nice-To-Have Features
=====================

6. Shared memory blocks may be used, managed, loaded, saved, etc. separately from any particular Array used to access them.

7. The dope vector (array layout) is accessible as a first-class object.  Dope vectors may be copied, manipulated and used to construct new Arrays.

8. Bounds checking.  Controlled by a template parameter.  A global (compile-time) parameter can also be used to turn off all bounds checking.

9. Bounds errors reported in a way that can generate a stacktrace.

10. Allows to write code that can work with an any-dimension Array.  This mode of access is not expected to be fast.

11. Vectorized operations, lazy evaluation magic, etc. as in Eigen::Tensor, Blitz++, etc.  This is a cool feature, and it is well known how to implement.  But it is not essential.  Initial design should be built in a way to not PRECLUDE these features from begin efficiently added in the future.

12. Support ultra-long dimensions (>4 billion extent in a dimension)

13. Support inter-process shared memory arrays (eg. boost::interprocess).

*/


#include <type_traits>

/** Transfers const qualification (if any) from DestT to SrcT;
Eg:  transfer_const<double, char const>::type == double const
     transfer_const<double, char>::type == double
*/
template<class DestT, class SrcT>
struct transfer_const {
    typedef std::conditional<
        std::is_const<SrcT>::value,
            std::add_const<DestT>::type
            std::remove_const<DestT>::type
    > type;
};

typedef std::function<void (std::string const &type, int idim, long low, long high, long index)> RangeErrorFn;

// https://stackoverflow.com/questions/13061979/shared-ptr-to-an-array-should-it-be-used
template< typename T >
struct array_deleter
{
  void operator ()( T const * p)
  { 
    delete[] p; 
  }
};


/**
Const handling: CharT is either char or const char.

TODO: Add STL-standard Allocator support.
TODO: char->std::byte for C++17 */
template<class CharT>
class MemoryBlock {
    typedef transfer_const<char,ValueT>::type CharT;

    std::shared_ptr<CharT> _held;    // Memory we hold

    CharT * _base;
    size_t _size_bytes;

public:
    size_t const size_bytes() { return size_bytes; }
    CharT *base() { return base(); }

    /** Allocate our own memory to a particular size */
    MemoryBlock(size_t size_bytes) :
        held(new CharT[size_bytes], array_deleter<CharT>()),
        _base(held.get()),
        _size_bytes(size_bytes) {}

    /** Use someone else's memory of a particular size */
    MemoryBlock(
        CharT * const base,
        CharT const size_bytes)
    : _base(base), _size_bytes(size_bytes) {}


private:
    /** Index into the MemoryBlock, by bytes, and possibly check ranges */
    inline CharT *index_bytes(ptrdiff_t const diff_bytes, RangeErrorFn const *range_error = nullptr) const
    {
        if (range_error) {
            if (diff_bytes < 0 || diff_bytes >= size_bytes)
                (*range_error)("Memory", 0, diff_bytes, 0, size_bytes);
        }
        return base + diff_bytes;
    }

};


template<class IndexT>
struct Dope {
    std::array<IndexT,2> range;    // [low, high)
    ptrdiff_t stride;
}

template<class IndexT>
inline ptrdiff_t index_diff(
    Dope const * const dopes,
    IndexT const * const index,
    int const rank,
    RangeErrorFn const * const range_error = nullptr)
{
    ptrdiff_t diff = 0;
    for (int i=0; i<rank; ++i) {
        if (range_error) {
            if ((index[i] < dopes[i].range[0]) || (index[i] >= dopes[i].range[1]))
                (*range_error)("Indexing", i, index[i], range[0], range[1]);
        }
        diff += index[i] * dopes[i].stride;
    }
    return diff;
}

template<class ValueT, class IndexT>
inline ValueT &index(
    MemoryBlock<typename transfer_const<char, ValueT>::type> const &memory,
    Dope const * const dopes,
    IndexT const * const index,
    int const rank,
    RangeErrorFn const * const range_error = nullptr)
{
    typedef typename transfer_const<char, ValueT>::type CharT;

    ptrdiff_t const diff = index_diff(dopes, index, rank, range_error);
    CharT * const loc = memory.index_bytes(diff*sizeof(ValueT), range_error);
    ValueT * const vloc = reinterpret_cast<ValueT *>(loc);    // same constness
    return *vloc;
}


// ---------------------------------------------------------------
template<class ValueT, class IndexT=int>    // ValueT = double, const double, etc.
class GeneralArray {
    typedef transfer_const<char, ValueT>::type CharT;

    MemoryBlock<CharT> _memory;    // Like a shared_ptr
    std::vector<Dope<IndexT>> _dopes;

public:
    int rank() const { return _dopes.size(); }

    ValueT &operator[](IndexT const *index, RangeErrorFn const * const range_error=nullptr) const
        { return index(_memory, &_dopes[0], index, rank(), range_error); }

    ValueT &operator[](std::vector<IndexT> const &index, RangeErrorFn const * const range_error=nullptr) const
        { return index(_memory, &_dopes[0], &index[0], rank(), range_error); }


};


template<class ValueT, int RANK, class IndexT=int>    // ValueT = double, const double, etc.
class Array {
    typedef typename transfer_const<char, ValueT>::type CharT;

    MemoryBlock<CharT> _memory;    // Like a shared_ptr
    std::vector<Dope<IndexT>> _dopes;

public:
    int rank() const { return RANK; }

    ValueT &operator[](IndexT const * const index, RangeErrorFn const * const range_error=nullptr) const
        { return index(_memory, &_dopes[0], index, rank(), range_error); }

};



// ---------------------------------------------------------------





class Dim {
    std::array<int,2> bound;
    size_t stride;
    std::string name;
};

template<class TypeT, int RANK>
class Array {
    std::shared_ptr<> mem;
};
