#ifndef EVENTS_UTILS_HH
#define EVENTS_UTILS_HH

#include <unistd.h>
#include <cassert>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>

namespace asbi::evts {

	class Semaphore {
	private:
		std::mutex mut;
		std::condition_variable condvar;
		int counter;
	public:
		explicit Semaphore(int counter):
			counter(counter) {}

		void release();
		void acquire();
		bool tryAcquire();

		// returns false if timed out
		bool acquireFor(std::chrono::duration<double, std::milli>);
	};

	// get current timestamp
	std::chrono::duration<double, std::milli> timestamp();

	// std::chrono::duration<double, std::milli> to double (in seconds)
	double to_double_seconds(std::chrono::duration<double, std::milli>);

	// double (in seconds) to std::chrono::duration<double, std::milli>
	std::chrono::duration<double, std::milli> to_chrono(double);


	template<typename T, typename Comp = std::less<T>> class SortedQueue {
	private:
		class Node {
		public:
			Node(T value, Node* next): value(value), next(next) {}
			T value;
			Node* next;
		};

		Node* head = nullptr;
	public:
		// SortedQueue();
		~SortedQueue() {
			for (Node* node = head; node != nullptr;) {
				auto tmp = node->next;
				delete node;
				node = tmp;
			}
		}

		void insert(T in) {
			Comp cmp = Comp();

			/*int s1 = size();
			Node **p = &head;
			while (*p != nullptr && cmp((*p)->value, in))
				p = &((*p)->next);

			if (*p == nullptr) {
				*p = node;
			} else {
			 	node->next = (*p)->next;
				*p = node;
			}
			assert(s1 + 1 == size());*/



			Node *drag = nullptr, *curr = head;
			while (curr != nullptr && cmp(curr->value, in)) {
				drag = curr;
				curr = curr->next;
			}

			if (drag == nullptr)
				head = new Node(in, head);
			else
				drag->next = new Node(in, curr);

		}

		T first() {
			assert(head != nullptr);
			return head->value;
		}

		void removeFirst() {
			assert(head != nullptr);
			auto tmp = head->next;
			delete head;
			head = tmp;
		}

		bool empty() { return head == nullptr; }

		unsigned int size() {
			unsigned int i = 0;
			for (Node* node = head; node != nullptr; node = node->next)
				i++;
			return i;
		}

	};

}

#endif
