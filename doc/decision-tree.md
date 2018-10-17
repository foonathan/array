In case you don't know what to use.
The container decision tree helps you pick a container,
the view decision tree helps you pick the right view,
and the block storage decision tree helps you pick a block storage.

## Container Decision Tree

> When in doubt, use `std::vector`.

The problem with that approach:
In this library, *all* containers are `std::vector`!

You want to store:

* … just some objects of type `T`.
  Do you want often need to check whether `T` is already present or look it up based on a member?
    * Yes. Do you have a hash function for `T`?
        * Yes: Use some hash set from another library (but not `std::unordered_(multi)set`).
        * No:
            * Duplicates are not allowed: `array::flat_set<T>`
            * Duplicates are allowed: `array::flat_multiset<T>`
    * No. Do you want to access elements by index and/or the order is important?
        * Yes: `array::array<T>`
        * No: `array::bag<T>`

* … a mapping of `Key` to `Value`.
 Do you have a hash function for `Key`?
    * Yes: Use some hash map from another library (but not `std::unordered_(multi)map`).
    * No:
        * Duplicates are not allowed: `array::flat_map<Key, Value>`
        * Duplicates are allowed: `array::flat_multimap<Key, Value>`

* … a variant of `T1`, `T2`, …, `Tn`.
  Do you want to access elements by index and/or the order is important?
    * Yes: Repeat with `T = std::variant<T1, T2, …, Tn>`.
    * No: `array::variant_bag<BlockStorage, T1, T2, …, Tn>`.

## View Decision Tree

You want to use a view, if you don't want to add/remove elements to a container.

Do you want to use the view to construct a container or would have used `std::initializer_list`?

* Yes: `array::input_view<T, BlockStorage>` where `BlockStorage` is the block storage of your container.
* No. Do you need a sorted input?
    * Yes: `array::sorted_view<T, Compare>` where `Compare` is the key compare (usually you don't need to touch it).
    * No. Does it make sense to ask for the nth element?
        * Yes: `array::array_view<T>`
        * No: `array::block_view<T>`

If you need to modify the elements, pass `T`.
Otherwise, use `const T`.

## Block Storage Decision Tree

What do you know about the number of objects in the container?

* It is at most `N` where `N` is small enough that I can waste unused space: `array::block_storage_embedded<N * sizeof(T)>`.
* It is often `N` where `N` is small (single digit) but sometimes larger.
  Do you have a custom allocator?
    * Yes: `array::block_storage_heap_sbo<N * sizeof(T), MyCustomAllocator>`
    * No: `array::block_storage_heap_sbo<N * sizeof(T)>`.
  You can also use the `array::small_XXX` convenience aliases.
* I don't know anything.
  Do you have a custom allocator?
    * Yes: `array::block_storage_heap<MyCustomAllocator>`.
    * No: `array::block_storage_default` or just ignore the template parameter all together.

If you care about growth factors:
Look at the defaulted template parameters in the block storage.

If you have a fancy allocator that can decide the growth factors by itself:
Implement your own `BlockStorage`.

