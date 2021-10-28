#if defined(linux) || defined(__linux) || defined(__linux__)

#include <cstring.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

static const unsigned char pattern[16] =
{
  2, 3, 5, 7,  11, 13, 17, 19,  23, 29, 31, 37,  41, 43, 47, 53
};

constexpr auto pattern_size = sizeof(pattern);
constexpr std::size_t pattern_rep = 4095U;
constexpr auto buffer_size = pattern_size * pattern_rep;

inline void prepare_buffer(unsigned char* buf)
{
  for (std::size_t i = 0U; i < pattern_rep; ++i)
  {
    memcpy(buf + i * pattern_size, pattern, pattern_size);
  }
}

void* ordinary_clear_func(void* argv)
{
  unsigned char buf[buffer_size];
  prepare_buffer(buf);
  memset(buf, 0, buffer_size);
  std::this_thread::sleep_for(500ms);
  return argv;
}

void* explicit_clear_func(void* argv)
{
  unsigned char buf[buffer_size];
  prepare_buffer(buf);
  memset_explicit(buf, 0, buffer_size);
  std::this_thread::sleep_for(500ms);
  return argv;
}

TEST(CString, explicit_clear)
{
  pthread_attr_t attr;
  pthread_t tid;
  const int min_stack_size = 0x10000U; //64KB
  const int stack_size = 2 * min_stack_size;
  const size_t alignment = 4096U;
  void *stack_base = std::aligned_alloc(alignment, min_stack_size * 3);

  ASSERT_EQ(pthread_attr_init(&attr), 0);
  ASSERT_EQ(pthread_attr_setstack(&attr, stack_base, stack_size), 0);
  ASSERT_EQ(pthread_create(&tid, &attr, explicit_clear_func, nullptr), 0);

  std::this_thread::sleep_for(5ms);

  EXPECT_NE(
    memcmp(static_cast<unsigned char*>(stack_base) + stack_size - buffer_size, pattern, pattern_size),
    0);

  EXPECT_EQ(pthread_join(tid, nullptr), 0);
  EXPECT_EQ(pthread_attr_destroy(&attr), 0);

  std::free(stack_base);
}

TEST(CString, ordinary_clear)
{
  pthread_attr_t attr;
  pthread_t tid;
  const int min_stack_size = 0x10000U; //64KB
  const int stack_size = 2 * min_stack_size;
  const size_t alignment = 4096U;
  void *stack_base = std::aligned_alloc(alignment, min_stack_size * 3);

  ASSERT_EQ(pthread_attr_init(&attr), 0);
  ASSERT_EQ(pthread_attr_setstack(&attr, stack_base, stack_size), 0);
  ASSERT_EQ(pthread_create(&tid, &attr, ordinary_clear_func, nullptr), 0);

  std::this_thread::sleep_for(5ms);

#if defined(NDEBUG)
  // ordinary memset is optimized away
  EXPECT_EQ(
    memcmp(static_cast<unsigned char*>(stack_base) + stack_size - buffer_size, pattern, pattern_size),
    0);
#else
  // ordinary memset is not optimized away
  EXPECT_NE(
    memcmp(static_cast<unsigned char*>(stack_base) + stack_size - buffer_size, pattern, pattern_size),
    0);
#endif

  EXPECT_EQ(pthread_join(tid, nullptr), 0);
  EXPECT_EQ(pthread_attr_destroy(&attr), 0);

  std::free(stack_base);
}

#endif
