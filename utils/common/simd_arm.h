/* Copyright (C) 2021-2022 Mariadb Corporation.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA. */

#pragma once


#ifdef __aarch64__
#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>
#endif
#include "arm_neon.h"
#include <cstdint>
#include <type_traits>
#ifdef __OPTIMIZE__
#define MCS_FORCE_INLINE __attribute__((__always_inline__)) inline
#else
#define MCS_FORCE_INLINE inline
#endif

#include "mcs_datatype.h"


namespace simd
{
#ifdef __ARM_FEATURE_SVE
using MT = svbool_t;
using PT = svbool_t;
MCS_FORCE_INLINE MT MTAllTrue()
{
  return svptrue_b8();
}
MCS_FORCE_INLINE MT MTAllFalse()
{
  return svpfalse();
}
MCS_FORCE_INLINE MT MTAnd(PT& pg, MT& x, MT& y)
{
  return svand_z(pg, x, y);
}
MCS_FORCE_INLINE MT MTOr(PT& pg, MT& x, MT& y)
{
  return svorr_z(pg, x, y);
}
MCS_FORCE_INLINE MT MTEor(PT& pg, MT& x, MT& y)
{
  return sveor_z(pg, x, y);
}
template <typename T>
struct TypeToVecWrapperType;
template <>
struct TypeToVecWrapperType<int8_t>
{
  using SimdType = svint8_t;
};
template <>
struct TypeToVecWrapperType<uint8_t>
{
  using SimdType = svuint8_t;
};
template <>
struct TypeToVecWrapperType<int16_t>
{
  using SimdType = svint16_t;
};
template <>
struct TypeToVecWrapperType<uint16_t>
{
  using SimdType = svuint16_t;
};
template <>
struct TypeToVecWrapperType<int32_t>
{
  using SimdType = svint32_t;
};
template <>
struct TypeToVecWrapperType<uint32_t>
{
  using SimdType = svuint32_t;
};
template <>
struct TypeToVecWrapperType<int64_t>
{
  using SimdType = svint64_t;
};
template <>
struct TypeToVecWrapperType<uint64_t>
{
  using SimdType = svuint64_t;
};
template <>
struct TypeToVecWrapperType<float>
{
  using SimdType = svfloat32_t;
};
template <>
struct TypeToVecWrapperType<double>
{
  using SimdType = svfloat64_t;
};
template <typename T, ENUM_KIND KIND, typename ENABLE = void>
struct StorageToFiltering;
template <typename T, ENUM_KIND KIND>
struct StorageToFiltering<T, KIND,
                          typename std::enable_if<KIND == KIND_FLOAT && sizeof(double) == sizeof(T)>::type>
{
  using type = double;
};

template <typename T, ENUM_KIND KIND>
struct StorageToFiltering<T, KIND,
                          typename std::enable_if<KIND == KIND_FLOAT && sizeof(float) == sizeof(T)>::type>
{
  using type = float;
};

template <typename T, ENUM_KIND KIND>
struct StorageToFiltering<T, KIND, typename std::enable_if<KIND != KIND_FLOAT>::type>
{
  using type = T;
};
template <typename T>
class SimdFilterProcessor
{
 public:
  using PredicationType = svbool_t;
  using SimdType = typename TypeToVecWrapperType<T>::SimdType;
  using FilterType = T;
  MCS_FORCE_INLINE PredicationType getSinglePredication(uint64_t i)
  {
    PredicationType op1 = getRangePredication(0, i);
    PredicationType op2 = getRangePredication(0, i + 1);
    return MTEor(op2, op1, op2);
  }
  MCS_FORCE_INLINE PredicationType getRangePredication(uint64_t from, uint64_t to)
  {
    if constexpr (sizeof(T) == 1)
      return svwhilelt_b8(from, to);
    else if constexpr (sizeof(T) == 2)
      return svwhilelt_b16(from, to);
    else if constexpr (sizeof(T) == 4)
      return svwhilelt_b32(from, to);
    else if constexpr (sizeof(T) == 8)
      return svwhilelt_b64(from, to);
  }
  MCS_FORCE_INLINE uint64_t stepLength()
  {
    if constexpr (sizeof(T) == 1)
      return svcntb();
    else if constexpr (sizeof(T) == 2)
      return svcnth();
    else if constexpr (sizeof(T) == 4)
      return svcntw();
    else if constexpr (sizeof(T) == 8)
      return svcntd();
  }
  MCS_FORCE_INLINE SimdType loadValue(PredicationType& pg, const T fill)
  {
    if constexpr (is_same_v<T, int8_t>)
      return svdup_s8_x(pg, fill);
    else if constexpr (is_same_v<T, uint8_t>)
      return svdup_u8_x(pg, fill);
    else if constexpr (is_same_v<T, int16_t>)
      return svdup_s16_x(pg, fill);
    else if constexpr (is_same_v<T, uint16_t>)
      return svdup_u16_x(pg, fill);
    else if constexpr (is_same_v<T, int32_t>)
      return svdup_s32_x(pg, fill);
    else if constexpr (is_same_v<T, uint32_t>)
      return svdup_u32_x(pg, fill);
    else if constexpr (is_same_v<T, int64_t>)
      return svdup_s64_x(pg, fill);
    else if constexpr (is_same_v<T, uint64_t>)
      return svdup_u64_x(pg, fill);
    else if constexpr (is_same_v<T, float>)
      return svdup_f32_x(pg, fill);
    else if constexpr (is_same_v<T, double>)
      return svdup_f64_x(pg, fill);
  }
  MCS_FORCE_INLINE SimdType loadFrom(PredicationType& pg, const char* from)
  {
    return svld1(pg, reinterpret_cast<const T*>(from));
  }
  MCS_FORCE_INLINE SimdType loadFromIndex(PredicationType& pg, const char* base, const uint16_t* index)
  {
    SimdType value;
    uint64_t n = stepLength();
    for (uint64_t i = 0; i < n; ++i)
      *((T*)&value + i) = reinterpret_cast<const T*>(base)[index[i]];
    return value;
  }
  MCS_FORCE_INLINE MT cmpEq(PredicationType& pg, SimdType& x, T& y)
  {
    return svcmpeq(pg, x, y);
  }
  MCS_FORCE_INLINE MT nullEmptyCmpNe(PredicationType& pg, SimdType& x, T y)
  {
    if constexpr (std::is_same_v<T, float>)
    {
      return svcmpne(pg, svreinterpret_s32(x),static_cast<int32_t>( y));
    }
    else if constexpr (std::is_same_v<T, double>)
    {
      return svcmpne(pg, svreinterpret_s64(x),static_cast<int64_t>( y));
    }
    return cmpNe(pg, x, y);
  }
  MCS_FORCE_INLINE MT cmpNe(PredicationType& pg, SimdType& x, T& y)
  {
    return svcmpne(pg, x, y);
  }
  MCS_FORCE_INLINE MT cmpGt(PredicationType& pg, SimdType& x, T& y)
  {
    return svcmpgt(pg, x, y);
  }
  MCS_FORCE_INLINE MT cmpGe(PredicationType& pg, SimdType& x, T& y)
  {
    return svcmpge(pg, x, y);
  }
  MCS_FORCE_INLINE MT cmpLe(PredicationType& pg, SimdType& x, T& y)
  {
    return svcmple(pg, x, y);
  }
  MCS_FORCE_INLINE MT cmpLt(PredicationType& pg, SimdType& x, T& y)
  {
    return svcmplt(pg, x, y);
  }
  MCS_FORCE_INLINE MT cmple(PredicationType& pg, SimdType& x, T& y)
  {
    return svcmple(pg, x, y);
  }
  MCS_FORCE_INLINE MT cmpAlwaysFalse(PredicationType& pg, SimdType& x, T& y)
  {
    return svpfalse();
  }
  MCS_FORCE_INLINE MT cmpAlwaysTrue(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE bool testAny(const PredicationType& pg, const PredicationType& op)
  {
    return svptest_any(pg, op);
  }
  MCS_FORCE_INLINE void store(PredicationType& pg, char* dst, SimdType& x)
  {
    svst1(pg, reinterpret_cast<T*>(dst), x);
  }
  MCS_FORCE_INLINE uint64_t countElements(PredicationType& pg)
  {
    if constexpr (sizeof(T) == 1)
      return svcntp_b8(pg, pg);
    else if constexpr (sizeof(T) == 2)
      return svcntp_b16(pg, pg);
    else if constexpr (sizeof(T) == 4)
      return svcntp_b32(pg, pg);
    else if constexpr (sizeof(T) == 8)
      return svcntp_b64(pg, pg);
  }
};

template <>
class SimdFilterProcessor<__int128>
{
 public:
  using T=int64_t;
  using PredicationType = svbool_t;
  using SimdType = svint64_t;
  using FilterType = int64_t;
  MCS_FORCE_INLINE PredicationType getSinglePredication(uint64_t i)
  {
    PredicationType op1 = getRangePredication(0, i);
    PredicationType op2 = getRangePredication(0, i + 1);
    return MTEor(op2, op1, op2);
  }
  MCS_FORCE_INLINE PredicationType getRangePredication(uint64_t from, uint64_t to)
  {
      return svwhilelt_b64(from, to);
  }
  MCS_FORCE_INLINE uint64_t stepLength()
  {
      return svcntd();
  }
  MCS_FORCE_INLINE SimdType loadValue(PredicationType& pg, const int64_t fill)
  {
      return svdup_s64_x(pg, fill);
  }
  MCS_FORCE_INLINE SimdType loadFrom(PredicationType& pg, const char* from)
  {
    return svld1(pg, reinterpret_cast<const T*>(from));
  }
  MCS_FORCE_INLINE SimdType loadFromIndex(PredicationType& pg, const char* base, const uint16_t* index)
  {
    SimdType value;
    uint64_t n = stepLength();
    for (uint64_t i = 0; i < n; ++i)
      *((T*)&value + i) = reinterpret_cast<const T*>(base)[index[i]];
    return value;
  }
  MCS_FORCE_INLINE MT cmpEq(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT nullEmptyCmpNe(PredicationType& pg, SimdType& x, T y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT cmpNe(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT cmpGt(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT cmpGe(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT cmpLe(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT cmpLt(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT cmple(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE MT cmpAlwaysFalse(PredicationType& pg, SimdType& x, T& y)
  {
    return svpfalse();
  }
  MCS_FORCE_INLINE MT cmpAlwaysTrue(PredicationType& pg, SimdType& x, T& y)
  {
    return pg;
  }
  MCS_FORCE_INLINE bool testAny(const PredicationType& pg, const PredicationType& op)
  {
    return svptest_any(pg, op);
  }
  MCS_FORCE_INLINE void store(PredicationType& pg, char* dst, SimdType& x)
  {
    svst1(pg, reinterpret_cast<T*>(dst), x);
  }
  MCS_FORCE_INLINE uint64_t countElements(PredicationType& pg)
  {
      return svcntp_b64(pg, pg);
  }
};
#else
// the type is decided by the basic type
using vi1_t =int8x16_t;
using vi2_t =int16x8_t;
using vi4_t =int32x4_t;
using vi8_t =int64x2_t;
using vi1u_t =uint8x16_t;
using vi2u_t =uint16x8_t;
using vi4u_t =uint32x4_t;
using vi8u_t =uint64x2_t;
using vi128f_t = float32x4_t;
using vi128d_t = float64x2_t;
using int128_t = __int128;
using MT = uint16_t;
//the type is used by the  fun like arm__neon__mm__...
using ArmNeonSSEVecType=uint8x16_t;
//wrapper types
struct vi1_wr
{
  int8x16_t v;
};
struct vi2_wr
{
  int16x8_t v;
};
struct vi4_wr
{
  int32x4_t v;
};
struct vi8_wr
{
  int64x2_t v;
};
struct vi16_wr
{
  int128_t v;
};
struct viu1_wr
{
  uint8x16_t v;
};
struct viu2_wr
{
  uint16x8_t v;
};
struct viu4_wr
{
  uint32x4_t v;
};
struct viu8_wr
{
  uint64x2_t v;
};

struct vi128f_wr
{
  float32x4_t v;
};
struct vi128d_wr
{
  float64x2_t v;
};

template<int W>
struct WidthToSVecWrapperType;

template <>
struct WidthToSVecWrapperType<1>
{
    using Vectype=int8x16_t;
    using WrapperType=struct vi1_wr;
};

template <>
struct WidthToSVecWrapperType<2>
{
  using Vectype = int16x8_t;
  using WrapperType=struct vi2_wr;
};

template <>
struct WidthToSVecWrapperType<4>
{
  using Vectype = int32x4_t;
  using WrapperType=struct vi4_wr;
};

template <>
struct WidthToSVecWrapperType<8>
{
  using Vectype = int64x2_t;
  using WrapperType=struct vi8_wr;
};
template <>
struct WidthToSVecWrapperType<16>
{
  using Vectype = int128_t;
  using WrapperType = struct vi16_wr;
};
template <int W>
struct WidthToVecWrapperType;

template <>
struct WidthToVecWrapperType<1>
{
  using Vectype = uint8x16_t;
  using WrapperType = struct viu1_wr;
};

template <>
struct WidthToVecWrapperType<2>
{
  using Vectype = uint16x8_t;
  using WrapperType = struct viu2_wr;
};

template <>
struct WidthToVecWrapperType<4>
{
  using Vectype = uint32x4_t;
  using WrapperType = struct viu4_wr;
};

template <>
struct WidthToVecWrapperType<8>
{
  using Vectype = uint64x2_t;
  using WrapperType = struct viu8_wr;
};

//We get the simd and wrapper type of basic type by TypeToVecWrapperType.
template <typename T, typename ENABLE=void>
struct TypeToVecWrapperType;

template <typename T>
struct TypeToVecWrapperType<T, typename std::enable_if<std::is_same<T,__int128>::value>::type>
 : WidthToSVecWrapperType<sizeof(__int128)>
{
};

template <typename T>
struct TypeToVecWrapperType<T, typename std::enable_if<std::is_unsigned<T>::value>::type> 
    : WidthToVecWrapperType<sizeof(T)>
{
};

template <typename T>
struct TypeToVecWrapperType<T, typename std::enable_if<std::is_signed<T>::value>::type> 
    : WidthToSVecWrapperType<sizeof(T)>
{
};


template <typename T, ENUM_KIND KIND, typename ENABLE = void>
struct IntegralToSIMD;

template <typename T, ENUM_KIND KIND>
struct IntegralToSIMD<T, KIND,
                      typename std::enable_if<KIND == KIND_FLOAT && sizeof(double) == sizeof(T)>::type>
{
  using type = vi128d_wr;
};

template <typename T, ENUM_KIND KIND>
struct IntegralToSIMD<T, KIND,
                      typename std::enable_if<KIND == KIND_FLOAT && sizeof(float) == sizeof(T)>::type>
{
  using type = vi128f_wr;
};

template <typename T, ENUM_KIND KIND>
struct IntegralToSIMD<T, KIND, typename std::enable_if<KIND != KIND_FLOAT>::type>
{
  using type = TypeToVecWrapperType<T>::WrapperType;
};

template <typename T, ENUM_KIND KIND, typename ENABLE = void>
struct StorageToFiltering;

template <typename T, ENUM_KIND KIND>
struct StorageToFiltering<T, KIND,
                          typename std::enable_if<KIND == KIND_FLOAT && sizeof(double) == sizeof(T)>::type>
{
  using type = double;
};

template <typename T, ENUM_KIND KIND>
struct StorageToFiltering<T, KIND,
                          typename std::enable_if<KIND == KIND_FLOAT && sizeof(float) == sizeof(T)>::type>
{
  using type = float;
};

template <typename T, ENUM_KIND KIND>
struct StorageToFiltering<T, KIND, typename std::enable_if<KIND != KIND_FLOAT>::type>
{
  using type = T;
};

// these are the x86 instructions that need to be realized by some arm neon instructions
// the implementations of mm_movemask_epi8 for each type are different because of performance

MCS_FORCE_INLINE MT arm_neon_mm_movemask_epi8_64(ArmNeonSSEVecType input)
{
  return static_cast<MT>(vgetq_lane_u8(input, 0) | ((int)vgetq_lane_u8(input, 8) << 8));
}

MCS_FORCE_INLINE MT arm_neon_mm_movemask_epi8_32(ArmNeonSSEVecType input)
{
  input = vreinterpretq_u8_u64(vshrq_n_u64(vreinterpretq_u64_u8(input), 4));
  return static_cast<MT>(vgetq_lane_u8(input, 3) | ((int)vgetq_lane_u8(input, 11) << 8));
}

MCS_FORCE_INLINE MT arm_neon_mm_movemask_epi8_16(ArmNeonSSEVecType input)
{
  input = vreinterpretq_u8_u16(vshrq_n_u16(vreinterpretq_u16_u8(input), 14));
  input = vreinterpretq_u8_u32(vsraq_n_u32(vreinterpretq_u32_u8(input), vreinterpretq_u32_u8(input), 14));
  input = vreinterpretq_u8_u64(vsraq_n_u64(vreinterpretq_u64_u8(input), vreinterpretq_u64_u8(input), 28));
  return static_cast<MT>(vgetq_lane_u8(input, 0) | ((int)vgetq_lane_u8(input, 8) << 8));
}
MCS_FORCE_INLINE MT arm_neon_mm_movemask_epi8(ArmNeonSSEVecType input)
{
  // Example input (half scale):
  // 0x89 FF 1D C0 00 10 99 33

  // Shift out everything but the sign bits
  // 0x01 01 00 01 00 00 01 00
  uint16x8_t high_bits = vreinterpretq_u16_u8(vshrq_n_u8(input, 7));

  // Merge the even lanes together with vsra. The '??' bytes are garbage.
  // vsri could also be used, but it is slightly slower on aarch64.
  // 0x??03 ??02 ??00 ??01
  uint32x4_t paired16 = vreinterpretq_u32_u16(vsraq_n_u16(high_bits, high_bits, 7));
  // Repeat with wider lanes.
  // 0x??????0B ??????04
  uint64x2_t paired32 = vreinterpretq_u64_u32(vsraq_n_u32(paired16, paired16, 14));
  // 0x??????????????4B
  uint8x16_t paired64 = vreinterpretq_u8_u64(vsraq_n_u64(paired32, paired32, 28));
  // Extract the low 8 bits from each lane and join.
  // 0x4B
  return static_cast<MT>(vgetq_lane_u8(paired64, 0) | ((int)vgetq_lane_u8(paired64, 8) << 8));
}


MCS_FORCE_INLINE MT arm_neon_mm_movemask_pd(ArmNeonSSEVecType a)
{
  uint64x2_t input = vreinterpretq_u64_u8(a);
  uint64x2_t high_bits = vshrq_n_u64(input, 63);
  return static_cast<MT> (vgetq_lane_u64(high_bits, 0) | (vgetq_lane_u64(high_bits, 1) << 1));
}
MCS_FORCE_INLINE MT arm_neon_mm_movemask_ps(ArmNeonSSEVecType a)
{
  uint32x4_t input = vreinterpretq_u32_u8(a);
  static const int32x4_t shift = {0, 1, 2, 3};
  uint32x4_t tmp = vshrq_n_u32(input, 31);
  return static_cast<MT>(vaddvq_u32(vshlq_u32(tmp, shift)));
}

MCS_FORCE_INLINE void arm_neon_mm_maskmoveu_si128(ArmNeonSSEVecType a, ArmNeonSSEVecType mask, char* mem_addr)
{
  int8x16_t shr_mask = vshrq_n_s8(vreinterpretq_s8_u8(mask), 7);
  float32x4_t b = vld1q_f32((float*)mem_addr);
  int8x16_t masked =
      vbslq_s8(vreinterpretq_u8_s8(shr_mask), vreinterpretq_s8_u8(a), vreinterpretq_s8_f32(b));
  vst1q_s8((int8_t*)mem_addr, masked);
}

template <typename VT, typename T, typename ENABLE = void>
class SimdFilterProcessor;

// Dummy class that captures all impossible cases, e.g. integer vector as VT and flot as CHECK_T.we use int32_t to do operations
template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<sizeof(CHECK_T) == 16 ||
                            (std::is_same<VT, vi128f_wr>::value && !std::is_same<CHECK_T, float>::value &&
                             !std::is_same<CHECK_T, double>::value)>::type>
{
  // This is a dummy class that is not currently used.
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = int32_t;
  using SimdWrapperType = vi4_wr;
  using SimdType = int32x4_t;
  using FilterType = T;
  using StorageType = T;
  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_s32(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_s32(reinterpret_cast<const int32_t*>(from));
  }

  MCS_FORCE_INLINE MT cmpDummy(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }
  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpDummy(x, y);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_s32(0);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_s32(reinterpret_cast<int32_t*>(dst), x);
  }
};

template <typename VT, typename T>
class SimdFilterProcessor<
    VT, T,
    typename std::enable_if<std::is_same<VT, vi128d_wr>::value && std::is_same<T, double>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using FilterType = T;
  using NullEmptySimdType = typename WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using SimdWrapperType = simd::vi128d_wr;
  using SimdType = simd::vi128d_t;
  using StorageSimdType = typename WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using StorageType = typename datatypes::WidthToSIntegralType<sizeof(T)>::type;
  using StorageWrapperTypeType =typename WidthToSVecWrapperType<sizeof(T)>::WrapperType;
  using StorageVecProcType = SimdFilterProcessor<StorageWrapperTypeType, StorageType>;
  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    StorageVecProcType nullEmptyProcessor;
    // This spec borrows the expr from u-/int64 based proceesor class.
    return (SimdType)nullEmptyProcessor.loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_f64(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_f64(reinterpret_cast<const T*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vceqq_f64(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcgeq_f64(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcgtq_f64(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcleq_f64(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcltq_f64(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return cmpEq(x,y) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_pd((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    StorageVecProcType nullEmptyProcessor;
    NullEmptySimdType* xAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&x);
    NullEmptySimdType* yAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&y);
    // This spec borrows the expr from u-/int64 based proceesor class.
    return nullEmptyProcessor.cmpNe(*xAsIntVecPtr, *yAsIntVecPtr);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    StorageVecProcType nullEmptyProcessor;

    NullEmptySimdType* xAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&x);
    NullEmptySimdType* yAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&y);
    // This spec borrows the expr from u-/int64 based proceesor class.
    return nullEmptyProcessor.cmpEq(*xAsIntVecPtr, *yAsIntVecPtr);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_f64(0);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_f64(reinterpret_cast<T*>(dst), x);
  }
};

template <typename VT, typename T>
class SimdFilterProcessor<
    VT, T, typename std::enable_if<std::is_same<VT, vi128f_wr>::value && std::is_same<T, float>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using FilterType = T;
  using NullEmptySimdType =typename  WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using SimdWrapperType = vi128f_wr;
  using SimdType = vi128f_t;
  using StorageSimdType = typename WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using StorageType = typename datatypes::WidthToSIntegralType<sizeof(T)>::type;
  using StorageWrapperTypeType =typename WidthToSVecWrapperType<sizeof(T)>::WrapperType;
  using StorageVecProcType = SimdFilterProcessor<StorageWrapperTypeType, StorageType>;
  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    StorageVecProcType nullEmptyProcessor;
    // This spec borrows the expr from u-/int64 based proceesor class.
    return (SimdType)nullEmptyProcessor.loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_f32(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_f32(reinterpret_cast<const T*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vceqq_f32(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcgeq_f32(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcgtq_f32(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcleq_f32(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcltq_f32(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vmvnq_u32(vceqq_f32(x, y)));
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_ps((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    StorageVecProcType nullEmptyProcessor;

    NullEmptySimdType* xAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&x);
    NullEmptySimdType* yAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&y);
    // This spec borrows the expr from u-/int64 based proceesor class.
    return nullEmptyProcessor.cmpNe(*xAsIntVecPtr, *yAsIntVecPtr);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    StorageVecProcType nullEmptyProcessor;

    NullEmptySimdType* xAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&x);
    NullEmptySimdType* yAsIntVecPtr = reinterpret_cast<NullEmptySimdType*>(&y);
    // This spec borrows the expr from u-/int64 based proceesor class.
    return nullEmptyProcessor.cmpEq(*xAsIntVecPtr, *yAsIntVecPtr);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_f32(0);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_f32(reinterpret_cast<T*>(dst), x);
  }
};

template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<std::is_same<VT, vi8_wr>::value && std::is_same<CHECK_T, int64_t>::value &&
                            !std::is_same<CHECK_T, double>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = typename datatypes::WidthToSIntegralType<sizeof(CHECK_T)>::type;
  using SimdWrapperType = typename WidthToSVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType =typename WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_s64(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_s64(reinterpret_cast<const int64_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType) vcgeq_s64(x,y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcgtq_s64(x, y));
  }

  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vceqq_s64(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcleq_s64(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcltq_s64(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return cmpEq(x,y)^0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_s64(0);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_s64(reinterpret_cast<int64_t*>(dst), x);
  }
};

template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<std::is_same<VT, viu8_wr>::value && std::is_same<CHECK_T, uint64_t>::value &&
                            !std::is_same<CHECK_T, double>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = uint64_t;
  using SimdWrapperType = typename WidthToVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType = typename WidthToVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_u64(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_u64(reinterpret_cast<const uint64_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcgeq_u64(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcgtq_u64(x, y));
  }

  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vceqq_u64(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vcleq_u64(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return cmpGt(y, x);
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vceqq_u64(x, y)) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8_64((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_u64(0);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_u64(reinterpret_cast<uint64_t*>(dst), x);
  }
};
template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<std::is_same<VT, vi4_wr>::value && std::is_same<CHECK_T, int32_t>::value &&
                            !std::is_same<CHECK_T, float>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = typename datatypes::WidthToSIntegralType<sizeof(CHECK_T)>::type;
  using SimdWrapperType =typename WidthToSVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType = typename WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_s32(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_s32(reinterpret_cast<const int32_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType) vceqq_s32(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcgeq_s32(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcgtq_s32(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcleq_s32(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcltq_s32(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vceqq_s32(x, y)) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_s32(0);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_s32(reinterpret_cast<int32_t*>(dst), x);
  }
};

template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<std::is_same<VT, viu4_wr>::value && std::is_same<CHECK_T, uint32_t>::value &&
                            !std::is_same<CHECK_T, float>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = uint32_t;
  using SimdWrapperType = typename WidthToVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType = typename WidthToVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_u32(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_u32(reinterpret_cast<const uint32_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vceqq_u32(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcgeq_u32(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcgtq_u32(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vcleq_u32(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return cmpGt(y, x);
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vceqq_u32(x, y)) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8_32((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_u32(0);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_u32(reinterpret_cast<uint32_t*>(dst), x);
  }
};

template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<std::is_same<VT, vi2_wr>::value && std::is_same<CHECK_T, int16_t>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = typename datatypes::WidthToSIntegralType<sizeof(CHECK_T)>::type;
  using SimdWrapperType = typename WidthToSVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType = typename WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_s16(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_s16(reinterpret_cast<const int16_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vceqq_s16(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vcgeq_s16(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vcgtq_s16(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vcleq_s16(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vcltq_s16(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return cmpEq(x,y) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_s16(0);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_s16(reinterpret_cast<int16_t*>(dst), x);
  }
};

template <typename VT, typename CHECK_T>
class SimdFilterProcessor<VT, CHECK_T,
                          typename std::enable_if<std::is_same<VT, viu2_wr>::value &&
                                                  std::is_same<CHECK_T, uint16_t>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T =  uint16_t;
  using SimdWrapperType = typename WidthToVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType = typename WidthToVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_u16(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_u16(reinterpret_cast<const uint16_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vceqq_u16(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vcgeq_u16(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vcgtq_u16(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return cmpGe(y, x);
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vcltq_u16(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vceqq_u16(x, y)) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }

  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8_16((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_u16(0);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_u16(reinterpret_cast<uint16_t*>(dst), x);
  }
};

template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<std::is_same<VT, vi1_wr>::value && std::is_same<CHECK_T, int8_t>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = typename datatypes::WidthToSIntegralType<sizeof(CHECK_T)>::type;
  using SimdWrapperType = typename WidthToSVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType = typename WidthToSVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_s8(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_s8(reinterpret_cast<const int8_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vceqq_s8(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcgeq_s8(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcgtq_s8(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcleq_s8(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcltq_s8(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vceqq_s8(x, y)) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }
  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_s8(0);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_s8(reinterpret_cast<int8_t*>(dst), x);
  }
};

template <typename VT, typename CHECK_T>
class SimdFilterProcessor<
    VT, CHECK_T,
    typename std::enable_if<std::is_same<VT, viu1_wr>::value && std::is_same<CHECK_T, uint8_t>::value>::type>
{
 public:
  constexpr static const uint16_t vecByteSize = 16U;
  constexpr static const uint16_t vecBitSize = 128U;
  using T = uint8_t;
  using SimdWrapperType =typename WidthToVecWrapperType<sizeof(T)>::WrapperType;
  using SimdType = typename WidthToVecWrapperType<sizeof(T)>::Vectype;
  using FilterType = T;
  using StorageType = T;

  constexpr static const uint16_t FilterMaskStep = sizeof(T);
  // Load value
  MCS_FORCE_INLINE SimdType emptyNullLoadValue(const T fill)
  {
    return loadValue(fill);
  }

  MCS_FORCE_INLINE SimdType loadValue(const T fill)
  {
    return vdupq_n_u8(fill);
  }

  // Load from
  MCS_FORCE_INLINE SimdType loadFrom(const char* from)
  {
    return vld1q_u8(reinterpret_cast<const uint8_t*>(from));
  }

  // Compare
  MCS_FORCE_INLINE MT cmpEq(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vceqq_u8(x, y));
  }

  MCS_FORCE_INLINE MT cmpGe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcgeq_u8(x, y));
  }

  MCS_FORCE_INLINE MT cmpGt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcgtq_u8(x, y));
  }

  MCS_FORCE_INLINE MT cmpLe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcleq_u8(x, y));
  }

  MCS_FORCE_INLINE MT cmpLt(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vcltq_u8(x, y));
  }

  MCS_FORCE_INLINE MT cmpNe(SimdType& x, SimdType& y)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vceqq_u8(x, y)) ^ 0xFFFF;
  }

  MCS_FORCE_INLINE MT cmpAlwaysFalse(SimdType& x, SimdType& y)
  {
    return 0;
  }

  MCS_FORCE_INLINE MT cmpAlwaysTrue(SimdType& x, SimdType& y)
  {
    return 0xFFFF;
  }


  // misc
  MCS_FORCE_INLINE MT convertVectorToBitMask(SimdType& vmask)
  {
    return arm_neon_mm_movemask_epi8((ArmNeonSSEVecType)vmask);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpNe(SimdType& x, SimdType& y)
  {
    return cmpNe(x, y);
  }

  MCS_FORCE_INLINE MT nullEmptyCmpEq(SimdType& x, SimdType& y)
  {
    return cmpEq(x, y);
  }

  MCS_FORCE_INLINE SimdType setToZero()
  {
    return vdupq_n_u8(0);
  }

  // store
  MCS_FORCE_INLINE void storeWMask(SimdType& x, SimdType& vmask, char* dst)
  {
    arm_neon_mm_maskmoveu_si128((ArmNeonSSEVecType)x, (ArmNeonSSEVecType)vmask, dst);
  }

  MCS_FORCE_INLINE void store(char* dst, SimdType& x)
  {
    vst1q_u8(reinterpret_cast<uint8_t*>(dst), x);
  }
};
#endif
};      // namespace simd

#endif
