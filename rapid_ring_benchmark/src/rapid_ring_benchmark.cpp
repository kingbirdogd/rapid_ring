#include <iostream>
#include <thread>
#include <rapid_ring/ring_buffer_queue.hpp>
#include <rapid_ring/ring_buffer_disruptor.hpp>

template
<
	template
	<
		class T, 
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE, 
		template<class> class allocator_tmp
	> class queue_type,
	uint64_t BUFFER_SIZE,
	uint64_t WRITE_CNT, 
	uint64_t BLOCK_SIZE, 
	uint64_t WRITE_THREAD_CNT, 
	uint64_t READ_THREAD_CNT, 
	uint64_t CACHELINE_SIZE = 64,
	template<class> class allocator_tmp = std::allocator
>
class ring_buffer_queue_benchmark
{
private:
	static_assert(BLOCK_SIZE <= 1000 && BLOCK_SIZE >= 1, "test BLOCK_SIZE should less than 1000 and large than 10");
private:
	const static uint64_t WRITE_SIZE = WRITE_CNT * BLOCK_SIZE;
	const static uint64_t READ_CNT = WRITE_CNT * WRITE_THREAD_CNT / READ_THREAD_CNT;
	const static uint64_t READ_SIZE = READ_CNT * BLOCK_SIZE;
	const static uint64_t thread_base = 10000000000;
private:
	using test_queue = queue_type<uint64_t, BUFFER_SIZE, CACHELINE_SIZE, allocator_tmp>;
	using queue_node_type = typename test_queue::node;
private:
	class writer
	{
	private:
	private:
		std::thread th_;
		queue_node_type buffer_[BLOCK_SIZE];
		queue_node_type *check_buffer_;
		test_queue &q_;
		uint64_t time_spand_;
		uint64_t thread_id_;
		bool is_finished_;
		bool check_;
	private:
		inline void write()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.block_enqueue(buffer_, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void write_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.block_enqueue(check_buffer_ + i * BLOCK_SIZE, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void write_one()
		{
			uint64_t value = 0;
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.enqueue(value);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void write_one_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.enqueue(check_buffer_[i]);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

	public:
		inline writer(test_queue &q, uint64_t thread_id, bool check)
				:
				th_()
				, buffer_{}
				, check_buffer_(nullptr)
				, q_(q)
				, time_spand_(0)
				, thread_id_(thread_id)
				, is_finished_(false)
				, check_(check)
		{
			if (check_)
			{
				auto base = thread_base * thread_id_ + 1;
				check_buffer_ = new queue_node_type[WRITE_SIZE];
				for (uint64_t i = 0; i < WRITE_SIZE; ++i)
					(uint64_t &) (check_buffer_[i]) = base + i;
			}
		}

		inline ~writer()
		{
			if (check_)
			{
				delete[] check_buffer_;
			}
		}

		inline void start()
		{
			if (check_)
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								 write_check();
							}
						)
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								write_one_check();
							}
						 )
					);
				}
			}
			else
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								 write();
							}
				 		)
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								write_one();
							}
						 )
					);
				}
			}
		}

		inline bool is_finished()
		{
			if (is_finished_)
			{
				return true;
			}
			if (th_.joinable())
			{
				th_.join();
				is_finished_ = true;
				return true;
			}
			return false;
		}

		inline void report()
		{
			std::cout << "write thread id:" << thread_id_
					  << ", time consume:" << time_spand_ << " nanoseconds"
					  << ", item count:" << WRITE_SIZE
					  << ", operation count:" << WRITE_CNT
					  << ", nanos pre item:" << time_spand_ / WRITE_SIZE
					  << ", nanos pre operations:" << time_spand_ / WRITE_CNT
					  << ", ips(items per second):" << WRITE_SIZE * 1000000000 / time_spand_
					  << ", ops(operation per second):" << WRITE_CNT * 1000000000 / time_spand_
					  << std::endl;
		}

	private:
		template<typename T, typename... TArgs>
		static uint64_t max_time_spend(T &v1, TArgs &... args)
		{
			return std::max(v1.time_spand_, max_time_spend(args...));
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1, T &v2)
		{
			return std::max(v1.time_spand_, v2.time_spand_);
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1)
		{
			return v1.time_spand_;
		}

	public:
		template<typename... TArgs>
		static void report(TArgs &... args)
		{
			auto time_spend = max_time_spend(args...);
			std::cout << "write thread summary, thread count:" << WRITE_THREAD_CNT
					  << ", time consume:" << time_spend << " nanoseconds"
					  << ", item count:" << (WRITE_SIZE * WRITE_THREAD_CNT)
					  << ", operation count:" << (WRITE_CNT * WRITE_THREAD_CNT)
					  << ", nanos pre item:" << time_spend / (WRITE_SIZE * WRITE_THREAD_CNT)
					  << ", nanos pre operations:" << time_spend / (WRITE_CNT * WRITE_THREAD_CNT)
					  << ", ips(items per second):" << (WRITE_SIZE * WRITE_THREAD_CNT) * 1000000000 / time_spend
					  << ", ops(operation per second):" << (WRITE_CNT * WRITE_THREAD_CNT) * 1000000000 / time_spend
					  << std::endl;
		}
	};

	class reader
	{
	private:
		std::thread th_;
		queue_node_type buffer_[BLOCK_SIZE];
		queue_node_type *check_buffer_;
		test_queue &q_;
		uint64_t time_spand_;
		uint64_t thread_id_;
		bool is_finished_;
		bool check_;
	private:
		inline void read()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.block_dequeue(buffer_, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void read_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.block_dequeue(check_buffer_ + i * BLOCK_SIZE, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void read_one()
		{
			uint64_t value;
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.dequeue(value);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void read_one_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.dequeue(check_buffer_[i]);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

	public:
		inline reader(test_queue &q, uint64_t thread_id, bool check)
				:
				th_()
				, buffer_{}
				, check_buffer_(nullptr)
				, q_(q)
				, time_spand_(0)
				, thread_id_(thread_id)
				, is_finished_(false)
				, check_(check)
		{
			if (check_)
			{
				check_buffer_ = new queue_node_type[READ_SIZE];
			}
		}

		inline ~reader()
		{
			if (check_)
			{
				delete[] check_buffer_;
			}
		}

		inline void start()
		{
			if (check_)
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								read_check();
							}
						 )
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								read_one_check();
							}
						)
					);
				}
			}
			else
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								read();
							}
						)
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								read_one();
							}
						)
					);
				}
			}
		}

		inline bool is_finished()
		{
			if (is_finished_)
			{
				return true;
			}
			if (th_.joinable())
			{
				th_.join();
				is_finished_ = true;
				return true;
			}
			return false;
		}

		inline void check()
		{
			std::vector<uint64_t> vec;
			for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
				vec.push_back(0);
			for (uint64_t i = 0; i < READ_SIZE; ++i)
			{
				auto thread_id = check_buffer_[i] / thread_base;
				uint64_t value = check_buffer_[i] % thread_base;
				if (value <= vec[thread_id])
				{
					std::cout << "Check fail, read thread id:"
							  << thread_id_ << ", duplicate read write thread id:" << thread_id
							  << ", current value:" << vec[thread_id]
							  << ", error value:" << value
							  << std::endl;
				}
				else
				{
					vec[thread_id] = value;
				}
			}
		}

		inline void report()
		{
			if (check_)
			{
				check();
			}
			std::cout << "read thread id:" << thread_id_
					  << ", item count:" << READ_SIZE
					  << ", operation count:" << READ_CNT
					  << ", time consume:" << time_spand_ << " nanoseconds"
					  << ", nanos pre item:" << time_spand_ / READ_SIZE
					  << ", nanos pre operations:" << time_spand_ / READ_CNT
					  << ", ips(items per second):" << READ_SIZE * 1000000000 / time_spand_
					  << ", ops(operation per second):" << READ_CNT * 1000000000 / time_spand_
					  << std::endl;
		}

	private:
		template<typename T, typename... TArgs>
		static uint64_t max_time_spend(T &v1, TArgs &... args)
		{
			return std::max(v1.time_spand_, max_time_spend(args...));
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1, T &v2)
		{
			return std::max(v1.time_spand_, v2.time_spand_);
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1)
		{
			return v1.time_spand_;
		}

	public:
		template<typename... TArgs>
		static void report(TArgs &... args)
		{
			auto time_spend = max_time_spend(args...);
			std::cout << "read thread summary, thread count:" << READ_THREAD_CNT
					  << ", time consume:" << time_spend << " nanoseconds"
					  << ", item count:" << (READ_SIZE * READ_THREAD_CNT)
					  << ", operation count:" << (READ_CNT * READ_THREAD_CNT)
					  << ", nanos pre item:" << time_spend / (READ_SIZE * READ_THREAD_CNT)
					  << ", nanos pre operations:" << time_spend / (READ_CNT * READ_THREAD_CNT)
					  << ", ips(items per second):" << (READ_SIZE * READ_THREAD_CNT) * 1000000000 / time_spend
					  << ", ops(operation per second):" << (READ_CNT * READ_THREAD_CNT) * 1000000000 / time_spend
					  << std::endl;
		}
	};

