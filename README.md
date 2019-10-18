# rapid_ring

## Getting Started

This Project Provide
1.Lock Free MPMC ring buffer queue

2.Lock Free SPMC ring buffer queue

3.Lock Free MPSC ring buffer queue

4.Lock Free SPSC ring buffer queue

5.Lock Free MP Disruptor (Mutiple Producer Disruptor)

6 Lock Free SP Disruptor (Single Producer Disruptor)

About Disruptor concept, please ref https://lmax-exchange.github.io/disruptor/, Here provide a C++ version

## Lock Free ring buffer queue
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
	class ring_buffer_queue;
}

### Interface

void block_dequeue(typename base::node *ptr, uint64_t size);

bool try_block_dequeue(typename base::node *ptr, uint64_t size);

void dequeue(T &value);

bool try_dequeue(T &value);

### Specializations Template
namespace rapid_ring
{

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

## Lock Free ring buffer Disruptor
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
	class ring_buffer_disruptor;
  
  	template
	<
		template<uint64_t, uint64_t, template<class> class> class write_barrier_tmp, 
		typename T,
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE,
		template<class> class allocator_tmp
	>
	class ring_buffer_disruptor::configure; //Comsumer configure helper class
  
  
	template
	<
		template<uint64_t, uint64_t, template<class> class> class write_barrier_tmp, 
		typename T,
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE,
		template<class> class allocator_tmp
	>
	class ring_buffer_disruptor::comsumer_st; //Single Thread Comsumer
  
  	template
	<
		template<uint64_t, uint64_t, template<class> class> class write_barrier_tmp, 
		typename T,
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE,
		template<class> class allocator_tmp
	>
	class ring_buffer_disruptor::comsumer_mt; //Multiple Thread Comsumer
  
  
  
  
}

### Disruptor Interface

		virtual uint64_t wait(uint64_t min) override;

		virtual void add(rapid_ring::barrier::barrier_interface &b) override;

		virtual void clear() override;

		auto get_configure();

### Disruptor Configure Helper Interface
			void x_depends_y(ref x, ref y);
      
			void commit();

			void clear();
      
### Disruptor Comsumer Helper Interface

			virtual void add(rapid_ring::barrier::barrier_interface &b) override;

			virtual void clear() override;

			virtual uint64_t wait(uint64_t min) override;

			void block_dequeue(typename base::node *ptr, uint64_t size);

			bool try_block_dequeue(typename base::node *ptr, uint64_t size);
      
      void dequeue(T &value);
 
## Benchmark and example

Please reference to rapid_ring_benchmark/src/rapid_ring_benchmark.cpp

Benchmark result with MacBook Pro (13-inch, 2019, Four Thunderbolt 3 ports), 2.4 GHz Quad-Core Intel Core i5, 16 GB 2133 MHz LPDDR3

Please reference to benchmark_raw.txt

## Authors

AU Gwok Dung David

Email: kingbirdogd@gmail.com

Linkedin: https://www.linkedin.com/in/david-au-15632561/


## License

This project is licensed under the BSD License







