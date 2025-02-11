#define M61_DISABLE 1
#include "m61.hh"

#include <limits.h>

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "hexdump.hh"

/// Maximum allowed malloc size
#define MAX_MALLOC_SIZE INT_MAX

/// Shortcut for max alignment
#define ALIGNMENT alignof(std::max_align_t)

/// Global statistics object, initialized with default values
static m61_statistics static_stats = {
    0,           //nactive
    0,           //active_size
    0,           //ntotal
    0,           //total_size
    0,           //nfail
    0,           //fail_size
    (size_t)-1,  //heap_min
    0,           //heap_max
};

/// Number of entries in hitters report to include after last 20% or higher entry
static const int REPORT_SIZE = 5;

/// Higher this value is, the less heavy hitters will be logged
static const int SAMPLING_RATE = 10;

/// Container for managing heavy hitters
struct heavy_hitters_arena {
    struct hh_key {
        const char *file;
        long line;

        bool operator==(const hh_key &key) const {
            return file == key.file && line == key.line;
        }
    };

    struct hh_value {
        size_t size;
        size_t count;

        hh_value &operator+=(const hh_value &value) {
            this->size += value.size;
            this->count += value.count;
            return *this;
        }
    };

    struct hh_hash {
        std::size_t operator()(const hh_key &key) const {
            // http://stackoverflow.com/a/1646913/126995
            size_t res = 17;
            res = res * 31 + std::hash<const char *>()(key.file);
            res = res * 31 + std::hash<long>()(key.line);
            return res;
        }
    };

    hh_value total;
    std::unordered_map<hh_key, hh_value, hh_hash> counter_ptrs;

    /// Mark a malloc at a file:line pair with a given size
    void add(const char *file, long line, size_t size) {
        hh_key key = {file, line};
        hh_value value = {size, 1};

        if (rand() % SAMPLING_RATE == 0) {
            total += value;
            counter_ptrs[key] += value;
        }
    }

    void print_list(std::vector<std::pair<hh_key, size_t>> list, size_t total_count, const char *print_format) {
        // Sort the vector by increasing order of its pair's second value
        std::sort(list.begin(), list.end(), [](std::pair<hh_key, size_t> a, std::pair<hh_key, size_t> b) { return (a.second > b.second); });

        int report_size_tracker = 0;

        for (auto map_pair : list) {
            float percent = 100. * map_pair.second / total_count;

            // Track how many heavy hitters below 20% we have tracked so far
            if (percent < 20) {
                report_size_tracker++;
            }

            if (report_size_tracker > REPORT_SIZE) {
                break;
            }

            printf(print_format, map_pair.first.file, map_pair.first.line, map_pair.second * SAMPLING_RATE, percent);
        }
    }

    /// Print a report of heavy/frequent hitters
    void print_report() {
        std::vector<std::pair<hh_key, size_t>> sizes;
        std::vector<std::pair<hh_key, size_t>> counts;

        // copy key-value pairs from the map to the vector
        std::unordered_map<hh_key, hh_value>::iterator it2;
        for (it2 = counter_ptrs.begin(); it2 != counter_ptrs.end(); it2++) {
            sizes.push_back(std::make_pair(it2->first, it2->second.size));
            counts.push_back(std::make_pair(it2->first, it2->second.count));
        }

        print_list(sizes, total.size, "HEAVY HITTER: %s:%ld: %lu bytes (~%0.1f)\n");
        printf("\n");
        print_list(counts, total.count, "FREQUENT HITTER: %s:%ld: %lu times (~%0.1f)\n");
    }
};

heavy_hitters_arena hh_arena;

/// Global set of allocated memory blocks
static std::unordered_set<uintptr_t> allocs;

/// Metadata header struct for heap blocks
struct metadata {
    /// Size of the payload
    size_t sz;
    /// Pointer to file name where malloc was called
    const char *file;
    /// Line number where malloc was called
    long line;
};

/// Aligned size of metadata segment
static const size_t metadata_size = sizeof(metadata) + (-sizeof(metadata)) % ALIGNMENT;

/// Given a pointer to a heap payload, returns the pointer to the corresponding metadata segment
inline metadata *metadata_ptr(void *ptr) {
    return (metadata *)(reinterpret_cast<char *>(ptr) - metadata_size);
}

/// Size of padding segment after the payload (used to detect wild writes)
static const size_t padding_size = 1 * ALIGNMENT;