private:
	test_queue q_;
	writer *w_;
	reader *r_;
public:
	inline ring_buffer_queue_benchmark(bool check = false)
			:
			q_()
			, w_(nullptr)
			, r_(nullptr)
	{
		q_.warm_up();
		w_ = reinterpret_cast<writer *>(new uint8_t[sizeof(writer) * WRITE_THREAD_CNT]);
		r_ = reinterpret_cast<reader *>(new uint8_t[sizeof(reader) * READ_THREAD_CNT]);
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			new(w_ + i) writer(q_, i, check);
		for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			new(r_ + i) reader(q_, i, check);
	}

	inline ~ring_buffer_queue_benchmark()
	{
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			w_[i].~writer();
		for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			r_[i].~reader();
		delete[] reinterpret_cast<uint8_t *>(w_);
		delete[] reinterpret_cast<uint8_t *>(r_);
	}

	inline void test()
	{
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			w_[i].start();
		for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			r_[i].start();
		while (true)
		{
			uint64_t finished_cnt = 0;
			for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			{
				if (w_[i].is_finished())
				{
					++finished_cnt;
				}
			}
			if (finished_cnt == WRITE_THREAD_CNT)
			{
				break;
			}
		}
		while (true)
		{
			uint64_t finished_cnt = 0;
			for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			{
				if (r_[i].is_finished())
				{
					++finished_cnt;
				}
			}
			if (finished_cnt == READ_THREAD_CNT)
			{
				break;
			}
		}
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			w_[i].report();
		for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			r_[i].report();
		write_report();
		read_report();
	}

