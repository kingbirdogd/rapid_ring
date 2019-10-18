#ifndef __RAPID_RING_SMART_COPY_HPP__
#define __RAPID_RING_SMART_COPY_HPP__

#include <cstring>
#include <rapid_ring/meta/sfinae/has_signature.hpp>

namespace rapid_ring
{
	namespace meta
	{
		template<typename T>
		class smart_copy
		{
		private:
			template<typename U, bool has_move>
			struct smart_copy_helper;

			template<typename U>
			struct smart_copy_helper<U, true>
			{
				static void copy(U &x, U &y)
				{
					x = std::move(y);
				}
			};

			template<typename U>
			struct smart_copy_helper<U, false>
			{
				static void copy(U &x, U &y)
				{
					x = y;
				}
			};

		public:
			static void copy(T &x, T &y)
			{
				smart_copy_helper<T, rapid_ring::meta::sfinae::has_move_assignment<T>::value>::copy(x, y);
			}
		};
	}
}

#endif //__RAPID_RING_SMART_COPY_HPP__