/// Byte to fill padding segment with
static const char magic_padding_byte = 0xFF;

/// Given a pointer to a heap payload, returns the pointer to the corresponding payload segment
inline char *padding_ptr(void *ptr, size_t sz) {
    return (char *)(reinterpret_cast<char *>(ptr) + sz);
}

/// Check if segment of memory is filled with 1s
bool check_padding(void *ptr, size_t sz) {
    char *ptr_temp = (char *)ptr;

    // Loop through the padding segment and check for modifications
    while (ptr_temp < (char *)ptr + sz) {
        if (*(ptr_temp++) != (char) magic_padding_byte) {
            return false;
        }
    }

    return true;
}

/// m61_malloc(sz, file, line)
///    Return a pointer to `sz` bytes of newly-allocated dynamic memory.
///    The memory is not initialized. If `sz == 0`, then m61_malloc must
///    return a unique, newly-allocated pointer value. The allocation
///    request was at location `file`:`line`.
void *m61_malloc(size_t sz, const char *file, long line) {
    (void)file, (void)line;  // avoid uninitialized variable warnings

    // Add to heavy hitters
    hh_arena.add(file, line, sz);

    // Calculate the total size of heap block
    size_t total_sz = sz + metadata_size + padding_size;

    // Check if the size is too big
    if (total_sz > sz && total_sz < MAX_MALLOC_SIZE) {
        // Malloc heap block with extra room for metadata
        char *ptr = (char *)base_malloc(total_sz);

        // Check if succesful
        if (ptr) {
            // Initialize metadata
            metadata block_data = {
                sz,
                file,
                line};

            // Fill metadata
            *(metadata *)ptr = block_data;
            ptr += metadata_size;

            // Fill padding
            memset(padding_ptr(ptr, sz), magic_padding_byte, padding_size);

            // Add to active allocations (will point to the payload)
            allocs.insert(reinterpret_cast<uintptr_t>(ptr));

            // Update statistics
            ++static_stats.nactive;
            ++static_stats.ntotal;
            static_stats.active_size += sz;
            static_stats.total_size += sz;

            if (reinterpret_cast<uintptr_t>(ptr) + sz > static_stats.heap_max) {
                static_stats.heap_max = reinterpret_cast<uintptr_t>(ptr) + sz;
            }

            if (reinterpret_cast<uintptr_t>(ptr) < static_stats.heap_min) {
                static_stats.heap_min = reinterpret_cast<uintptr_t>(ptr);
            }

            return ptr;
        }
    }

    // Memory allocation failed
    ++static_stats.nfail;
    static_stats.fail_size += sz;

    return NULL;
}

/// m61_free(ptr, file, line)
///    Free the memory space pointed to by `ptr`, which must have been
///    returned by a previous call to m61_malloc. If `ptr == NULL`,
///    does nothing. The free was called at location `file`:`line`.
void m61_free(void *ptr, const char *file, long line) {
    (void)file, (void)line;  // avoid uninitialized variable warnings

    // Make sure null pointer was not passed in
    if (ptr == NULL) {
        return;
    }

    // Check if pointer is in the heap
    if (static_stats.heap_min > reinterpret_cast<uintptr_t>(ptr) || static_stats.heap_max < reinterpret_cast<uintptr_t>(ptr)) {
        // Not in the heap
        fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, not in heap\n",
                file, line, ptr);
        abort();
    }

    // Check if pointer was previously allocated
    bool was_previously_allocated = allocs.find(reinterpret_cast<uintptr_t>(ptr)) != allocs.end();

    // Check if block was already freed
    bool was_previously_freed = check_padding(metadata_ptr(ptr), metadata_size);

    if (was_previously_freed && !was_previously_allocated) {
        // Must be a double free
        fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, double free\n",
                file, line, ptr);
        abort();
    }

    // Move pointer to beginning of heap block
    char *block_ptr = (char *)metadata_ptr(ptr);
    metadata data = *(metadata *)block_ptr;

    // Check the padding to see if there was a wild write
    bool was_wild_write = !check_padding(padding_ptr(ptr, data.sz), padding_size);

    if (was_previously_allocated && was_wild_write) {
        // Must be a wild write issue
        fprintf(stderr, "MEMORY BUG: %s:%ld: detected wild write during free of pointer %p\n",
                file, line, ptr);
        abort();
    }

    if (!was_previously_allocated) {
        // Must be a not allocated issue
        fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, not allocated\n",
                file, line, ptr);

        // Try finding if this is a pointer inside an existing heap block
        metadata base_metadata;

        for (auto base_block_ptr : allocs) {
            base_metadata = *(metadata *)metadata_ptr(reinterpret_cast<void *>(base_block_ptr));

            // Check if ptr is inside of heap block and print overlap
            if (base_block_ptr < reinterpret_cast<uintptr_t>(ptr) && reinterpret_cast<uintptr_t>(ptr) <= base_block_ptr + base_metadata.sz) {
                fprintf(stderr, "  %s:%ld: %p is %lu bytes inside a %lu byte region allocated here\n",
                        base_metadata.file, base_metadata.line, ptr, reinterpret_cast<uintptr_t>(ptr) - base_block_ptr, base_metadata.sz);

                break;
            }
        }

        abort();
    }

    // Update stats
    --static_stats.nactive;
    static_stats.active_size -= data.sz;

    // Mark the heap block as freed
    memset(metadata_ptr(ptr), magic_padding_byte, metadata_size);

    // Erase from active allocations
    allocs.erase(reinterpret_cast<uintptr_t>(ptr));

    base_free(block_ptr);
}