private:
	template<uint64_t N, typename T, typename... TArgs>
	struct report_helper
	{
		static void report(T *array, TArgs &... args)
		{
			report_helper<N - 1, T, T &, TArgs...>::report(array, array[N], args...);
		}
	};

	template<typename T, typename... TArgs>
	struct report_helper<0, T, TArgs...>
	{
		static void report(T *array, TArgs &... args)
		{
			T::report(array[0], args...);
		}
	};

	void write_report()
	{
		report_helper<WRITE_THREAD_CNT - 1, writer>::report(w_);
	}

	void read_report()
	{
		report_helper<READ_THREAD_CNT - 1, reader>::report(r_);
	}
};


template
<
	template
	<
		class T,
		uint64_t SIZE, 
		uint64_t CACHELINE_SIZE, 
		template<class> class allocator_tmp
	>class queue_type,
	uint64_t BUFFER_SIZE, 
	uint64_t WRITE_CNT, 
	uint64_t BLOCK_SIZE, 
	uint64_t WRITE_THREAD_CNT, 
	uint64_t CACHELINE_SIZE = 64,
	template<class> class allocator_tmp = std::allocator
>
class ring_buffer_disruptor_benchmark
{
/*
 client grap:
                        ->C1->
                    ->         ->
                ->               ->
    ring_buffer -> ->   ->C2 ->  -> ->C3
                ->          <-  <- <-
                    ->C4<-
    C1 link to ring
    C2 link to ring
    C3 ling to C1, C2
    C4 link to ring, C3
 */
private:
	static_assert(BLOCK_SIZE <= 1000 && BLOCK_SIZE >= 1, "test BLOCK_SIZE should less than 1000 and large than 10");
private:
	const static uint64_t WRITE_SIZE = WRITE_CNT * BLOCK_SIZE;
	const static uint64_t READ_CNT = WRITE_CNT * WRITE_THREAD_CNT;
	const static uint64_t READ_SIZE = READ_CNT * BLOCK_SIZE;
	const static uint64_t READ_THREAD_CNT = 4;
	const static uint64_t thread_base = 10000000000;
private:
	using test_queue = queue_type<uint64_t, BUFFER_SIZE, CACHELINE_SIZE, allocator_tmp>;
	using test_comsumer = typename test_queue::comsumer_st;
	using queue_node_type = typename test_queue::node;
private:
	class writer
	{
	private:
	private:
		std::thread th_;
		queue_node_type buffer_[BLOCK_SIZE];
		queue_node_type *check_buffer_;
		test_queue &q_;
		uint64_t time_spand_;
		uint64_t thread_id_;
		bool is_finished_;
		bool check_;
	private:
		inline void write()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.block_enqueue(buffer_, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void write_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.block_enqueue(check_buffer_ + i * BLOCK_SIZE, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void write_one()
		{
			uint64_t value = 0;
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.enqueue(value);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void write_one_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < WRITE_CNT; ++i)
				q_.enqueue(check_buffer_[i]);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

	public:
		inline writer(test_queue &q, uint64_t thread_id, bool check)
				:
				th_(),
				buffer_{},
				check_buffer_(nullptr),
				q_(q),
				time_spand_(0),
				thread_id_(thread_id),
				is_finished_(false),
				check_(check)
		{
			if (check_)
			{
				auto base = thread_base * thread_id_ + 1;
				check_buffer_ = new queue_node_type[WRITE_SIZE];
				for (uint64_t i = 0; i < WRITE_SIZE; ++i)
					(uint64_t &) (check_buffer_[i]) = base + i;
			}
		}

		inline ~writer()
		{
			if (check_)
			{
				delete[] check_buffer_;
			}
		}

		inline void start()
		{
			if (check_)
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								write_check();
							}
						)
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{ 
								write_one_check();
							}
						)
					);
				}
			}
			else
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								write();
							}
						)
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								write_one();
							}
						)
					);
				}
			}
		}

		inline bool is_finished()
		{
			if (is_finished_)
			{
				return true;
			}
			if (th_.joinable())
			{
				th_.join();
				is_finished_ = true;
				return true;
			}
			return false;
		}

		inline void report()
		{
			std::cout << "write thread id:" << thread_id_
					  << ", time consume:" << time_spand_ << " nanoseconds"
					  << ", item count:" << WRITE_SIZE
					  << ", operation count:" << WRITE_CNT
					  << ", nanos pre item:" << time_spand_ / WRITE_SIZE
					  << ", nanos pre operations:" << time_spand_ / WRITE_CNT
					  << ", ips(items per second):" << WRITE_SIZE * 1000000000 / time_spand_
					  << ", ops(operation per second):" << WRITE_CNT * 1000000000 / time_spand_
					  << std::endl;
		}

	private:
		template<typename T, typename... TArgs>
		static uint64_t max_time_spend(T &v1, TArgs &... args)
		{
			return std::max(v1.time_spand_, max_time_spend(args...));
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1, T &v2)
		{
			return std::max(v1.time_spand_, v2.time_spand_);
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1)
		{
			return v1.time_spand_;
		}

	public:
		template<typename... TArgs>
		static void report(TArgs &... args)
		{
			auto time_spend = max_time_spend(args...);
			std::cout << "write thread summary, thread count:" << WRITE_THREAD_CNT
					  << ", time consume:" << time_spend << " nanoseconds"
					  << ", item count:" << (WRITE_SIZE * WRITE_THREAD_CNT)
					  << ", operation count:" << (WRITE_CNT * WRITE_THREAD_CNT)
					  << ", nanos pre item:" << time_spend / (WRITE_SIZE * WRITE_THREAD_CNT)
					  << ", nanos pre operations:" << time_spend / (WRITE_CNT * WRITE_THREAD_CNT)
					  << ", ips(items per second):" << (WRITE_SIZE * WRITE_THREAD_CNT) * 1000000000 / time_spend
					  << ", ops(operation per second):" << (WRITE_CNT * WRITE_THREAD_CNT) * 1000000000 / time_spend
					  << std::endl;
		}
	};

	class reader
	{
	private:
		std::thread th_;
		queue_node_type buffer_[BLOCK_SIZE];
		queue_node_type *check_buffer_;
		test_comsumer &q_;
		uint64_t time_spand_;
		uint64_t thread_id_;
		bool is_finished_;
		bool check_;
	private:
		inline void read()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.block_dequeue(buffer_, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void read_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.block_dequeue(check_buffer_ + i * BLOCK_SIZE, BLOCK_SIZE);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void read_one()
		{
			uint64_t value;
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.dequeue(value);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

		inline void read_one_check()
		{
			auto start = std::chrono::high_resolution_clock::now();
			for (uint64_t i = 0; i < READ_CNT; ++i)
				q_.dequeue(check_buffer_[i]);
			auto finish = std::chrono::high_resolution_clock::now();
			time_spand_ = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
		}

	public:
		inline reader(test_comsumer &q, uint64_t thread_id, bool check)
				:
				th_()
				, buffer_{}
				, check_buffer_(nullptr)
				, q_(q)
				, time_spand_(0)
				, thread_id_(thread_id)
				, is_finished_(false)
				, check_(check)
		{
			if (check_)
			{
				check_buffer_ = new queue_node_type[READ_SIZE];
			}
		}

		inline ~reader()
		{
			if (check_)
			{
				delete[] check_buffer_;
			}
		}

		inline void start()
		{
			if (check_)
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								read_check();
							}
						)
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								read_one_check();
							}
						)
					);
				}
			}
			else
			{
				if (1 != BLOCK_SIZE)
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{ 
								read();
							}
						)
					);
				}
				else
				{
					th_ = std::move
					(
						std::thread
						(
							[&]()
							{
								read_one();
							}
						)
					);
				}
			}
		}

		inline bool is_finished()
		{
			if (is_finished_)
			{
				return true;
			}
			if (th_.joinable())
			{
				th_.join();
				is_finished_ = true;
				return true;
			}
			return false;
		}

		inline void check()
		{
			std::vector<uint64_t> vec;
			for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
				vec.push_back(0);
			for (uint64_t i = 0; i < READ_SIZE; ++i)
			{
				auto thread_id = check_buffer_[i] / thread_base;
				uint64_t value = check_buffer_[i] % thread_base;
				if (value <= vec[thread_id])
				{
					std::cout << "Check fail, read thread id:"
							  << thread_id_ << ", duplicate read write thread id:" << thread_id
							  << ", current value:" << vec[thread_id]
							  << ", error value:" << value
							  << std::endl;
				}
				else
				{
					vec[thread_id] = value;
				}
			}
		}

		inline void report()
		{
			if (check_)
			{
				check();
			}
			std::cout << "read thread id:" << thread_id_
					  << ", item count:" << READ_SIZE
					  << ", operation count:" << READ_CNT
					  << ", time consume:" << time_spand_ << " nanoseconds"
					  << ", nanos pre item:" << time_spand_ / READ_SIZE
					  << ", nanos pre operations:" << time_spand_ / READ_CNT
					  << ", ips(items per second):" << READ_SIZE * 1000000000 / time_spand_
					  << ", ops(operation per second):" << READ_CNT * 1000000000 / time_spand_
					  << std::endl;
		}

	private:
		template<typename T, typename... TArgs>
		static uint64_t max_time_spend(T &v1, TArgs &... args)
		{
			return std::max(v1.time_spand_, max_time_spend(args...));
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1, T &v2)
		{
			return std::max(v1.time_spand_, v2.time_spand_);
		}

		template<typename T>
		static uint64_t max_time_spend(T &v1)
		{
			return v1.time_spand_;
		}

	public:
		template<typename... TArgs>
		static void report(TArgs &... args)
		{
			auto time_spend = max_time_spend(args...);
			std::cout << "read thread summary, thread count: 4"
					  << ", time consume:" << time_spend << " nanoseconds"
					  << ", item count:" << READ_SIZE * READ_THREAD_CNT
					  << ", operation count:" << READ_CNT * READ_THREAD_CNT
					  << ", nanos pre item:" << time_spend / READ_SIZE / READ_THREAD_CNT
					  << ", nanos pre operations:" << time_spend / READ_CNT / READ_THREAD_CNT
					  << ", ips(items per second):" << READ_SIZE * READ_THREAD_CNT * 1000000000 / time_spend
					  << ", ops(operation per second):" << READ_CNT * READ_THREAD_CNT * 1000000000 / time_spend
					  << std::endl;
		}
	};

