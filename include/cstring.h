#ifndef STDX_CSTRING_H
#define STDX_CSTRING_H

#include <atomic>
#include <cstring>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif

//
//  C23
//

/**
 * \brief The memset_explicit function copies the value of @a ch
 *        (converted to an unsigned char) into each of the first @a count
 *        characters of the object pointed to by @a dest.
 *        The purpose of this function is to make sensitive information stored
 *        in the object inaccessible.
 * \param dest pointer to the object to fill
 * \param ch fill byte
 * \param count number of bytes to fill
 * \return a copy of dest
 * \note C++ proposal: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1315r6.html
 * \note The intention is that the memory store is always performed (i.e., never elided),
 *       regardless of optimizations. This is in contrast to calls to the memset function.
 */
inline void* memset_explicit(void *dest, int ch, size_t count)
{
  auto* s = memset(dest, ch, count);
  // https://stackoverflow.com/questions/50428450/what-does-asm-volatile-pause-memory-do
  // https://preshing.com/20120625/memory-ordering-at-compile-time/
  // when -O2 or -O3 is on
  // the following line prevents the compiler to optimize away the call of memset
  // https://stackoverflow.com/questions/14449141/the-difference-between-asm-asm-volatile-and-clobbering-memory
  // compiler barrier:
  // - the linux inline assembler is not allowed to be used by the project coding rules
  // asm volatile ("" ::: "memory");
  // - the windows compiler intrinsic _ReadWriteBarrier is deprecated
  //  https://docs.microsoft.com/en-us/cpp/intrinsics/readwritebarrier?view=msvc-160
  //
  // the msvc /std:c++17 /Ot - without a compiler_barrier doesn't optimize away the call of memset
  // the linux g++ 9.3.0 -O2 - without a compiler_barrier the call of memset is optimized away
  //
  // std::atomic_thread_fence:
  // gcc 9.3.0 -std=c++17 -O2 generates mfence asm instruction; the call of memset is not optimized away
  //
  // std::atomic_signal_fence:
  // gcc 9.3.0 -std=c++17 -O2 no mfence asm instruction is generated,
  // however the call of memset is not optimized away too
  std::atomic_signal_fence(std::memory_order_seq_cst);

  return s;
}

#ifdef __cplusplus
}
#endif

namespace stdx
{
  //
  //  CXX23
  //

  /**
   * \brief Copies the value of @a ch (converted to an unsigned char) into each byte of
   *        the object pointed to by @a dest.
   *        The purpose of this function is to make sensitive information stored
   *        in the object inaccessible.
   * \tparam T the type of object
   * \param that reference to the object to fill
   * \param ch fill byte
   * \note C++ proposal: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1315r6.html
   * \note The intention is that the memory store is always performed (i.e., never elided),
   *       regardless of optimizations. This is in contrast to calls to the memset function.
   */
  template<typename T,
    std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>>* = nullptr>
  void memset_explicit(T& that, int ch) noexcept
  {
     memset_explicit(std::addressof(that), ch, sizeof(that));
  }
}

#endif
