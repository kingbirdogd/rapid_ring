#ifndef __RAPID_RING_RING_BUFFER_HPP__
#define __RAPID_RING_RING_BUFFER_HPP__
#include <cstring>
#include <type_traits>
#include <algorithm>
#include <functional>
#include <atomic>
#include <vector>
#include <rapid_ring/meta/smart_copy.hpp>
#include <rapid_ring/meta/meta_cache_line.hpp>
#include <rapid_ring/meta/math/meta_math.hpp>

namespace rapid_ring
{
	template
	<
		template<uint64_t, uint64_t, template<class> class> class write_barrier_tmp,
		typename TReadBarrier,
		typename T,
	 	uint64_t SIZE, 
		uint64_t CACHELINE_SIZE,
		template<class> class allocator_tmp
	>
	class ring_buffer_base
	{
	private:
		using barrier_type = write_barrier_tmp<SIZE, CACHELINE_SIZE, allocator_tmp>;
	public:
		using node = rapid_ring::meta::cacheline::cacheline_packer<T, CACHELINE_SIZE>;
	public:
		const static uint64_t buffer_size = rapid_ring::meta::math::size_to_pow_2<SIZE>::value;
	protected:
		using uint64_packer = rapid_ring::meta::cacheline::cacheline_packer<std::atomic_uint64_t, CACHELINE_SIZE>;
	protected:
		using node_allocator = allocator_tmp<node>;
	protected:
		using buffer_packer = rapid_ring::meta::cacheline::cacheline_packer<node *, CACHELINE_SIZE>;
		using node_allocator_packer = rapid_ring::meta::cacheline::cacheline_packer<node_allocator, CACHELINE_SIZE>;
		using moder = rapid_ring::meta::math::mode_operator<SIZE>;
	protected:
		buffer_packer buffer_;
		node_allocator_packer node_alloc_;
		barrier_type write_barrier_;
		TReadBarrier read_barrier_;
	public:
		class read_reserve
		{
		private:
			using cb = std::function<void()>;
		public:
			node* buff_;
			node* buff2_;
			uint64_t size_first_;
			uint64_t size_;
			cb cb_;
		public:
			void release()
			{
				if (cb_)
				{
					cb_();
					cb_ = std::move(cb());
				}
			}
		public:
			read_reserve():
				buff_(0),
				buff2_(0),
				size_first_(0),
				size_(0),
				cb_()
			{
			}
			~read_reserve()
			{
				release();
			}
			read_reserve(const read_reserve&) = delete;
			read_reserve(read_reserve&& r):
				buff_(r.buff_),
				buff2_(r.buff2_),
				size_first_(r.size_first_),
				size_(r.size_),
				cb_(std::move(r.cb_))
			{
			}
			read_reserve& operator= (const read_reserve&) = delete;
			read_reserve& operator= (read_reserve&& r)
			{
				release();
				buff_ = r.buff_;
				buff2_ = r.buff2_;
				size_first_ = r.size_first_;
				size_ = r.size_;
				cb_ = std::move(r.cb_);
			}
			operator bool() const
			{
				return (0 != size_);
			}
			T& operator[] (uint64_t idx)
			{
				if (idx < size_first_)
					return buff_[idx];
				else
					return buff_[idx - size_first_];
			}
			uint64_t size() const
			{
				return size_;
			}
		};
	public:
		template<typename... TArgs>
		ring_buffer_base(TArgs... args)
				:
				buffer_()
				, node_alloc_()
				, write_barrier_()
				, read_barrier_()
		{
			(node *&) buffer_ = node_alloc_.allocate(buffer_size);
			for (uint64_t i = 0; i < buffer_size; ++i)
				new((node *&) buffer_ + i) node(args...);
		}

		~ring_buffer_base()
		{
			for (uint64_t i = 0; i < buffer_size; ++i)
				((node *&) buffer_ + i)->~node();
			node_alloc_.deallocate(buffer_, buffer_size);
		}

	public:
		void block_enqueue(node *ptr, uint64_t size)
		{
			static_assert(std::is_trivially_copyable<node>::value, "non copytable");
			auto base = write_barrier_.reserve(size);
			auto end = base + size;
			read_barrier_.wait(end - buffer_size);
			auto pos = moder::mode(base);
			auto rest = buffer_size - pos;
			if (rest >= size)
			{
				std::memcpy(buffer_ + pos, ptr, size * sizeof(node));
			}
			else
			{
				std::memcpy(buffer_ + pos, ptr, rest * sizeof(node));
				std::memcpy(buffer_, ptr + rest, (size - rest) * sizeof(node));
			}
			write_barrier_.set(pos, end);
		}

		bool try_block_enqueue(node *ptr, uint64_t size)
		{
			static_assert(std::is_trivially_copyable<node>::value, "non copytable");
			auto base = write_barrier_.try_reserve();
			auto end = base + size;
			if (0 == read_barrier_.try_wait(end - buffer_size))
			{
				return false;
			}
			if (!write_barrier_.set_reserve(base, end))
			{
				return false;
			}
			auto pos = moder::mode(base);
			auto rest = buffer_size - pos;
			if (rest >= size)
			{
				std::memcpy(buffer_ + pos, ptr, size * sizeof(node));
			}
			else
			{
				std::memcpy(buffer_ + pos, ptr, rest * sizeof(node));
				std::memcpy(buffer_, ptr + rest, (size - rest) * sizeof(node));
			}
			write_barrier_.set(pos, end);
			return true;
		}

		template<typename TNodeType>
		void enqueue(TNodeType &&value)
		{
			auto base = write_barrier_.reserve(1);
			auto end = base + 1;
			read_barrier_.wait(end - buffer_size);
			auto pos = moder::mode(base);
			(TNodeType &) buffer_[pos] = std::forward<TNodeType>(value);
			write_barrier_.set(pos, end);
		}

		template<typename TNodeType>
		bool try_enqueue(TNodeType &&value)
		{
			auto base = write_barrier_.try_reserve();
			auto end = base + 1;
			if (0 == read_barrier_.try_wait(end - buffer_size))
			{
				return false;
			}
			if (!write_barrier_.set_reserve(base, end))
			{
				return false;
			}
			auto pos = moder::mode(base);
			(TNodeType &) buffer_[pos] = std::forward<TNodeType>(value);
			write_barrier_.set(pos, end);
			return true;
		}

		void reset()
		{
			read_barrier_.reset();
			write_barrier_.reset();
		}

		uint64_t size()
		{
			return write_barrier_.ready() - read_barrier_.ready();
		}
	};
}
#endif //__RAPID_RING_RING_BUFFER_HPP__
