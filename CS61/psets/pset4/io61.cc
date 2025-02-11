#include "io61.hh"
#include <algorithm>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <climits>
#include <cerrno>

// io61.c
//    YOUR CODE HERE!


// io61_file
//    Data structure for io61 file wrappers. Add your own stuff.

struct io61_file {
    int fd;
    static constexpr off_t bufsize = 4096; // block size for this cache
    unsigned char cbuf[bufsize];
    // These “tags” are addresses—file offsets—that describe the cache’s contents.
    off_t tag;      // file offset of first byte in cache (0 when file is opened)
    off_t end_tag;  // file offset one past last valid byte in cache
    off_t pos_tag;  // file offset of next char to read in cache
    int mode;
        
    bool filled() {
        return this->end_tag - this->tag == this->bufsize;
    };

    unsigned char* cache_ptr() {
        return &cbuf[this->pos_tag - this->tag];
    };

    size_t data_size() {
        return this->end_tag - this->pos_tag;
    };
};

inline void io61_check_invariants(io61_file* f) {
    (void) f;
    //assert(f->tag <= f->pos_tag && f->pos_tag <= f->end_tag);
    //assert(f->end_tag - f->pos_tag <= f->bufsize);
}

inline void io61_check_write_invariants(io61_file* f) {
    (void) f;
    //assert(f->data_size() == 0);
}

void io61_reset_cache(io61_file* f) {
    f->tag = f->pos_tag = f->end_tag;
}

ssize_t io61_fill(io61_file* f) {
    io61_check_invariants(f);
    
    io61_reset_cache(f);

    ssize_t n = read(f->fd, f->cbuf, f->bufsize);
    if (n >= 0) {
        f->end_tag = f->tag + n;
    }

    io61_check_invariants(f);

    return n; 
}

// io61_fdopen(fd, mode)
//    Return a new io61_file for file descriptor `fd`. `mode` is
//    either O_RDONLY for a read-only file or O_WRONLY for a
//    write-only file. You need not support read/write files.

io61_file* io61_fdopen(int fd, int mode) {
    assert(fd >= 0);
    io61_file* f = new io61_file;
    f->fd = fd;
    f->mode = mode;
    f->tag = f->pos_tag = f->end_tag = 0;
    return f;
}


// io61_close(f)
//    Close the io61_file `f` and release all its resources.

int io61_close(io61_file* f) {
    io61_flush(f);
    int r = close(f->fd);
    delete f;
    return r;
}


// io61_read(f, buf, sz)
//    Read up to `sz` characters from `f` into `buf`. Returns the number of
//    characters read on success; normally this is `sz`. Returns a short
//    count, which might be zero, if the file ended before `sz` characters
//    could be read. Returns -1 if an error occurred before any characters
//    were read.

ssize_t io61_read(io61_file* f, unsigned char* buf, size_t sz) {
    io61_check_invariants(f);

    size_t pos = 0;
    size_t cpy_size = 0;

    while (pos < sz) {
        if (f->pos_tag == f->end_tag) {
            if (io61_fill(f) == -1) {
                return -1;
            }

            if (f->pos_tag == f->end_tag) {
                break;
            }
        }

        if (sz - pos > f->data_size()) {
           cpy_size = f->data_size();
        } else {
            cpy_size = sz - pos;   
        }

        memcpy(&buf[pos], f->cache_ptr(), cpy_size);
        f->pos_tag += cpy_size;
        pos += cpy_size;
    }

    return pos;
}


// io61_readc(f)
//    Read a single (unsigned) character from `f` and return it. Returns EOF
//    (which is -1) on error or end-of-file.

int io61_readc(io61_file* f) {
    unsigned char buf[1];
    if (io61_read(f, buf, 1) == 1) {
        return buf[0];
    } else {
        return EOF;
    }
}


// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success or
//    -1 on error.

int io61_writec(io61_file* f, int ch) {
    unsigned char buf[1];
    buf[0] = ch;
    if (io61_write(f, buf, 1) == 1) {
        return 0;
    } else {
        return -1;
    }
}


// io61_write(f, buf, sz)
//    Write `sz` characters from `buf` to `f`. Returns the number of
//    characters written on success; normally this is `sz`. Returns -1 if
//    an error occurred before any characters were written.

ssize_t io61_write(io61_file* f, const unsigned char* buf, size_t sz) {
    io61_check_invariants(f);

    // Write cache invariant.
    io61_check_write_invariants(f);

    size_t newsz = 0;
    size_t pos = 0;
    while (pos < sz) {
        if (f->end_tag == f->tag + f->bufsize) {
            io61_flush(f);
        }

        if (sz - pos > (size_t) (f->bufsize + f->tag - f->end_tag)) {
            newsz = (size_t) (f->bufsize + f->tag - f->end_tag);
        } else {
            newsz = sz - pos;
        }   

        memcpy(f->cache_ptr(), &buf[pos], newsz);
        f->pos_tag += newsz;
        f->end_tag += newsz;
        pos += newsz;
    }
    return pos;
}


// io61_flush(f)
//    Forces a write of all buffered data written to `f`.
//    If `f` was opened read-only, io61_flush(f) may either drop all
//    data buffered for reading, or do nothing.

int io61_flush(io61_file* f) {
    if (f->mode != O_WRONLY) {
        return -1;
    }

    io61_check_invariants(f);

    io61_check_write_invariants(f);

    ssize_t n = write(f->fd, f->cbuf, f->pos_tag - f->tag);
    assert((size_t) n == (size_t) f->pos_tag - f->tag);
    f->tag = f->pos_tag;

    return 0;
}


// io61_seek(f, pos)
//    Change the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.

int io61_seek(io61_file* f, off_t pos) {
    io61_check_invariants(f);
    
    // Check if lseek is inside cache
    if (f->tag <= pos && pos <= f->end_tag) {
        f->pos_tag = pos;
        return 0;
    } else {
        // Otherwise flush the cache and fill again
        f->pos_tag = f->end_tag;
        io61_flush(f);
        
        // Move back to an aligned pos
        ssize_t aligned_pos = f->bufsize * ((size_t) pos / f->bufsize);
        off_t r = lseek(f->fd, (off_t) aligned_pos, SEEK_SET);
        
        if (r == aligned_pos) {
            f->tag = f->end_tag = f->pos_tag = aligned_pos;
           
            // Fill 
            if (io61_fill(f) >= 0) {
                f->pos_tag = pos;
                return 0;
            }
        }

        return -1;
    }
}


// You shouldn't need to change these functions.

// io61_open_check(filename, mode)
//    Open the file corresponding to `filename` and return its io61_file.
//    If `!filename`, returns either the standard input or the
//    standard output, depending on `mode`. Exits with an error message if
//    `filename != nullptr` and the named file cannot be opened.

io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    if (filename) {
        fd = open(filename, mode, 0666);
    } else if ((mode & O_ACCMODE) == O_RDONLY) {
        fd = STDIN_FILENO;
    } else {
        fd = STDOUT_FILENO;
    }
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}


// io61_filesize(f)
//    Return the size of `f` in bytes. Returns -1 if `f` does not have a
//    well-defined size (for instance, if it is a pipe).

off_t io61_filesize(io61_file* f) {
    struct stat s;
    int r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode)) {
        return s.st_size;
    } else {
        return -1;
    }
}
