#pragma once

#include <cstdint>
#include <bit>
#include <tuple>

// ----

// A dynamically allocated array indexed by uint64_t's
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
  };

  Chunk * first_chunk = new Chunk {};

  // ----

  // Return a pointer to the chunk where a value lives, and its index within that chunk
  //
  std::tuple<uint64_t *, uint8_t> index_chunk (uint64_t ix)
  {
    // Swapping the order makes it so indicies that are close together ends up in the same chunk
    ix = std::byteswap (ix);

    Chunk * cur = first_chunk;
    for (int n = 0; n < 7; n++)
    {
      Chunk *& next = cur->chunks[ix & 0xFF];
      if (next == nullptr) next = new Chunk {};
      cur = next;
      ix >>= 8;
    }

    return std::make_tuple
      ( cur->values
      , (uint8_t) ix
      );
  }

  uint64_t & operator [] (uint64_t ix)
  {
    auto [ptr, off] = index_chunk (ix);
    return ptr[off];
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
        delete (ptr->chunks[i]);
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

    delete ptr;
  }
};

