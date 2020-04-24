#include <rapid_ring/ring_buffer_queue.hpp>
#include <iostream>

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
	return 0;
}




