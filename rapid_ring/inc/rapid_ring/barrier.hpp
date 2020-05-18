#ifndef __RAPID_RING_BARRIER_HPP__
#define __RAPID_RING_BARRIER_HPP__

#include <limits>
#include <rapid_ring/meta/meta_cache_line.hpp>
#include <rapid_ring/meta/math/meta_math.hpp>

namespace rapid_ring
{
	namespace barrier
	{
		class barrier_interface
		{
		public:
			virtual ~barrier_interface()
			{
			}

			virtual uint64_t wait(uint64_t) = 0;

			virtual uint64_t try_wait(uint64_t) = 0;

			virtual void clear() = 0;

			virtual void add(barrier_interface &) = 0;
		};

		template<uint64_t CACHELINE_SIZE>
		using barrier_interface_packer = rapid_ring::meta::cacheline::cacheline_packer<barrier_interface, CACHELINE_SIZE>;
		using barrier_interface_ptr = barrier_interface *;
		template<uint64_t CACHELINE_SIZE>
		using barrier_interface_ptr_packer = rapid_ring::meta::cacheline::cacheline_packer<barrier_interface_ptr, CACHELINE_SIZE>;
		template
		<
			uint64_t CACHELINE_SIZE,
			template<class> class allocator_tmp
		>
		using barrier_interface_vec = std::vector<barrier_interface_ptr_packer<CACHELINE_SIZE>, allocator_tmp<barrier_interface_ptr_packer<CACHELINE_SIZE>>>;
		template
		<
			uint64_t CACHELINE_SIZE, 
			template<class> class allocator_tmp
		>
		using barrier_interface_vec_packer = rapid_ring::meta::cacheline::cacheline_packer<barrier_interface_vec<CACHELINE_SIZE, allocator_tmp>, CACHELINE_SIZE>;

		template
		<
			uint64_t CACHELINE_SIZE,
			template<class> class allocator_tmp
		>
		class barriers
		{
		private:
			using vec_type = barrier_interface_vec_packer<CACHELINE_SIZE, allocator_tmp>;
		private:
			vec_type vec_;
		public:
			void add(barrier_interface &b)
			{
				barrier_interface_ptr_packer<CACHELINE_SIZE> item;
				(barrier_interface_ptr &) item = &b;
				vec_.push_back(item);
			}

			void clear()
			{
				vec_.resize(0);
			}

			uint64_t wait(uint64_t min)
			{
				auto rt = std::numeric_limits<uint64_t>::max();
				for (uint64_t i = 0; i < vec_.size(); ++i)
				{
					auto v = ((barrier_interface_ptr &) (vec_[i]))->wait(min);
					if (v < rt)
					{
						rt = v;
					}
				}
				return rt;
			}

			uint64_t try_wait(uint64_t min)
			{
				auto rt = std::numeric_limits<uint64_t>::max();
				for (uint64_t i = 0; i < vec_.size(); ++i)
				{
					auto v = ((barrier_interface_ptr &) (vec_[i]))->try_wait(min);
					if (v < rt)
					{
						rt = v;
					}
				}
				return rt;
			}


		};

		template
		<
			uint64_t SIZE, 
			uint64_t CACHELINE_SIZE, 
			template<class> class allocator_tmp
		>
		class ring_buffer_barrier_mt
		{
		private:
			const static uint64_t buffer_size = rapid_ring::meta::math::size_to_pow_2<SIZE>::value;
		private:
			using moder = rapid_ring::meta::math::mode_operator<SIZE>;
			using uint64_cache_packer = rapid_ring::meta::cacheline::cacheline_packer<uint64_t, CACHELINE_SIZE>;
			using uint64_packer = rapid_ring::meta::cacheline::cacheline_packer<std::atomic_uint64_t, CACHELINE_SIZE>;
			using barrier_allocator = allocator_tmp<uint64_packer>;
			using barrier_packer = rapid_ring::meta::cacheline::cacheline_packer<uint64_packer *, CACHELINE_SIZE>;
			using barrier_allocator_packer = rapid_ring::meta::cacheline::cacheline_packer<barrier_allocator, CACHELINE_SIZE>;
		private:
			uint64_cache_packer ready_cache_;
			uint64_packer reserve_;
			uint64_packer ready_;
			barrier_packer barrier_;
			barrier_allocator_packer barrier_alloc_;
		public:
			void reset()
			{
				(uint64_t &) ready_cache_ = buffer_size;
				reserve_.store(buffer_size);
				ready_.store(buffer_size);
				(uint64_packer *&) barrier_ = barrier_alloc_.allocate(buffer_size);
				std::memset(barrier_, 0, sizeof(barrier_packer) * buffer_size);
			}

			ring_buffer_barrier_mt()
					:
					ready_cache_()
					, reserve_()
					, ready_()
					, barrier_()
					, barrier_alloc_()
			{
				reset();
			}

			~ring_buffer_barrier_mt()
			{
				barrier_alloc_.deallocate(barrier_, buffer_size);
			}

