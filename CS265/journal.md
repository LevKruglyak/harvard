## Hybrid Indexes
* Develop common interface between B-tree nodes/layers and learned (PLA or RS) indexes.
* Easy to tune zero-cost generic parameters, with easy to access benchmarking

# Technical Implementation Details
* Memory Mapping in Rust
  * memmap2 crate, requires one unsafe block in case file modified while mmaped
  * Safety:
    * on some platforms, there should be a way to `lock` a file
    * use simple crate `fs2`
    * constructing an mmap fails if the file is already locked (unit tested, but still edge cases)
    * unmap/unlock on `Drop`
  * Simple benchmark
    * writing to large file seems to work fine, much faster than reading the whole thing
  * Byte `transmutation`:
    * use `bytemuck` crate
    * mmap wrapper supports arrays of structs which implement `Pod` (no UTF-32 chars, pointers, etc)
    * standard key/value -> i64 with [u8; 8] -> 16 bytes
    * TODO: regrowable Mmap
  

* Generic Configuration
  * all types should be ZST, and everything should compile to equivalent assembly
  * Non-Ideal Scenario
    * Long generic lists for every implementation
    * Verbose, lot of refactoring for additional generics
  * Option 1:
    * singular configuration trait with associated types/constants
    * BTree / LI would have a single generic type C: Config
    * then statically sized arrays could be of the form [K; C::BLOCK_SIZE]
    * unfortunately, this is unstable (generic_const_exprs) [https://github.com/rust-lang/rust/issues/76560]
  * Option 2:
    * Same as previous, but to avoid unstable:
    * For every type that requires const generics, wrap it in a const generic free trait
    * Example: implementation of `Block` trait for all [u64; D] (```impl<const D: usize> Block for [u64; D]```)
    * the `Block` trait could require implementations to return a (mutable) slice to the data
    * Potentially more flexible in the cases we require (e.g. also support SmallVec or other dynamically growable memory)

# Algorithm Implementation Details
* Insertion is a big problem
  * Easiest way would be to use interior mutability, but this gets into conflict with bit encoding.
  * Since entries are copyable, want something like ```&mmut[..] is type &[Cell<Entry>]```
* Simple immutable Btree implementation
  * Slowest part of construction is fetching maximum of the root nodes (i.e. data movement from disk)

# Questions
* Data sets for benchmarking (OSM Hilbert curve etc?)
* Per level or per node hybridization?
  * How generic should this be?
    * Presumably, fully generic is impossible
    * Single generic `HybridizationPolicy` parameter
  * Enum Size problem
    * enum is size of maximum + alignment
    * so if all the blocks have different sizes, (e.g. internal, root, model, etc) we may have a problem
    * -> store additional metadata
      * per level hybridization -> store a tiny table mapping level id to type, store different block types separate
      * per node hybridization -> store enum `Internal(u32)` or `Learned(u32)` or `Root(u32)`, etc, maybe bit field to further optimize
* Look at [https://github.com/spacejam/sled] for production level db in Rust
* const generic issue for safely representing `zerocopy` structs
  * no implementation until `generic_const_exprs`
  * to store blocks with generic sizes, might need to use slice types or something (or copy)






  Support generic models














TODO:
* in-place inserts
* no deletes
* index construction benchmarks

benchmark
* 10 Gb -> different distribution, just keys
* -> construction for different distributions
* -> query time (with warm up)
* -> insert (todo)
















