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

typedef std::function<void (int idim, long low, long high, long index)> RangeErrorFn;

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
Const handling:
   const MemoryBlock: Indexing returns const char
   MemoryBlock: Indexing returns char

   In either case, MemoryBlock is immutable (due to consts inside)

TODO: Add STL-standard Allocator support.
TODO: char->std::byte for C++17 */
class MemoryBlock {
    std::shared_ptr<char> held;    // Memory we hold

public:
    char * const base;
    size_t const size_bytes;

    /** Allocate our own memory to a particular size */
    MemoryBlock(size_t _size_bytes) :
        held(new char[_size_bytes]),
        base(held.get()),
        size_bytes(_size_bytes) {}

    /** Use someone else's memory of a particular size */
    MemoryBlock(
        char const *_base,
        char const _size_bytes)
    : base(_base), size_bytes(_size_bytes) {}


private:
    /** Index into the MemoryBlock, by bytes, and possibly check ranges */
    inline char const *index_bytes(ptrdiff_t const diff_bytes, RangeErrorFn *range_error = nullptr) const
    {
        if (range_error) {
            if (diff_bytes < 0 || diff_bytes >= size_bytes)
                (*range_error)(diff_bytes, 0, size_bytes);
        }
        return base + diff_bytes;
    }

    inline char *index_bytes(ptrdiff_t const diff_bytes, RangeErrorFn *range_error = nullptr)
        { return const_cast<char *>(index(diff_bytes, range_error)); }


public:
    template<class ValueT>
    inline ValueT const &index(ptrdiff_t const diff, RangeErrorFn *range_error = nullptr) const
    {
        return *reinterpret_cast<ValueT *>(
            index_bytes(diff*sizeof(ValueT), range_error));
    }

    template<class ValueT>
    inline ValueT &index(ptrdiff_t const diff, RangeErrorFn *range_error = nullptr)
    {
        return *const_cast<ValueT *>(&index(diff, range_error));
    }


};


template<class IndexT>
struct Dope {
    std::array<IndexT,2> range;    // [low, high)
    ptrdiff_t stride;
}

template<class IndexT>
inline ptrdiff_t index(
    Dope const * const dopes,
    IndexT const * const index,
    int const rank,
    RangeErrorFn *range_error = nullptr)
{
    ptrdiff_t diff = 0;
    for (int i=0; i<rank; ++i) {
        if (range_error) {
            if ((index[i] < dopes[i].range[0]) || (index[i] >= dopes[i].range[1]))
                (*range_error)(index[i], range[0], range[1]);
        }
        diff += index[i] * dopes[i].stride;
    }
    return diff;
}
// ---------------------------------------------------------------
template<class IndexT, class ValueT>
class GeneralArray {
    MemoryBlock const memory;
    std::vector<Dope<IndexT>> const dopes;

    int rank() { return dopes.size(); }

    ValueT &operator[](IndexT *index, RangeErrorFn *range_error=nullptr)
    {
        return memory.index<ValueT>(index(&dopes[0], index, rank(), range_error));
    }

    ValueT &operator[](IndexT *index, RangeErrorFn *range_error=nullptr) const
    {
        return memory.index<ValueT>(index(&dopes[0], index, rank(), range_error));
    }

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
