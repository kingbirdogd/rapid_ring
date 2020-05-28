#ifndef __RAPID_RING_RING_BUFFER_QUEUE_HPP__
#define __RAPID_RING_RING_BUFFER_QUEUE_HPP__
#include <rapid_ring/ring_buffer.hpp>
#include <rapid_ring/barrier.hpp>

namespace rapid_ring
{
	template
	<
		template<uint64_t, uint64_t, template<class> class> class write_barrier_tmp,
		template<uint64_t, uint64_t, template<class> class> class read_barrier_tmp, 
		typename T,
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE,
		template<class> class allocator_tmp
	>
	class ring_buffer_queue
			:
			public ring_buffer_base
			<
				write_barrier_tmp,
				read_barrier_tmp<SIZE, CACHELINE_SIZE, allocator_tmp>,
				T,
				SIZE,
				CACHELINE_SIZE,
				allocator_tmp
			>
	{
		using base = 
		ring_buffer_base
		<
			write_barrier_tmp,
			read_barrier_tmp<SIZE, CACHELINE_SIZE, allocator_tmp>,
			T,
			SIZE,
			CACHELINE_SIZE,
			allocator_tmp
		>;
		using reader = typename base::read_reserve;
	public:
		ring_buffer_queue()
				:
				base()
		{
		}

		~ring_buffer_queue()
		{
		}

	public:
		void block_dequeue(typename base::node *ptr, uint64_t size)
		{
			static_assert(std::is_trivially_copyable<typename base::node>::value, "non copytable");
			auto base = base::read_barrier_.reserve(size);
			auto end = base + size;
			base::write_barrier_.wait(end);
			auto pos = base::moder::mode(base);
			auto rest = base::buffer_size - pos;
			if (rest >= size)
			{
				std::memcpy(ptr, base::buffer_ + pos, size * sizeof(typename base::node));
			}
			else
			{
				std::memcpy(ptr, base::buffer_ + pos, rest * sizeof(typename base::node));
				std::memcpy(ptr + rest, base::buffer_, (size - rest) * sizeof(typename base::node));
			}
			base::read_barrier_.set(pos, end);
		}

		bool try_block_dequeue(typename base::node *ptr, uint64_t size)
		{
			static_assert(std::is_trivially_copyable<typename base::node>::value, "non copytable");
			auto base = base::read_barrier_.try_reserve();
			auto end = base + size;
			if (0 == base::write_barrier_.try_wait(end))
			{
				return false;
			}
			if (!base::read_barrier_.set_reserve(base, end))
			{
				return false;
			}
			auto pos = base::moder::mode(base);
			auto rest = base::buffer_size - pos;
			if (rest >= size)
			{
				std::memcpy(ptr, base::buffer_ + pos, size * sizeof(typename base::node));
			}
			else
			{
				std::memcpy(ptr, base::buffer_ + pos, rest * sizeof(typename base::node));
				std::memcpy(ptr + rest, base::buffer_, (size - rest) * sizeof(typename base::node));
			}
			base::read_barrier_.set(pos, end);
			return true;
		}

		void dequeue(T &value)
		{
			auto base = base::read_barrier_.reserve(1);
			auto end = base + 1;
			base::write_barrier_.wait(end);
			auto pos = base::moder::mode(base);
			rapid_ring::meta::smart_copy<T>::copy(value, base::buffer_[pos]);
			base::read_barrier_.set(pos, end);
		}

		bool try_dequeue(T &value)
		{
			auto base = base::read_barrier_.try_reserve();
			auto end = base + 1;
			if (0 == base::write_barrier_.try_wait(end))
			{
				return false;
			}
			if (!base::read_barrier_.set_reserve(base, end))
			{
				return false;
			}
			auto pos = base::moder::mode(base);
			rapid_ring::meta::smart_copy<T>::copy(value, base::buffer_[pos]);
			base::read_barrier_.set(pos, end);
			return true;
		}

		reader try_block_dequeue(uint64_t size)
		{
			reader r;
			static_assert(std::is_trivially_copyable<typename base::node>::value, "non copytable");
			auto base = base::read_barrier_.try_reserve();
			auto end = base + size;
			if (0 == base::write_barrier_.try_wait(end))
			{
				return r;
			}
			if (!base::read_barrier_.set_reserve(base, end))
			{
				return r;
			}
			auto pos = base::moder::mode(base);
			auto rest = base::buffer_size - pos;
			if (rest >= size)
			{
				r.buff_ = base::buffer_ + pos;
				r.size_first_ = size;
				r.size_ = size;
			}
			else
			{
				r.buff_ = base::buffer_ + pos;
				r.size_first_ = rest;
				r.buff2_ = base::buffer_;
				r.size_ = size;
			}
			r.cb_ = std::move([this, pos, end]()
			{
				base::read_barrier_.set(pos, end);
			});
			return r;
		}

		template <typename ...TArgs>
		void warm_up(TArgs&& ...args)
		{
			T obj(args...);
			for (uint64_t i = 0; i < base::buffer_size; ++i)
			{
				base::enqueue(obj);
				dequeue(obj);
			}
		}
	};

	template
	<
		typename T, 
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE = 64, 
		template<class> class allocator_tmp = std::allocator
	>
	using spsc_ring_buffer_queue  = 
	ring_buffer_queue
	<
		rapid_ring::barrier::ring_buffer_barrier_st,
		rapid_ring::barrier::ring_buffer_barrier_st,
		T,
		SIZE,
		CACHELINE_SIZE,
		allocator_tmp
	>;

	template
	<
		typename T, 
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE = 64, 
		template<class> class allocator_tmp = std::allocator
	>
	using mpsc_ring_buffer_queue  = 
	ring_buffer_queue
	<
		rapid_ring::barrier::ring_buffer_barrier_mt,
		rapid_ring::barrier::ring_buffer_barrier_st,
		T,
		SIZE,
		CACHELINE_SIZE,
		allocator_tmp
	>;

	template
	<
		typename T, 
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE = 64, 
		template<class> class allocator_tmp = std::allocator
	>
	using spmc_ring_buffer_queue  = 
	ring_buffer_queue
	<
		rapid_ring::barrier::ring_buffer_barrier_st,
		rapid_ring::barrier::ring_buffer_barrier_mt,
		T,
		SIZE,
		CACHELINE_SIZE,
		allocator_tmp
	>;

	template
	<
		typename T,
		uint64_t SIZE,
		uint64_t CACHELINE_SIZE = 64, 
		template<class>	class allocator_tmp = std::allocator
	>
	using mpmc_ring_buffer_queue  = 
	ring_buffer_queue
	<
		rapid_ring::barrier::ring_buffer_barrier_mt,
		rapid_ring::barrier::ring_buffer_barrier_mt,
		T,
		SIZE,
		CACHELINE_SIZE,
		allocator_tmp
	>;
}
#endif //__RAPID_RING_RING_BUFFER_QUEUE_HPP__