/// m61_calloc(nmemb, sz, file, line)
///    Return a pointer to newly-allocated dynamic memory big enough to
///    hold an array of `nmemb` elements of `sz` bytes each. If `sz == 0`,
///    then must return a unique, newly-allocated pointer value. Returned
///    memory should be initialized to zero. The allocation request was at
///    location `file`:`line`.
void *m61_calloc(size_t nmemb, size_t sz, const char *file, long line) {
    // Make sure malloc size isn't too large
    if (sz <= nmemb * sz && nmemb <= nmemb * sz) {
        void *ptr = m61_malloc(nmemb * sz, file, line);

        // If malloc was succesful, clear allocated region and return ptr.
        if (ptr) {
            memset(ptr, 0, nmemb * sz);
            return ptr;
        }
    } else {
        // Memory allocation failed
        ++static_stats.nfail;
        static_stats.fail_size += nmemb * sz;
    }

    return nullptr;
}

/// m61_realloc(ptr, sz, file, line)
///    Reallocate the dynamic memory pointed to by `ptr` to hold at least
///    `sz` bytes, returning a pointer to the new block. If `ptr` is
///    `nullptr`, behaves like `m61_malloc(sz, file, line)`. If `sz` is 0,
///    behaves like `m61_free(ptr, file, line)`. The allocation request
///    was at location `file`:`line`.
void *m61_realloc(void *ptr, size_t sz, const char *file, long line) {
    if (ptr == nullptr) {
        return m61_malloc(sz, file, line);
    }

    // Check if given pointer was previously allocated
    if (allocs.find(reinterpret_cast<uintptr_t>(ptr)) != allocs.end()) {
        // Extract the size of the old heap block
        size_t old_sz = metadata_ptr(ptr)->sz;

        // Free the old block
        m61_free(ptr, file, line);

        if (sz != 0) {
            void *new_ptr = m61_malloc(sz, file, line);

            // If allocation was succesful, copy data from old heap block to new heap block
            if (new_ptr != nullptr) {
                memccpy(new_ptr, ptr, 1, std::min(old_sz, sz));
            }

            return new_ptr;
        }
    } else {
        fprintf(stderr, "MEMORY BUG: %s:%ld: invalid realloc of pointer %p, not allocated\n",
                file, line, ptr);
        abort();
    }

    return nullptr;
}

/// Store the current memory statistics in `*stats`.
void m61_get_statistics(m61_statistics *stats) {
    *stats = static_stats;
}

/// Print the current memory statistics.
void m61_print_statistics() {
    m61_statistics stats;
    m61_get_statistics(&stats);

    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

/// Print a report of all currently-active allocated blocks of dynamic memory
void m61_print_leak_report() {
    metadata data;

    // Loop through active allocated blocks
    for (uintptr_t leak : allocs) {
        // Fetch metadata
        data = *metadata_ptr(reinterpret_cast<void*>(leak));

        // Print leak report
        printf("LEAK CHECK: %s:%ld: allocated object %p with size %ld\n",
               data.file, data.line, reinterpret_cast<char *>(leak), data.sz);
    }
}

/// Print a report of heavily-used allocation locations.
void m61_print_heavy_hitter_report() {
    hh_arena.print_report();
}
