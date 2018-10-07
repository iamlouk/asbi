#include "utils.hh"
#include <chrono>

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

bool Semaphore::tryAcquire() {
	std::unique_lock<std::mutex> lock(mut);
	if (counter > 0) {
		counter--;
		return true;
	}
	return false;
}

bool Semaphore::acquireFor(std::chrono::duration<double, std::milli> timeout) {
	std::unique_lock<std::mutex> lock(mut);
	if (condvar.wait_for(lock, timeout, [this](){ return this->counter > 0; })) {
		counter--;
		return true;
	}
	return false;
}

std::chrono::duration<double, std::milli> asbi::evts::timestamp() {
	using namespace std::chrono;
	auto ms = duration_cast<duration<double, std::milli>>(system_clock::now().time_since_epoch());
	return ms;
}

double asbi::evts::to_double_seconds(std::chrono::duration<double, std::milli> ms) {
	return ms.count() / 1000.0;
}

std::chrono::duration<double, std::milli> asbi::evts::to_chrono(double secs) {
	using namespace std::chrono;
	return duration<double, std::milli>(secs * 1000.0);
}
