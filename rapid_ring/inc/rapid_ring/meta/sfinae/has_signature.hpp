#ifndef __RAPID_RING_HAS_SIGNATURE_H__
#define __RAPID_RING_HAS_SIGNATURE_H__

#define HAS_SIGNATURE_DEFINE(NAME) \
namespace rapid_ring \
{\
	namespace meta \
	{\
		namespace sfinae \
		{\
			template <typename T, typename RT, typename... Args> \
			class has_signature_ ## NAME \
			{\
				template<typename U, RT (U::*)(Args...) const> struct SFINAE {};\
				template<typename U> static char Test(SFINAE<U, &U::NAME>*);\
				template<typename U> static int Test(...);\
			public:\
				static const bool value = sizeof(Test<T>(0)) == sizeof(char);\
			};\
		}\
	}\
}

namespace rapid_ring
{
	namespace meta
	{
		namespace sfinae
		{
			template<typename T, typename... Args>
			class has_assignment
			{
				template<typename U, U &(U::*)(Args...) const>
				struct SFINAE
				{
				};

				template<typename U>
				static char Test(SFINAE<U, &U::operator=> *);

				template<typename U>
				static int Test(...);

			public:
				static const bool value = sizeof(Test<T>(0)) == sizeof(char);
			public:
			};

			template<typename T>
			using has_copy_assignment = has_assignment<T, const T &>;

			template<typename T>
			using has_move_assignment = has_assignment<T, T &&>;
		}
	}
}

#endif //__RAPID_RING_HAS_SIGNATURE_H__
