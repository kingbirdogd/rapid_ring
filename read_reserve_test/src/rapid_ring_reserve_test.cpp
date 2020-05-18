#include <rapid_ring/ring_buffer_queue.hpp>
#include <rapid_ring/ring_buffer_disruptor.hpp>
#include <iostream>
#include <thread>
#include <exception>


void ring_buffer_disruptor_test()
{
	using queue = rapid_ring::mp_ring_buffer_disruptor<int, 16>;
	using comsumer = queue::comsumer_mt;
	std::vector<int> cnt1;
	std::vector<int> cnt2;
	queue q;
	comsumer c1(q);
	comsumer c2(q);
	auto cfg = q.get_configure();
	cfg.x_depends_y(c1, q);
	cfg.x_depends_y(c2, q);
	cfg.commit();
	std::thread t1([&]()
	{
		int v = 0;
		while (v < 2048)
		{
			int v2 = 0;
			if(c1.try_dequeue(v2))
			{
				if (1 != v2 - v)
				{
					throw std::runtime_error(std::string("T1 fail,v2:") + std::to_string(v2) + ",v:" + std::to_string(v));
				}
				v = v2;
				std::cout << "T1 dequeue:" << v << std::endl;
			}
		}
	});
	std::thread t2([&]()
	{
		int v = 0;
		while (v < 2048)
		{
			int v2 = 0;
			if(c2.try_dequeue(v2))
			{
				if (1 != v2 - v)
				{
					throw std::runtime_error(std::string("T2 fail,v2:") + std::to_string(v2) + ",v:" + std::to_string(v));
				}
				v = v2;
				std::cout << "T2 dequeue:" << v << std::endl;
			}
		}
	});
	for (int i = 1; i <= 2048; ++i)
	{
		std::cout << "Enqueue:" << i << std::endl;
		q.enqueue(i);
	}
	t1.join();
	t2.join();
	std::cout << "Disruptor test success!" << std::endl;
}

void ring_buffer_queue_test()
{
	using queue = rapid_ring::spsc_ring_buffer_queue<int, 64>;
	std::vector<int> cnt1;
	queue q;
	std::thread t1([&]()
	{
		int v = 0;
		while (v < 2048)
		{
			if(q.try_dequeue(v))
			{
				std::cout << "T1 dequeue:" << v << std::endl;
				cnt1.push_back(v);
			}
		}
	});
	for (int i = 1; i <= 2048; ++i)
	{
		std::cout << "Enqueue:" << i << std::endl;
		q.enqueue(i);
	}
	t1.join();
	std::cout << "cnt1:" << cnt1.size() << std::endl;
	std::cout << "Queue test success!" << std::endl;
}

int main(void)
{
	using queue = rapid_ring::spmc_ring_buffer_queue<int, 16>;
	queue q;
	q.enqueue(1);
	q.enqueue(2);
	q.enqueue(3);
	q.enqueue(4);
	q.enqueue(5);
	q.enqueue(6);
	q.enqueue(7);
	q.enqueue(8);
	q.enqueue(9);
	q.enqueue(10);
	auto r1 = q.try_block_dequeue(2);
	auto r2 = q.try_block_dequeue(6);
	std::cout << r2[0] << std::endl;
	std::cout << r2[1] << std::endl;
	std::cout << r2[2] << std::endl;
	std::cout << r2[3] << std::endl;
	std::cout << r2[4] << std::endl;
	std::cout << r2[5] << std::endl;
	std::cout << r1[0] << std::endl;
	std::cout << r1[1] << std::endl;
	r2.release();
	std::cout << r1[0] << std::endl;
	std::cout << r1[1] << std::endl;
	r1.release();
	q.enqueue(11);
	q.enqueue(12);
	q.enqueue(13);
	q.enqueue(14);
	q.enqueue(15);
	q.enqueue(16);
	q.enqueue(17);
	q.enqueue(18);
	q.enqueue(19);
	q.enqueue(20);
	ring_buffer_queue_test();
	ring_buffer_disruptor_test();
	return 0;
}




