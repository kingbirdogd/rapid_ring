#ifndef RAPID_RING_INC_RAPID_RING_RING_BUFFER_OBJECT_POLL_HPP_
#define RAPID_RING_INC_RAPID_RING_RING_BUFFER_OBJECT_POLL_HPP_

#include <rapid_ring/ring_buffer_queue.hpp>

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
	class ring_buffer_object_pool
	{
	private:
		using queue = ring_buffer_queue<write_barrier_tmp,
				read_barrier_tmp,
				T*,
				SIZE,
				CACHELINE_SIZE,
				allocator_tmp>;
	private:
		queue q_;
		unsigned char buffer_[queue::buffer_size * sizeof(T)];
	public:
		template <typename... TArgs>
		ring_buffer_object_pool(TArgs&& ...args):
			q_(),
			buffer_{}
		{
			q_.warm_up();
			std::memset(buffer_, 0, queue::buffer_size * sizeof(T));
			auto ptr = static_cast<T*>(static_cast<void*>(buffer_));
			for (uint64_t i = 0; i < queue::buffer_size; ++i)
			{
				new (ptr + i) T(args...);
				q_.enqueue(ptr + i);
			}
		}
		~ring_buffer_object_pool()
		{
			auto ptr = static_cast<T*>(static_cast<void*>(buffer_));
			for (uint64_t i = 0; i < queue::buffer_size; ++i)
			{
				(ptr + i)->~T();
			}
		}

		T* get_obj()
		{
			T* ptr = nullptr;
			q_.dequeue(ptr);
			return ptr;
		}

		void release_obj(T* ptr)
		{
			q_.enqueue(ptr);
		}

		T* try_get_obj()
		{
			T* ptr = nullptr;
			if (q_.try_dequeue(ptr))
				return ptr;
			else
				return nullptr;
		}

		bool try_release_obj(T* ptr)
		{
			return q_.try_enqueue(ptr);
		}
	};

	template
	<
		typename T,
		uint64_t SIZE,
		uint64_t CACHELINE_SIZE = 64,
		template<class> class allocator_tmp = std::allocator
	>
	using spsc_ring_buffer_object_pool  =
	ring_buffer_object_pool
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
	using mpsc_ring_buffer_object_pool  =
	ring_buffer_object_pool
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
	using spmc_ring_buffer_object_pool  =
	ring_buffer_object_pool
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
	using mpmc_ring_buffer_object_pool  =
	ring_buffer_object_pool
	<
		rapid_ring::barrier::ring_buffer_barrier_mt,
		rapid_ring::barrier::ring_buffer_barrier_mt,
		T,
		SIZE,
		CACHELINE_SIZE,
		allocator_tmp
	>;
}



#endif /* RAPID_RING_INC_RAPID_RING_RING_BUFFER_OBJECT_POLL_HPP_ */