private:
	test_queue q_;
	test_comsumer c1_;
	test_comsumer c2_;
	test_comsumer c3_;
	test_comsumer c4_;
	writer *w_;
	reader *r_;
public:
	inline ring_buffer_disruptor_benchmark(bool check = false)
			:
			q_()
			, c1_(q_)
			, c2_(q_)
			, c3_(q_)
			, c4_(q_)
			, w_(nullptr)
			, r_(nullptr)
	{
		w_ = reinterpret_cast<writer *>(new uint8_t[sizeof(writer) * WRITE_THREAD_CNT]);
		r_ = reinterpret_cast<reader *>(new uint8_t[sizeof(reader) * READ_THREAD_CNT]);
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			new(w_ + i) writer(q_, i, check);
		new(r_ + 0) reader(c1_, 0, check);
		new(r_ + 1) reader(c2_, 1, check);
		new(r_ + 2) reader(c3_, 2, check);
		new(r_ + 3) reader(c4_, 3, check);
		auto cfg = q_.get_configure();
		cfg.x_depends_y(c1_, q_);
		cfg.x_depends_y(c2_, q_);
		cfg.x_depends_y(c3_, c1_);
		cfg.x_depends_y(c3_, c2_);
		cfg.x_depends_y(c4_, c3_);
		cfg.x_depends_y(c3_, q_);
		cfg.commit();
		cfg.warm_up();
	}

	inline ~ring_buffer_disruptor_benchmark()
	{
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			w_[i].~writer();
		for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			r_[i].~reader();
		delete[] reinterpret_cast<uint8_t *>(w_);
		delete[] reinterpret_cast<uint8_t *>(r_);
	}

	inline void test()
	{
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			w_[i].start();
		for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			r_[i].start();
		while (true)
		{
			uint64_t finished_cnt = 0;
			for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			{
				if (w_[i].is_finished())
				{
					++finished_cnt;
				}
			}
			if (finished_cnt == WRITE_THREAD_CNT)
			{
				break;
			}
		}
		while (true)
		{
			uint64_t finished_cnt = 0;
			for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			{
				if (r_[i].is_finished())
				{
					++finished_cnt;
				}
			}
			if (finished_cnt == READ_THREAD_CNT)
			{
				break;
			}
		}
		for (uint64_t i = 0; i < WRITE_THREAD_CNT; ++i)
			w_[i].report();
		for (uint64_t i = 0; i < READ_THREAD_CNT; ++i)
			r_[i].report();
		write_report();
		read_report();
	}

