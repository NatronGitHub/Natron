// https://github.com/cctbx/cctbx_project/blob/master/boost_adaptbx/floating_point_exceptions.h
#ifndef FLOATING_POINT_EXCEPTIONS_H
#define FLOATING_POINT_EXCEPTIONS_H

#ifdef __GNUC__
#include <fenv.h>
#ifdef __SSE2__
#include <xmmintrin.h>
#endif
#endif


namespace boost_adaptbx { namespace floating_point {

  /** Floating point exception trapping

      Background information:
      gcc on x86-64 (all platforms) [2],
      Apple gcc on Intel macs [1]:
        floating-point math is done on vector unit (SSE2) by default

      gcc on i386 (all platform except Apple) [2]:
        floating-point math is done on scalar unit (i387)

      Linux:
        feenableexcept enables/disables FP exception trapping for i387 and SSE2
      gcc on all Intel platform:
        _MM_SET_EXCEPTION_MASK from xmmintrin.h enables/disables exception
        trapping for SSE2

      [1] http://developer.apple.com/documentation/performance/Conceptual/Accelerate_sse_migration/migration_sse_translation/chapter_4_section_2.html
      [2] GCC documentation
  */

  inline void trap_exceptions(bool division_by_zero, bool invalid, bool overflow) {
    #ifdef __linux
      int enabled = 0;
      int disabled = 0;
      #ifdef FE_DIVBYZERO
        if (division_by_zero) enabled |= FE_DIVBYZERO;
        else disabled |= FE_DIVBYZERO;
      #endif
      #ifdef FE_INVALID
        if (invalid) enabled |= FE_INVALID;
        else disabled |= FE_INVALID;
      #endif
      #ifdef FE_OVERFLOW
        if(overflow) enabled |= FE_OVERFLOW;
        else disabled |= FE_OVERFLOW;
      #endif
      fedisableexcept(disabled);
      feenableexcept(enabled);
    #elif defined(__APPLE_CC__) && defined(__SSE2__)
#if 1
      unsigned int mask
        = _MM_MASK_INEXACT | _MM_MASK_UNDERFLOW | _MM_MASK_DENORM;
      if (!division_by_zero) mask |= _MM_MASK_DIV_ZERO;
      if (!invalid) mask |= _MM_MASK_INVALID;
      if (!overflow) mask |= _MM_MASK_OVERFLOW;
#else
      unsigned int mask = _MM_GET_EXCEPTION_MASK();
      if (division_by_zero) mask &= ~_MM_MASK_DIV_ZERO;
      if (invalid) mask &= ~_MM_MASK_INVALID;
      if (overflow) mask &= ~_MM_MASK_OVERFLOW;
#endif
      _MM_SET_EXCEPTION_MASK(mask);
    #endif
  }

  inline bool is_division_by_zero_trapped() {
    #ifdef __linux
      #ifdef FE_DIVBYZERO
        return fegetexcept() & FE_DIVBYZERO;
      #else
        return false;
      #endif
    #elif defined(__APPLE_CC__) && defined(__SSE2__)
      return !(_MM_GET_EXCEPTION_MASK() & _MM_MASK_DIV_ZERO);
    #else
      return false;
    #endif
  }

  inline bool is_invalid_trapped() {
    #ifdef __linux
      #ifdef FE_INVALID
        return fegetexcept() & FE_INVALID;
      #else
        return false;
      #endif
    #elif defined(__APPLE_CC__) && defined(__SSE2__)
      return !(_MM_GET_EXCEPTION_MASK() & _MM_MASK_INVALID);
    #else
      return false;
    #endif
  }

  inline bool is_overflow_trapped() {
    #ifdef __linux
      #ifdef FE_OVERFLOW
        return fegetexcept() & FE_OVERFLOW;
      #else
        return false;
      #endif
    #elif defined(__APPLE_CC__) && defined(__SSE2__)
      return !(_MM_GET_EXCEPTION_MASK() & _MM_MASK_OVERFLOW);
    #else
      return false;
    #endif
  }

  class exception_trapping
  {
    private:
      bool division_by_zero_on_entry, invalid_on_entry, overflow_on_entry;
      bool division_by_zero_, invalid_, overflow_;

    public:
      enum { dont_trap=0, division_by_zero=1, invalid=2, overflow=4 };

      exception_trapping(int flag)
        : division_by_zero_on_entry(is_division_by_zero_trapped()),
          invalid_on_entry(is_invalid_trapped()),
          overflow_on_entry(is_overflow_trapped()),
          division_by_zero_(flag & division_by_zero),
          invalid_(flag & invalid),
          overflow_(flag & overflow)
      {
        trap_exceptions(division_by_zero_, invalid_, overflow_);
      }

      ~exception_trapping() {
        trap_exceptions(division_by_zero_on_entry,
                        invalid_on_entry,
                        overflow_on_entry);
      }
  };

}} // boost_adaptbx::floating_point

#endif // GUARD
