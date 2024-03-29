#include <thread>
#include <cassert>
#include <iostream>
#include "loop.hh"
#include "utils.hh"
#include "../include/context.hh"
#include "../include/types.hh"

using namespace asbi;

evts::Loop::Loop(Context* ctx, unsigned int start_threads): ctx(ctx), tasksRunning(0), tasksSem(0), eventsSem(0) {
	for (unsigned int i = 0; i < start_threads; i++)
		newThread();
}

evts::Loop::~Loop() {
	assert(events.empty() && tasksRunning == 0 && tasks.empty() && timers.empty());
	{
		std::unique_lock<std::mutex> lock(tasksMtx);
		for (unsigned int i = 0; i < threads.size(); ++i) {
			tasks.push_back(nullptr);
			tasksSem.release();
		}
	}

	for (auto &thread: threads)
		thread.join();
}


static void worker(evts::Loop* evtloop) {
	evts::Task* task = nullptr;
	while ((task = evtloop->getTask()) != nullptr) {
		task->fn([evtloop, task](std::vector<Value> *args, bool done) -> void {
			evtloop->addEvent(task, args, done);
		});

		// std::cerr << "Worker#" << std::this_thread::get_id() << " finished Task " << (void*)task << '\n';

		delete task;
	}
}

void evts::Loop::newThread() {
	threads.push_back(std::thread(worker, this));
}

void evts::Loop::start() {
	evts::Event* evt;
	while ((evt = getEvent()) != nullptr) {
		for (auto rit = evt->args.rbegin(); rit != evt->args.rend(); ++rit)
			ctx->push(*rit);

		Value::lambda(evt->callback).call(ctx, evt->args.size(), nullptr);
		delete evt;
	}
}

void evts::Loop::addTask(LambdaContainer* callback, taskfn_t fn) {
	addTask(new evts::Task(fn, callback));
}

void evts::Loop::addTask(evts::Task* task) {
	std::unique_lock<std::mutex> lock(tasksMtx);
	task->callback->gc_manage = false;
	tasks.push_back(task);
	if (tasksRunning >= threads.size())
		newThread();

	tasksSem.release();
}

evts::Task* evts::Loop::getTask() {
	tasksSem.acquire();

	std::unique_lock<std::mutex> lock(tasksMtx);
	auto task = tasks.front();
	tasks.pop_front();
	tasksRunning++;
	return task;
}

void evts::Loop::addEvent(evts::Task* task, std::vector<Value> *args, bool done) {
	auto evt = new evts::Event();
	evt->taskDone = done;
	evt->callback = task->callback;
	if (args == nullptr) {
		evt->doCall = false;
	} else {
		evt->args = *args;
		evt->doCall = true;
	}
	addEvent(evt);
}

void evts::Loop::addEvent(evts::Event* evt) {
	std::unique_lock<std::mutex> lock(eventsMtx);

	if (evt->taskDone) {
		std::unique_lock<std::mutex> lock(tasksMtx);
		tasksRunning--;
	}

	events.push_back(evt);
	eventsSem.release();
}

evts::Event* evts::Loop::getEvent() {
	// check if done:
	// TODO: extract in other function to return early and avoid unnecessary locking
	{
		std::unique_lock<std::mutex> lock1(eventsMtx);
		std::unique_lock<std::mutex> lock2(tasksMtx);
		if (events.empty() && tasksRunning == 0 && tasks.empty() && timers.empty())
			return nullptr;
	}

	// check for timers
	evts::Timer timer;
	if (getNextTimer(timer)) {
		auto evt = new evts::Event();
		evt->taskDone = true;
		evt->callback = timer.callback;
		evt->callback->gc_manage = true;
		evt->doCall = true;
		return evt;
	}


	// wait for event from task or timer
	if (timers.empty()) {
		eventsSem.acquire();
	} else {
		std::chrono::duration<double, std::milli> tts = timers.first().dueAt - evts::timestamp();
		auto timedout = !eventsSem.acquireFor(tts);
		if (timedout)
			return getEvent(); // so that we can check on timers again
	}


	// take event from queue
	evts::Event* evt;
	{
		std::unique_lock<std::mutex> lock(eventsMtx);
		assert(!events.empty());
		evt = events.front();
		events.pop_front();

		if (evt->taskDone)
			evt->callback->gc_manage = true;
	}

	if (evt->doCall) {
		return evt;
	} else {
		delete evt;
		return getEvent();
	}
}


bool evts::Loop::getNextTimer(evts::Timer& out) {
	if (timers.empty())
		return false;

	auto now = evts::timestamp();
	auto first = timers.first();

	if (first.dueAt <= now) {
		out = first;
		timers.removeFirst();
		return true;
	}

	return false;
}

void evts::Loop::addTimer(LambdaContainer* callback, double dueAtSec) {
	callback->gc_manage = false;
	timers.insert(evts::Timer(callback, evts::to_chrono(dueAtSec)));
}
