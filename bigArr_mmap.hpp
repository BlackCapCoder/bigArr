#pragma once

#include <cstdint>
#include <bit>
#include <tuple>
#include <sys/mman.h>

// ----

// A dynamically allocated array indexed by uint64_t's
// It also contains uint64's, and they are conveniently
// zero initialized and the same size as a pointer!
//
struct BigArr
{
  struct Chunk
  {
    static constexpr auto size = 256;

    union
    {
      Chunk *  chunks [size] {};
      uint64_t values [size];
    };

    // unlike `new Chunk`, mmap allow us to alloc multiple Chunk's at
    // the same time, but free them individually
    static Chunk * alloc (int n)
    {
      return reinterpret_cast <Chunk *>
        (mmap (NULL, sizeof (Chunk) * n, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    }
  };

  Chunk * first_chunk = Chunk::alloc (1);

  // ----

  // Return a pointer to the chunk where a value lives, and its index within that chunk
  std::tuple<uint64_t *, uint8_t> index_chunk (uint64_t ix)
  {
    // Swapping the order makes it so you only alloc a new Chunk whenever you
    // cross a multiple of 256, hence this function! If you don't plan on
    // crossing the 256-point you can just run this function once and operate
    // on the local array instead
    ix = std::byteswap (ix);

    Chunk * cur = first_chunk;
    for (int n = 0; n < 7; n++)
    {
      Chunk *& next = cur->chunks[ix & 0xff];
      if (next == nullptr)
      {
        Chunk * mem = Chunk::alloc (7 - n);
        for (; n < 7; n++, ix >>= 8)
          cur = cur->chunks[ix & 0xff] = mem++;
        break;
      }
      cur = next; ix >>= 8;
    }

    return std::make_tuple
      ( cur->values
      , (uint8_t) ix
      );
  }
  uint64_t & index_v (uint64_t ix)
  {
    auto [ptr, off] = index_chunk (ix);
    return ptr[off];
  }
  uint64_t & operator [] (uint64_t ix)
  {
    return index_v (ix);
  }

  // ----

  // index by 2D-coordinates as-if each Chunk was a 16x16 square
  // you get to a new chunk whenever you cross a multiple of 16 in either direction
  std::tuple<uint64_t *, uint8_t, uint8_t> index2_chunk (uint32_t x, uint32_t y)
  {
    x = std::byteswap (x), y = std::byteswap (y);
    Chunk * cur = first_chunk;

    // indexing is slightly awkward because byteswap flips every pair of nibbles,
    // but byteswap is still faster than tearing apart the original numbers in reverse.
    // The resulting index are the nibbles of the two input numbers weaved together in reverse order
    for (int i = 0; ; i++)
    {
      const uint8_t lx1 = x & 0xf; x >>= 4;
      const uint8_t lx2 = x & 0xf; x >>= 4;
      const uint8_t ly1 = y & 0xf; y >>= 4;
      const uint8_t ly2 = y & 0xf; y >>= 4;
      // todo: alloc everything in one go
      { Chunk *& next = cur->chunks[lx2 + (ly2 << 4)]; if (next == nullptr) next = Chunk::alloc(1); cur = next; }
      if (i < 3)
      { Chunk *& next = cur->chunks[lx1 + (ly1 << 4)]; if (next == nullptr) next = Chunk::alloc(1); cur = next; }
      else
      return std::make_tuple
        (cur->values, lx1, ly1);
    }
  }
  uint64_t & index2_v (uint32_t x, uint32_t y)
  {
    auto && [ptr, lx, ly] = index2_chunk (x, y);
    return ptr [lx + (ly << 4)];
  }

  // ----

  // Copy from a regular array into the big array
  void write (uint64_t dst, uint64_t * src, uint32_t n)
  {
    std::tuple <Chunk *, uint8_t> stack [7] {};
    uint8_t SP = 0;

    Chunk * cur = first_chunk;
    auto ix = std::byteswap (dst);

    while (true)
    {
      do {
        const uint8_t lx = ix & 0xff;
        stack[SP++] = (std::make_tuple (cur, lx));
        Chunk *& next = cur->chunks[lx];
        if (next == nullptr) next = Chunk::alloc(1);
        cur = next;
        ix >>= 8;
      } while (SP < 7);
      for (; ix < 256; ix++) {
        *src++ = cur->values[ix];
        if (--n == 0) return;
      }
      do {
        const auto [cur_, lx_] = stack[--SP];
        cur = cur_;
        ix = lx_ + 1;
      } while (ix == 256);
    }
  }

  // ----

  // Recursively free all the chunks, but not the values within them
  //
  ~BigArr ()
  {
    free_chunk (first_chunk, 8);
    first_chunk = nullptr;
  }

  // invariants:
  //   - n >= 2
  //   - ptr != nullptr
  //
  void free_chunk (Chunk * ptr, int n)
  {
    if (n==2)
    {
      for (int i = 0; i < Chunk::size; i++)
      {
        if (ptr->chunks[i] == nullptr) continue;
        munmap (ptr->chunks[i], sizeof (Chunk));
      }
    }
    else
    {
      for (int i = 0; i < Chunk::size; i++)
      {
        if (ptr->chunks[i] == nullptr) continue;
        free_chunk (ptr->chunks[i], n-1);
      }
    }

    munmap (ptr, sizeof (Chunk));
  }
};
