#ifndef __RAPID_RING_RING_BUFFER_DISRUPTOR_HPP__
#define __RAPID_RING_RING_BUFFER_DISRUPTOR_HPP__
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <rapid_ring/ring_buffer.hpp>
#include <rapid_ring/barrier.hpp>

namespace rapid_ring
{
	template
	<
		template<uint64_t, uint64_t, template<class> class> class write_barrier_tmp, 
		typename T,
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE,
		template<class> class allocator_tmp
	>
	class ring_buffer_disruptor
			:
			public ring_buffer_base
			<
				write_barrier_tmp,
				rapid_ring::barrier::barriers<CACHELINE_SIZE, allocator_tmp>,
				T,
				SIZE,
				CACHELINE_SIZE,
				allocator_tmp
			>,
			public rapid_ring::barrier::barrier_interface_packer<CACHELINE_SIZE>
	{
		using barriers_type = rapid_ring::barrier::barriers<CACHELINE_SIZE, allocator_tmp>;
		using base = ring_buffer_base
		<
			write_barrier_tmp,
			barriers_type,
			T,
			SIZE,
			CACHELINE_SIZE,
			allocator_tmp
		>;
		using reader = typename base::read_reserve;
	public:
		class link_not_found_exception
				:
				public std::exception
		{
		};

		class ring_link_to_other_exception
				:
				public std::exception
		{
		};

		class duplicate_link_exception
				:
				public std::exception
		{
		};

		class ring_link_exception
				:
				public std::exception
		{
		};

		class no_comsumer_link_exception
				:
				public std::exception
		{
		};

	private:
		class configure
		{
		private:
			using ptr = rapid_ring::barrier::barrier_interface *;
			using ref = rapid_ring::barrier::barrier_interface &;
			using ptr_set = std::unordered_set<ptr>;
			using max_step_map = std::map <uint64_t, ptr_set, std::greater<uint64_t>>;

			struct grap_node
			{
				uint64_t max_step;
				ptr_set link_set;

				grap_node()
					:
					max_step(0),
					link_set()
				{
				}
			};

			using grap_map = std::unordered_map<ptr, grap_node>;
		private:
			ring_buffer_disruptor &root_;
			grap_map grap_;
			max_step_map max_step_;
			bool already_commit_;

			configure() = delete;

		private:
			std::vector<ptr> toposort()
			{
				std::vector<ptr> rt;
				std::unordered_set<ptr> records;
				records.insert(&root_);
				auto grap = grap_;
				while (1 != grap.size())
				{
					for (auto it = grap.begin(); it != grap.end();)
					{
						for (auto it_link = it->second.link_set.begin(); it_link != it->second.link_set.end();)
						{
							auto dep_ptr = *it_link;
							if (records.end () != records.find(dep_ptr))
							{
								it_link = it->second.link_set.erase(it_link);
							}
							else
							{
								++it_link;
							}
						}
						if (it->second.link_set.empty() && it->first != &root_)
						{
							records.insert(it->first);
							rt.push_back(it->first);
							it = grap.erase(it);
						}
						else
						{
							++it;
						}
					}
				}
				return rt;
			}
		private:
			void check_ring(ptr x, ptr y)
			{
				if (x == y)
				{
					throw ring_link_exception();
				}
				auto &node = grap_[y];
				for (auto next : node.link_set)
					check_ring(x, next);
			}

		public:
			configure(ring_buffer_disruptor &r_)
					:
					root_(r_),
					grap_(),
					max_step_(),
					already_commit_(false)
			{
			}

			~configure()
			{
				commit();
			}

			void x_depends_y(ref x, ref y)
			{
				if (&x == &root_)
				{
					throw ring_link_to_other_exception();
				}
				else if (&y == &root_)
				{
					auto &node = grap_[&x];
					if (node.link_set
							.end() != node.link_set
										  .find(&y))
					{
						throw duplicate_link_exception();
					}
					node.link_set
						.insert(&y);
					if (0 == node.max_step)
					{
						node.max_step = 1;
						max_step_[node.max_step].insert(&x);
					}
				}
				else
				{
					if (grap_.end() == grap_.find(&y))
					{
						throw link_not_found_exception();
					}
					auto it_x = grap_.find(&x);
					if (grap_.end() != it_x)
					{
						if (it_x->second
								.link_set
								.end() != it_x->second
											  .link_set
											  .find(&y))
						{
							throw duplicate_link_exception();
						}

					}
					auto &node = grap_[&x];
					check_ring(&x, &y);
					node.link_set
						.insert(&y);
					auto &y_node = grap_[&y];
					auto step = y_node.max_step + 1;
					if (step > node.max_step)
					{
						node.max_step = step;
						max_step_[node.max_step].insert(&x);
					}
				}
				x.add(y);
			}

			void commit()
			{
				if (already_commit_)
				{
					return;
				}
				if (0 == max_step_.size())
				{
					throw no_comsumer_link_exception();
				}
				auto max_it = max_step_.begin();
				for (auto ptr : max_it->second)
					root_.base::read_barrier_
						 .add(*ptr);
				already_commit_ = true;
			}

			void clear()
			{
				if (already_commit_)
				{
					root_.clear();
					for (auto &it : grap_)
						it->second
						  .clear();
					grap_.clear();
					max_step_.clear();
					already_commit_ = false;
				}
			}

			template <typename ...TArgs>
			void warm_up(TArgs&& ...args)
			{
				if (!already_commit_)
					return;
				auto clients = toposort();
				T obj(args...);
				for (uint64_t i = 0; i < root_.base::buffer_size; ++i)
				{
					root_.enqueue(obj);
					for (auto& cli : clients)
					{
						try
						{
							auto ptr = dynamic_cast<comsumer_st*>(cli);
							ptr->dequeue(obj);
						}
						catch(...)
						{
							auto ptr = dynamic_cast<comsumer_mt*>(cli);
							ptr->dequeue(obj);
						}
					}
				}
			}
		};

		template<template<uint64_t, uint64_t, template<class> class> class read_barrier_tmp>
		class comsumer
			:
			public rapid_ring::barrier::barrier_interface_packer<CACHELINE_SIZE>
		{
		private:
			using ring_ptr_packer = rapid_ring::meta::cacheline::cacheline_packer<ring_buffer_disruptor *, CACHELINE_SIZE>;
			using read_barrier_type = read_barrier_tmp<SIZE, CACHELINE_SIZE, allocator_tmp>;
		protected:
			ring_ptr_packer ring_;
			barriers_type upper_barriers_;
			read_barrier_type read_barrier_;
		public:
			inline comsumer(ring_buffer_disruptor &ring)
					:
					ring_(),
					upper_barriers_(),
					read_barrier_()
			{
				(ring_buffer_disruptor *&) ring_ = &ring;
			}

			virtual ~comsumer()
			{
			}

			virtual void add(rapid_ring::barrier::barrier_interface &b) override
			{
				upper_barriers_.add(b);
			}

			virtual void clear() override
			{
				upper_barriers_.clear();
			}

			virtual uint64_t wait(uint64_t min) override
			{
				return read_barrier_.wait(min);
			}

			virtual uint64_t try_wait(uint64_t min) override
			{
				return read_barrier_.try_wait(min);
			}

			void block_dequeue(typename base::node *ptr, uint64_t size)
			{
				static_assert(std::is_trivially_copyable<typename base::node>::value, "non copytable");
				auto base = read_barrier_.reserve(size);
				auto end = base + size;
				upper_barriers_.wait(end);
				auto pos = base::moder::mode(base);
				auto rest = base::buffer_size - pos;
				if (rest >= size)
				{
					std::memcpy
					(
						ptr, 
						((ring_buffer_disruptor *&) (ring_))->base::buffer_ + pos,
						size * sizeof(typename base::node)
					);
				}
				else
				{
					std::memcpy
					(
						ptr, 
						((ring_buffer_disruptor *&) (ring_))->base::buffer_ + pos,
						rest * sizeof(typename base::node)
					);
					std::memcpy
					(
						ptr + rest,
						((ring_buffer_disruptor *&) (ring_))->base::buffer_,
						(size - rest) * sizeof(typename base::node)
					);
				}
				read_barrier_.set(pos, end);
			}

			bool try_block_dequeue(typename base::node *ptr, uint64_t size)
			{
				static_assert(std::is_trivially_copyable<typename base::node>::value, "non copytable");
				auto base = read_barrier_.try_reserve();
				auto end = base + size;
				if (0 == upper_barriers_.try_wait(end))
				{
					return false;
				}
				if (!read_barrier_.set_reserve(base, end))
				{
					return false;
				}
				auto pos = base::moder::mode(base);
				auto rest = base::buffer_size - pos;
				if (rest >= size)
				{
					std::memcpy
					(
						ptr, 
						((ring_buffer_disruptor *&) (ring_))->base::buffer_ + pos,
						size * sizeof(typename base::node)
					);
				}
				else
				{
					std::memcpy
					(
						ptr,
						((ring_buffer_disruptor *&) (ring_))->base::buffer_ + pos,
						rest * sizeof(typename base::node)
					);
					std::memcpy
					(
						ptr + rest,
						((ring_buffer_disruptor *&) (ring_))->base::buffer_,
						(size - rest) * sizeof(typename base::node)
					);
				}
				read_barrier_.set(pos, end);
				return true;
			}

			void dequeue(T &value)
			{
				auto base = read_barrier_.reserve(1);
				auto end = base + 1;
				upper_barriers_.wait(end);
				auto pos = base::moder::mode(base);
				rapid_ring::meta::smart_copy<T>::copy
				(
					value, 
					(((ring_buffer_disruptor *&) (ring_))->base::buffer_)[pos]
			 	);
				read_barrier_.set(pos, end);
			}

			bool try_dequeue(T &value)
			{
				auto base = read_barrier_.try_reserve();
				auto end = base + 1;
				if (0 == upper_barriers_.try_wait(end))
				{
					return false;
				}
				if (!read_barrier_.set_reserve(base, end))
				{
					return false;
				}
				auto pos = base::moder::mode(base);
				rapid_ring::meta::smart_copy<T>::copy
				(
					value, 
					(((ring_buffer_disruptor *&) (ring_))->base::buffer_)[pos]
				);
				read_barrier_.set(pos, end);
				return true;
			}

			reader try_block_dequeue(uint64_t size)
			{
				reader r;
				static_assert(std::is_trivially_copyable<typename base::node>::value, "non copytable");
				auto base = read_barrier_.try_reserve();
				auto end = base + size;
				if (0 == upper_barriers_.try_wait(end))
				{
					return r;
				}
				if (!read_barrier_.set_reserve(base, end))
				{
					return r;
				}
				auto pos = base::moder::mode(base);
				auto rest = base::buffer_size - pos;
				if (rest >= size)
				{
					r.buff_ = ((ring_buffer_disruptor *&) (ring_))->base::buffer_ + pos;
					r.size_first_ = size;
					r.size_ = size;
				}
				else
				{

					r.buff_ = ((ring_buffer_disruptor *&) (ring_))->base::buffer_ + pos;
					r.size_first_ = rest;
					r.buff2_ = ((ring_buffer_disruptor *&) (ring_))->base::buffer_;
					r.size_ = size;
				}
				r.cb_ = std::move([this, pos, end]()
				{
					read_barrier_.set(pos, end);
				});
				return r;
			}
		};

	public:
		using comsumer_st = comsumer<rapid_ring::barrier::ring_buffer_barrier_st>;
		using comsumer_mt = comsumer<rapid_ring::barrier::ring_buffer_barrier_mt>;
	public:
		template<typename... TArgs>
		ring_buffer_disruptor(TArgs... args)
				:
				base(args...)
		{
		}

		virtual ~ring_buffer_disruptor()
		{
		}

	public:
		virtual uint64_t wait(uint64_t min) override
		{
			return base::write_barrier_.wait(min);
		}

		virtual uint64_t try_wait(uint64_t min) override
		{
			return base::write_barrier_.try_wait(min);
		}

		virtual void add(rapid_ring::barrier::barrier_interface &b) override
		{
			base::read_barrier_.add(b);
		}

		virtual void clear() override
		{
			base::read_barrier_.clear();
		}

		auto get_configure()
		{
			return configure(*this);
		}
	};

	template
	<
		typename T, 
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE = 64, 
		template<class>	class allocator_tmp = std::allocator
	>
	using sp_ring_buffer_disruptor = ring_buffer_disruptor<rapid_ring::barrier::ring_buffer_barrier_st, T, SIZE, CACHELINE_SIZE, allocator_tmp>;

	template
	<
		typename T, 
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE = 64, 
		template<class>	class allocator_tmp = std::allocator
	>
	using mp_ring_buffer_disruptor = ring_buffer_disruptor<rapid_ring::barrier::ring_buffer_barrier_mt, T, SIZE, CACHELINE_SIZE, allocator_tmp>;

}
#endif //__RAPID_RING_RING_BUFFER_DISRUPTOR_HPP__
