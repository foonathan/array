# array

[![Build Status](https://travis-ci.com/foonathan/array.svg)](https://travis-ci.com/foonathan/array)
[![Build status](https://ci.appveyor.com/api/projects/status/iydnaute3tiqxi9u?svg=true)](https://ci.appveyor.com/project/foonathan/array)
[![Boost Licensed](https://img.shields.io/badge/license-Boost-blue.svg)](LICENSE)

> Note: This project is currently WIP, no guarantees are made until an 0.1 release.

This library is all about *arrays* — contiguous blocks of memory.
It basically provides a customizable `std::vector<T>` and containers built on top of it like flat sets and maps.

The equivalent of `std::vector<T>` — `array<T>` — does not take an `Allocator`.
Instead, it takes a `BlockStorage`.
This policy is responsible for completely managing the underlying memory block:
It stores the current address and size and has full control over `reserve()` and `shrink_to_fit()`.
This makes it possible to have a fixed sized array, an array with small buffer optimization and much more just by swapping the policy.

## Features

#### Block Storage Implementations

* `block_storage_embedded`: uses a member array of `char` for memory allocations
* `block_storage_heap<Heap, GrowthPolicy>`: dynamic memory allocation.
  The `Heap` controls *how* memory is allocated — policy with `allocate()` and `deallocate()`,
  the `GrowthPolicy` *how much* memory is allocated.

  `Heap`s:
    * `new_heap`: uses `::operator new`
    * `allocator_heap<Allocator>`: uses the given `Allocator` for allocation

  `GrowthPolicy`s:
    * `no_extra_growth`: only allocates the minimum amount of memory necessary
    * `factor_growth<Num, Den>`: grows by factor `Num / Den`

  `block_storage_new<GrowthPolicy>` is a convenience typedef, uses the `new_heap` and a custom `GrowthPolicy`.
* `block_storage_sbo`: first uses `block_storage_embedded`, then another `BlockStorage` (small buffer optimization)
* `block_storage_heap_sbo`: alias for `block_storage_sbo` that uses the given `Heap` for allocation
* `block_storage_default`: the default `BlockStorage` (`block_storage_new` right now)

#### Containers

* `array<T>`: the `std::vector<T>` of this library
* `small_array<T>`: convenience alias for `array<T>` with SBO
* `bag<T>`: an `array<T>` where order of elements isn't important, allows an `O(1)` erase
* `small_bag<T>`: convenience alias for `bag<T>` with SBO
* `flat_(multi)set<Key>`: a sorted `array<Key>` with `O(log n)` lookup & co plus a superior interface to `std::set`
* `flat_(multi)map<Key, Value>`: a `flat_set<Key>` and an `array<Value>` for key-value-storage,
again with superior interface compared to `std::map`

#### Views

* `block_view<T>`: a pointer + size pair that doesn't provide indexing (`bag<T>` provides only a `block_view<T>`)
* `array_view<T>`: a `block_view<T>` with indexing
* `sorted_view<T, Compare>`: an `array_view<T>` that is sorted according to `Compare`
* `input_view<T, BlockStorage>`: similar to `std::initializer_list` but allows stealing the memory
* `byte_view<T>()`: converts an `array_view<T>` into an `array_view<std:byte>` to allow byte-wise interpretation of the memory

#### Misc

* low-level memory manipulation utilities and algorithms
* `pointer_iterator<Tag, T>` utility to create distinct iterator types on top of pointers
* `ContiguousIterator` facilities

## FAQ

**Q: How many active projects do you have now? 4?**

A: 5-6 actually.

**Q: Do you have too much time?**

A: Apparently.

**Q: Does `size()` return a signed or unsigned?**

A: Unsigned, but I might change my mind.

**Q: Is `array_view<T>` mutable or immutable?**

A: Mutable because I don't have a better word right now.

**Q: Maybe something like `array_ref<T>`?**

A: FAQs are not really suited for bike shedding.

**Q: It breaks when I do this!**

A: Don't do that. And file an issue (or a PR, I have 5 other projects...).

**Q: This is awesome!**

A: Thanks. I do have a Patreon page, so consider checking it out:

[![Patreon](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://patreon.com/foonathan)

## Documentation

> Detailed reference documentation is WIP.

### Building

Header-only (almost everything is a template anyway), no dependencies (currently).

It requires at least C++11, but works better with C++14 or 17.
Compilers that are being tested on CI:

* Linux: GCC 4.8, 4.9, 5, 6, 7 and clang 3.9, 4, 5
* OSX: the clang coming with Xcode 8 and 9
* MSVC: Visual Studio 2015 and 2017

Newer compilers should work too.

#### `BlockStorage` concept

The core concept of this library is the `BlockStorage` — the type that controls memory block allocation:

```cpp
class BlockStorage
{
public:
    /// Whether or not the block storage may embed some objects inside.
    /// If this is true, the move and swap operations must actually move objects.
    /// If this is false, they will never physically move the objects.
    ///
    /// It is optional: if it is not provided, it defaults to [std::false_type]().
    using embedded_storage = std::integral_constant<bool, ...>;

    /// The arguments required to create the block storage.
    ///
    /// These can be runtime parameters or references to allocators.
    /// It must be a type that is nothrow (and cheaply) copyable.
    ///
    /// It is optional: If it is not provided, the empty type [array::default_argument_type]() is used.
    struct argument_type;

    //=== constructors/destructors ===//
    /// \effects Creates a block storage with the maximal block size possible without dynamic allocation.
    /// \notes If there is not `argument_type` typedef, it must take [array::default_argument_type]() directly.
    explicit BlockStorage(argument_type arg) noexcept;

    BlockStorage(const BlockStorage&) = delete;
    BlockStorage& operator=(const BlockStorage&) = delete;

    /// Releases the memory of the block, if it is not empty.
    /// \notes It does not need to destroy any elements.
    ~BlockStorage() noexcept;

    /// Swap.
    /// \effects Exchanges ownership over the allocated memory blocks.
    /// When possible this is done without moving the already constructed objects.
    /// If that is not possible, they shall be moved to the beginning of the new location
    /// as if [array::uninitialized_destructive_move]() was used.
    /// The views are updated to view the new location of the constructed objects, if necessary.
    /// \throws Anything thrown by `T`s copy or move constructor.
    /// \notes This function must not allocate dynamic memory.
    template <typename T>
    static void swap(BlockStorage& lhs, block_view<T>& lhs_constructed,
                     BlockStorage& rhs, block_view<T>& rhs_constructed)
                        noexcept(!embedded_storage::value || std::is_nothrow_move_constructible<T>::value);

    //=== reserve/shrink_to_fit ===//
    /// \effects Increases the allocated memory block by at least `min_additional_bytes`.
    /// The range of already created objects is passed as well,
    /// they shall be moved to the beginning of the new location
    /// as if [array::uninitialized_destructive_move]() was used.
    /// \throws Anything thrown by the allocation function, or the copy/move constructor of `T`.
    /// If an exception is thrown, nothing must have changed.
    /// \notes Use [array::uninitialized_destructive_move]() to move the objects over,
    /// it already provides the strong exception safety for you.
    template <typename T>
    void reserve(size_type min_additional_bytes, const block_view<T>& constructed_objects);

    /// \effects Non-binding request to decrease the currently allocated memory block to the minimum needed.
    /// The range of already created objects is passed, those must be moved to the new location like with `reserve()`.
    /// \throws Anything thrown by the allocation function, or the copy/move constructor of `T`.
    /// If an exception is thrown, nothing must have changed.
    /// \notes Use [array::uninitialized_destructive_move]() to move the objects over,
    /// it already provides the strong exception safety for you.
    template <typename T>
    void shrink_to_fit(const block_view<T>& constructed_objects);

    //=== accessors ===//
    /// \returns The currently allocated memory block.
    memory_block block() const noexcept;

    /// \returns The arguments passed to the constructor.
    ///
    /// This is optional: if `argument_type` isn't provided, it can be left out.
    /// If `argument_type` is provided, it is not optional.
    argument_type argument() const noexcept;

    /// \returns The maximum size of a memory block managed by a storage created with the given arguments,
    /// or `memory_block::max_size()` if there is no limitation by the storage itself.
    ///
    /// This function is optional: if it isn't provided, `memory_block::max_size()` is used instead.
    /// \param 0
    /// This parameter can be left out if the information doesn't depend on any arguments.
    static size_type max_size(argument_type) noexcept;
};
```

> Making some of the member functions optional is planned.

You can plug it into any container type of this library and fully control it.

#### Customizing only Allocation

If you just want to change the memory allocation and not the reserve policy,
you can just use the `Heap` and `GrowthPolicy` concepts of `block_storage_heap`.

`Heap` is a simple `Allocator` concept:

```cpp
struct Heap
{
    /// The handle for that particular heap.
    /// It must be cheaply and nothrow copyable.
    struct handle_type;

    /// Allocates a memory block of the given size and alignment or throws an exception if it is unable to do so.
    /// Doesn't need to handle size `0`.
    static memory_block allocate(handle_type& handle, size_type size, size_type alignment);

    /// Deallocates a memory block.
    /// Doesn't need to handle empty blocks.
    static void deallocate(handle_type& handle, memory_block&& block) noexcept;

    /// Returns the maximum size of a memory block, or [array::memory_block::max_size()]() if it isn't limited by the allocator.
    ///
    /// This function is optional: if it isn't provided, `memory_block::max_size()` is returned.
    static size_type max_size(const handle_type& handle) noexcept;
};
```

It is implemented by `new_heap`, for example, which simply forwards to `new` and `delete`.

The `GrowthPolicy` controls the growth factor of `reserve()` and `shrink_to_fit()`:

```cpp
struct GrowthPolicy
{
    /// \returns The new size of the memory block based on the current size,
    /// the minimal size it needs to increase, and the maximum size supported by the heap.
    /// \requires It must return at least `cur_size + additional_needed`.
    static size_type growth_size(size_type cur_size, size_type additional_needed,
                                 size_type max_size) noexcept;

    /// \returns The new size of the  memory block based on the current size,
    /// the minimal total size required, and the maximum size supported by the heap.
    /// \requires It must return at least `size_needed`.
    static size_type shrink_size(size_type cur_size, size_type size_needed,
                                 size_type max_size) noexcept;
};
```

You probably don't need to write a `GrowthPolicy` yourself as the library provides `no_extra_growth` and `factor_growth<Num, Den>`.

The two policy combined are used in `block_storage_heap<Heap, GrowthPolicy>`.
If you use `block_storage_new<default_growth>` (which is `block_storage_heap<new_heap, default_growth>`),
you have the behavior `std::vector` has today.

#### Small Buffer Optimization

If you want a small buffer optimization,
simply combine your big buffer policy of choice with `block_storage_sbo<SmallBuffer, BigBlockStorage>`.

#### Using the Array

`array<T>` is this library's `std::vector<T>` but with a customizable `BlockStorage`.
The interface is similarly enough so you don't need a tutorial, but note that it is not a drop-in replacement.

It also has some nice additional stuff like `append_range()` or view support (see below).

`bag<T>` is like `array<T>` but doesn't provide an index operator, as the position of elements is not guaranteed.
As such it can have a set-like interface with just `insert(element)`, but doesn't provide lookup.
It is used if you just want a collection of elements and later need to iterator over them in any order, for example.
Losing ordering guarantees means it can provide an O(1) `erase(iter)` (it swaps with the last element and does a `pop_back()`).

I heavily suggest seeing whether it is applicable for your use case.

#### Using the Set and Map

If you use `flat_set` or `flat_map` you have to provide a comparison predicate.
This is not something like `std::less` but instead a three way comparison modelling `KeyCompare`:

```cpp
/// The ordering of a key in relation to some other value - provided by the library.
enum class key_ordering
{
    less,       //< Other value is less than the key, i.e. sorted before it.
    equivalent, //< Other value is equivalent to the key, i.e. a duplicate.
    greater,    //< Other value is greater than key, i.e. sorted after it.
};

struct KeyCompare
{
    /// Compares the key with some other type.
    ///
    /// It must define a strict total ordering of the keys.
    /// `TransparentKey` may be restricted to certain types, or just the key type itself.
    template <typename Key, typename TransparentKey>
    static key_ordering compare(const Key& key, const TransparentKey& other) noexcept;
};
```

`key_compare_default` works for all types that provide a `.compare()` member function or a less-than operator,
but it can also be specialized for your own types:

```cpp
template <>
struct key_compare_default::customize_for<my_type>
{
    static key_ordering compare(const my_type& key, const my_type& other) noexcept
    {
        return ...;
    }

    // can be compared with strings which is less expensive than constructing the object first
    static key_ordering compare(const my_type& key, std::string_view other) noexcept
    {
        return ...;
    }
};
```

This should only be done if you want to put a type in a set that doesn't provide a compare function.
If you want to change the order of elements (e.g. reverse them for some reason), provide a different `KeyCompare` altogether.

`flat_set<T>` stores the keys as one sorted array and provides an interface similar to `std::set`,
but with more functionality (it has a `.contains()`, for example)!

If you want to have a mapping between keys and values, use `flat_map<Key, Value>`.
Keys and values are stored in separate arrays linked implicitly by having the same index.
This is the proposed design of `std::flat_map` as well.

The interface is similar to it as well, but better:
It does not use `std::pair`, so no more `insert(...).first->second`, instead you have `.iter()->value`, for example.
It also provides separate iterator over just the keys or just the values.

If you want to store the keys and values together in memory (maybe because the value is small),
use `flat_set<key_value_pair<Key, Value>>`.
You lose a little bit of convenience —
there is no `insert_or_assign()` and `lookup()` gives you a key value pair as result
(but only needs a key as input!), but might gain the extra performance.

The multi- variants behave just like you would expect.

### Using the Block Views

The library provides a hierarchy of view types, i.e. pointer plus size pairs.
They can be used just like `std::string_view` but for arbitrary types and are mutable.
All containers can be constructed and convert to a matching view:

* `block_view<T>` — the most basic view, doesn't provide array access.
`bag<T>` gives you a `block_view<T>`.

* `array_view<T>` — a `block_view<T>` with array access.
`array<T>` gives you an `array_view<T>`.

* `sorted_view<T, Compare>` — an `array_view<T>` that is sorted.
`flat_set<Key>` gives you a `sorted_view<T, Compare>`.

The views are essential because two `array<T>` with different `BlockStorage` policies are different types.
So the containers are not really suited for use in interfaces, just as implementation details.
Interfaces taking non-owning data should use the views instead.

For interfaces taking ownership, the special view `input_view<T, BlockStorage>` can be used instead.
It can be used to create a container as efficient as possible.
It either copies all elements, moves all elements, or takes ownership over a suitable memory block.
This allows moving memory between different containers using the same `BlockStorage`.

## Planned Features

#### Block Storage Implementations

* `pmr_heap`: uses a `std::pmr::memory_resource` for the allocation, for use with `block_storage_heap`
* A `BlockStorage` using an external fixed sized buffer
* A `BlockStorage` (adapter) that limits the maximum size

#### Containers

* `unsized_array<T>`: an `array<T>` that doesn't know its size, low-level building block
* `variant_bag<T1, T2, ...>`: an SOA optimized `bag<std::variant<T1, T2, ...>>`
* `ring_buffer<T>`: a ring buffer of elements
* multi dimensional stuff?
* hash table?

#### Misc

* Better docs
* Better tests, switch to doc test for templated test cases?
* Better (and documented) exception safety
* Some unified precondition checking
* Simplifying `BlockStorage` concepts
* Destructive move support
* Stable pointer that don't get invalidated on reserve (i.e. indices)
