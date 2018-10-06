#include "utils.hh"

using namespace asbi::evts;

void Semaphore::release() {
	std::unique_lock<std::mutex> lock(mut);
	counter++;
	condvar.notify_one();
}

void Semaphore::acquire() {
	std::unique_lock<std::mutex> lock(mut);
	condvar.wait(lock, [this](){ return this->counter > 0; });
	counter--;
}

void asbi::evts::to_usecs(double in, useconds_t &out) {
	out = static_cast<useconds_t>(in * 0.000001);
}
