#ifndef __RAPID_RING_META_CACHE_LINE_HPP__
#define __RAPID_RING_META_CACHE_LINE_HPP__

#include <cstdint>

namespace rapid_ring
{
	namespace meta
	{
		namespace cacheline
		{
			template<uint64_t CACHELINE_SIZE, uint64_t CURR_SIZE>
			class cacheline_filler
			{
			private:
				uint8_t filler[CACHELINE_SIZE - CURR_SIZE % CACHELINE_SIZE];
			};

			template<typename TPachType, uint64_t CACHELINE_SIZE, bool IS_MATCH_CACHELINE>
			class cacheline_class_packer_helper;

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			class cacheline_class_packer_helper<TPachType, CACHELINE_SIZE, false>
					:
					public TPachType
			{
			public:
				template<typename... TArgs>
				cacheline_class_packer_helper(TArgs... args)
						:
						TPachType(args...)
				{
				}

			private:
				cacheline_filler<CACHELINE_SIZE, sizeof(TPachType)> filler;
			};

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			class cacheline_class_packer_helper<TPachType, CACHELINE_SIZE, true>
					:
					public TPachType
			{
			public:
				template<typename... TArgs>
				cacheline_class_packer_helper(TArgs... args)
						:
						TPachType(args...)
				{
				}
			};

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			using cacheline_class_packer = 
			cacheline_class_packer_helper
			<
				TPachType, CACHELINE_SIZE,
				0 == sizeof(TPachType) % CACHELINE_SIZE
			>;

			template<typename TPachType, uint64_t CACHELINE_SIZE, bool IS_MATCH_CACHELINE>
			class cacheline_non_class_packer_helper;

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			class cacheline_non_class_packer_helper<TPachType, CACHELINE_SIZE, false>
			{
			private:
				TPachType value;
				cacheline_filler<CACHELINE_SIZE, sizeof(TPachType)> filler;
			public:
				operator TPachType &()
				{
					return value;
				}
			};

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			class cacheline_non_class_packer_helper<TPachType, CACHELINE_SIZE, true>
			{
			private:
				TPachType value;
			public:
				operator TPachType &()
				{
					return value;
				}
			};

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			using cacheline_non_class_packer = 
			cacheline_non_class_packer_helper
			<
				TPachType, CACHELINE_SIZE,
				0 == sizeof(TPachType) % CACHELINE_SIZE
			>;

			template<typename TPachType, uint64_t CACHELINE_SIZE, bool IS_CLASS>
			class cacheline_packer_helper;

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			class cacheline_packer_helper<TPachType, CACHELINE_SIZE, false>
					:
					public cacheline_non_class_packer<TPachType, CACHELINE_SIZE>
			{
			};

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			class cacheline_packer_helper<TPachType, CACHELINE_SIZE, true>
					:
					public cacheline_class_packer<TPachType, CACHELINE_SIZE>
			{
			};

			template<typename TPachType, uint64_t CACHELINE_SIZE>
			using cacheline_packer = cacheline_packer_helper<TPachType, CACHELINE_SIZE, std::is_class<TPachType>::value>;
		}
	}
}

#endif //__RAPID_RING_META_CACHE_LINE_HPP__
