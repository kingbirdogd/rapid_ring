#ifndef __RAPID_RING_META_MATH_HPP__
#define __RAPID_RING_META_MATH_HPP__

#include <cstdint>

namespace rapid_ring
{
	namespace meta
	{
		namespace math
		{
			template<uint64_t X, uint64_t N>
			struct uint64_t_pow
			{
				const static uint64_t value = uint64_t_pow<X, N - 1>::value * X;
			};
			template<uint64_t X>
			struct uint64_t_pow<X, 0>
			{
				const static uint64_t value = 1;
			};
			template<uint64_t N>
			struct pow_mod_2_base
			{
				const static uint64_t value = uint64_t_pow<2, N>::value - 1;
			};
			template<uint64_t SIZE, uint64_t N, bool IS_LARGE_TAHN>
			struct pow_2_size_helper;
			template<uint64_t SIZE, uint64_t N>
			struct pow_2_size_helper<SIZE, N, false>
			{
				const static uint64_t value = 
				pow_2_size_helper
				<
					SIZE,
					N + 1, uint64_t_pow<2, N + 1>::value >= SIZE
				>::value;
			};
			template<uint64_t SIZE, uint64_t N>
			struct pow_2_size_helper<SIZE, N, true>
			{
				const static uint64_t value = N;
			};
			template<uint64_t SIZE>
			struct pow_2_size
			{
				const static uint64_t value = pow_2_size_helper<SIZE, 0, uint64_t_pow<2, 0>::value >= SIZE>::value;
			};
			template<uint64_t SIZE>
			struct size_to_pow_2
			{
				const static uint64_t value = uint64_t_pow<2, pow_2_size<SIZE>::value>::value;
			};
			template<uint64_t SIZE>
			struct size_to_pow_2_mode_base
			{
				const static uint64_t value = size_to_pow_2<SIZE>::value - 1;
			};

			template<uint64_t SIZE>
			class mode_operator
			{
			private:
				const static uint64_t mode_base = meta::math::size_to_pow_2_mode_base<SIZE>::value;
			public:
				inline static uint64_t mode(uint64_t x)
				{
					return x & mode_base;
				}
			};
		}
	}
}

#endif //__RAPID_RING_META_MATH_HPP__