			uint64_t reserve(uint64_t size)
			{
				return reserve_.fetch_add(size, std::memory_order_relaxed);
			}

			uint64_t try_reserve()
			{
				return reserve_.load(std::memory_order_relaxed);
			}

			bool set_reserve(uint64_t base, uint64_t end)
			{
				return reserve_.compare_exchange_weak(
						base, end, std::memory_order_release, std::memory_order_relaxed
													 );
			}

			uint64_t wait(uint64_t min)
			{
				if (ready_cache_ > min)
				{
					return ready_cache_;
				}
				(uint64_t &) ready_cache_ = ready_.load(std::memory_order_relaxed);
				uint64_t pre_ready = ready_cache_;
				while (ready_cache_ < min)
				{
					auto next_ready = barrier_[moder::mode(pre_ready)].load(std::memory_order_relaxed);
					if (next_ready <= pre_ready)
					{
						if (ready_.compare_exchange_weak(
								ready_cache_,
								pre_ready, std::memory_order_release, std::memory_order_relaxed
														))
						{
							(uint64_t &) ready_cache_ = pre_ready;
						}
						else
						{
							(uint64_t &) ready_cache_ = ready_.load(std::memory_order_relaxed);
							pre_ready = ready_cache_;
						}
					}
					else
					{
						pre_ready = next_ready;
					}
				}
				return ready_cache_;
			}

			uint64_t try_wait(uint64_t min)
			{
				if (ready_cache_ > min)
				{
					return ready_cache_;
				}
				(uint64_t &) ready_cache_ = ready_.load(std::memory_order_relaxed);
				uint64_t pre_ready = ready_cache_;
				while (ready_cache_ < min)
				{
					auto next_ready = barrier_[moder::mode(pre_ready)].load(std::memory_order_relaxed);
					if (next_ready <= pre_ready)
					{
						if (ready_.compare_exchange_weak(
								ready_cache_,
								pre_ready, std::memory_order_release, std::memory_order_relaxed
														))
						{
							(uint64_t &) ready_cache_ = pre_ready;
							if (ready_cache_ < min)
							{
								return 0;
							}
						}
						else
						{
							(uint64_t &) ready_cache_ = ready_.load(std::memory_order_relaxed);
							pre_ready = ready_cache_;
						}
					}
					else
					{
						pre_ready = next_ready;
					}
				}
				return ready_cache_;
			}

			void set(uint64_t pos, uint64_t end)
			{
				barrier_[pos].store(end, std::memory_order_relaxed);
			}

			uint64_t ready()
			{
				return ready_.load(std::memory_order_relaxed);
			}
		};

		template
		<
			uint64_t SIZE, 
			uint64_t CACHELINE_SIZE, 
			template<class> class allocator_tmp
		>
		class ring_buffer_barrier_st
		{
		private:
			const static uint64_t buffer_size = rapid_ring::meta::math::size_to_pow_2<SIZE>::value;
		private:
			using uint64_cache_packer = rapid_ring::meta::cacheline::cacheline_packer<uint64_t, CACHELINE_SIZE>;
			using uint64_packer = rapid_ring::meta::cacheline::cacheline_packer<std::atomic_uint64_t, CACHELINE_SIZE>;
		private:
			uint64_cache_packer reserve_;
			uint64_cache_packer ready_cache_;
			uint64_packer ready_;
		public:
			void reset()
			{
				(uint64_t &) reserve_ = buffer_size;
				(uint64_t &) ready_cache_ = buffer_size;
				ready_.store(buffer_size, std::memory_order_relaxed);
			}

			ring_buffer_barrier_st()
					:
					reserve_()
					, ready_cache_()
					, ready_()
			{
				reset();
			}

			~ring_buffer_barrier_st()
			{
			}

			uint64_t reserve(uint64_t size)
			{
				auto rt = reserve_;
				reserve_ += size;
				return rt;
			}

			uint64_t try_reserve()
			{
				return reserve_;
			}

			bool set_reserve(uint64_t, uint64_t end)
			{
				(uint64_t &) reserve_ = end;
				return true;
			}

			uint64_t wait(uint64_t min)
			{
				while (ready_cache_ < min)
					(uint64_t &) ready_cache_ = ready_.load(std::memory_order_relaxed);
				return (uint64_t &) ready_cache_;
			}

			uint64_t try_wait(uint64_t min)
			{
				if (ready_cache_ < min)
				{
					(uint64_t &) ready_cache_ = ready_.load(std::memory_order_relaxed);
				}
				return (ready_cache_ < min) ? 0 : (uint64_t &) ready_cache_;
			}

			void set(uint64_t, uint64_t end)
			{
				ready_.store(end, std::memory_order_relaxed);
			}

			uint64_t ready()
			{
				return ready_.load(std::memory_order_relaxed);
			}
		};
	}
}

#endif //__RAPID_RING_BARRIER_HPP__
