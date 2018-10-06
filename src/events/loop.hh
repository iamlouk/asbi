#ifndef EVENTS_LOOP_HH
#define EVENTS_LOOP_HH

#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <functional>
#include "utils.hh"
#include "../include/types.hh"

namespace asbi {
	class Context; // forward decl.
};

namespace asbi::evts {

	class Event {
	public:
		std::vector<Value> args;
		LambdaContainer* callback;
		bool taskDone;
		bool doCall;
	};

	using callback_t = std::function<void(std::vector<Value>*, bool done)>;
	using taskfn_t = std::function<void(callback_t)>;

	// inserted by something like readline() into the tasks queue
	// tasks shall be run in therie own thread and create events for the event queue
	class Task {
	public:
		Task(taskfn_t fn, LambdaContainer* callback):
			fn(fn), callback(callback) {}

		taskfn_t fn;
		LambdaContainer* callback;
	};


	class Loop {
		// TODO: use c++ friend
	private:
		Context* ctx;

		unsigned int tasksRunning;
		std::mutex tasksMtx;
		Semaphore tasksSem;
		std::deque<Task*> tasks;

		std::mutex eventsMtx; // alway lock bevore tasksMtx
		Semaphore eventsSem;
		std::deque<Event*> events;

		std::vector<std::thread> threads;
		void newThread();

		void addEvent(Event*); // called by workers
		Event* getEvent(); // called by mainthread only (must know if there are running or left tasks)
		void addTask(Task*); // called by manthread only (must be thread save against workers)
	public:
		Loop(Context* ctx, unsigned int start_threads);
		~Loop();

		void start();

		void addTask(LambdaContainer*, taskfn_t);
		Task* getTask(); // called by workers

		void addEvent(Task*, std::vector<Value>*, bool done);
	};

}

#endif