private:
	template<uint64_t N, typename T, typename... TArgs>
	struct report_helper
	{
		static void report(T *array, TArgs &... args)
		{
			report_helper<N - 1, T, T &, TArgs...>::report(array, array[N], args...);
		}
	};

	template<typename T, typename... TArgs>
	struct report_helper<0, T, TArgs...>
	{
		static void report(T *array, TArgs &... args)
		{
			T::report(array[0], args...);
		}
	};

	void write_report()
	{
		report_helper<WRITE_THREAD_CNT - 1, writer>::report(w_);
	}

	void read_report()
	{
		report_helper<READ_THREAD_CNT - 1, reader>::report(r_);
	}
};

struct RemoteTest
{
	RemoteTest()
	{
		std::cout << "RemoteTest" << std::endl;
	}
	virtual ~RemoteTest()
	{
		std::cout << "~RemoteTest" << std::endl;
	}
};

int main(void)
{
	{
		rapid_ring::mpmc_ring_buffer_queue<RemoteTest, 10> test;
	}
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 200000000, 1, 1, 1> test1;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 20000000, 10, 1, 1> test2;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 2000000, 100, 1, 1> test3;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 200000, 1000, 1, 1> test4;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 50000000, 1, 4, 4> test5;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 5000000, 10, 4, 4> test6;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 500000, 100, 4, 4> test7;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 50000, 1000, 4, 4> test8;
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 2000000, 1, 1, 1> test1_check(true);
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 200000, 10, 1, 1> test2_check(true);
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 20000, 100, 1, 1> test3_check(true);
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 200, 1000, 1, 1> test4_check(true);
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 500000, 1, 4, 4> test5_check(true);
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 50000, 10, 4, 4> test6_check(true);
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 5000, 100, 4, 4> test7_check(true);
	ring_buffer_queue_benchmark<rapid_ring::mpmc_ring_buffer_queue, 1000000, 500, 1000, 4, 4> test8_check(true);
	ring_buffer_disruptor_benchmark<rapid_ring::mp_ring_buffer_disruptor, 1000000, 50000000, 1, 4> disruptor_test;
	ring_buffer_disruptor_benchmark<rapid_ring::mp_ring_buffer_disruptor, 1000000, 5000000, 1, 4> disruptor_test_check(
			true
																													 );
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 200000000, 1, 1, 1> test1_sp;
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 20000000, 10, 1, 1> test2_sp;
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 2000000, 100, 1, 1> test3_sp;
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 200000, 1000, 1, 1> test4_sp;
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 2000000, 1, 1, 1> test1_check_sp(true);
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 200000, 10, 1, 1> test2_check_sp(true);
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 20000, 100, 1, 1> test3_check_sp(true);
	ring_buffer_queue_benchmark<rapid_ring::spsc_ring_buffer_queue, 1000000, 200, 1000, 1, 1> test4_check_sp(true);
	ring_buffer_disruptor_benchmark<rapid_ring::sp_ring_buffer_disruptor, 1000000, 200000000, 1, 1> disruptor_test_sp;
	ring_buffer_disruptor_benchmark<rapid_ring::sp_ring_buffer_disruptor, 1000000, 20000000, 1, 1> disruptor_test_check_sp(
			true
																														 );
	std::cout << "Start test block size: 1, thread 1, 1" << std::endl;
	test1.test();
	std::cout << "Start test block size: 10, thread 1, 1" << std::endl;
	test2.test();
	std::cout << "Start test block size: 100, thread 1, 1" << std::endl;
	test3.test();
	std::cout << "Start test block size: 1000, thread 1, 1" << std::endl;
	test4.test();
	std::cout << "Start test block size: 1, thread 4, 4" << std::endl;
	test5.test();
	std::cout << "Start test block size: 10, thread 4, 4" << std::endl;
	test6.test();
	std::cout << "Start test block size: 100, thread 4, 4" << std::endl;
	test7.test();
	std::cout << "Start test block size: 1000, thread 4, 4" << std::endl;
	test8.test();
	std::cout << "Start test block with check size: 1, thread 1, 1" << std::endl;
	test1_check.test();
	std::cout << "Start test block with check size: 10, thread 1, 1" << std::endl;
	test2_check.test();
	std::cout << "Start test block with check size: 100, thread 1, 1" << std::endl;
	test3_check.test();
	std::cout << "Start test block with check size: 1000, thread 1, 1" << std::endl;
	test4_check.test();
	std::cout << "Start test block with check size: 1, thread 4, 4" << std::endl;
	test5_check.test();
	std::cout << "Start test block with check size: 10, thread 4, 4" << std::endl;
	test6_check.test();
	std::cout << "Start test block with check size: 100, thread 4, 4" << std::endl;
	test7_check.test();
	std::cout << "Start test block with check size: 1000, thread 4, 4" << std::endl;
	test8_check.test();
	std::cout << "Start test disruptor block size: 1, write thread 4" << std::endl;
	disruptor_test.test();
	std::cout << "Start test disruptor block with check size: 1, write thread 4" << std::endl;
	disruptor_test_check.test();
	std::cout << "SPSC Start test block size: 1, thread 1, 1" << std::endl;
	test1_sp.test();
	std::cout << "SPSC Start test block size: 10, thread 1, 1" << std::endl;
	test2_sp.test();
	std::cout << "SPSC Start test block size: 100, thread 1, 1" << std::endl;
	test3_sp.test();
	std::cout << "SPSC Start test block size: 1000, thread 1, 1" << std::endl;
	test4_sp.test();
	std::cout << "SPSC Start test block with check size: 1, thread 1, 1" << std::endl;
	test1_check_sp.test();
	std::cout << "SPSC Start test block with check size: 10, thread 1, 1" << std::endl;
	test2_check_sp.test();
	std::cout << "SPSC Start test block with check size: 100, thread 1, 1" << std::endl;
	test3_check_sp.test();
	std::cout << "SPSC Start test block with check size: 1000, thread 1, 1" << std::endl;
	test4_check_sp.test();
	std::cout << "SPSC Start test disruptor block size: 1, write thread 4" << std::endl;
	disruptor_test_sp.test();
	std::cout << "SPSC Start test disruptor block with check size: 1, write thread 4" << std::endl;
	disruptor_test_check_sp.test();
	return 0;
}
