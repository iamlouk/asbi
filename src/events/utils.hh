#ifndef EVENTS_UTILS_HH
#define EVENTS_UTILS_HH

#include <unistd.h>
#include <cassert>
#include <mutex>
#include <condition_variable>

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
	};

	template <typename T> class BlockingLinkedList {
	private:
		struct Node {
			Node(T value, Node* next): value(value), next(next) {}
			T value;
			Node* next;
		};

		std::mutex mut;
		Node* head = nullptr;
		Node* last = nullptr;

		Semaphore sem;

	public:
		BlockingLinkedList(): sem(0) {}

		void enqueue(T value) {
			{
				std::unique_lock<std::mutex> lock(mut);
				if (last == nullptr) {
					assert(head == nullptr);
					last = head = new Node(value, nullptr);
				} else {
					assert(head != nullptr);
					last = (last->next = new Node(value, nullptr));
				}
			}
			sem.release();
		}

		T dequeue() {
			sem.acquire();
			std::unique_lock<std::mutex> lock(mut);
			assert(head != nullptr && last != nullptr);
			assert(last->next == nullptr);

			auto node = head;
			if (last == head)
				last = nullptr;

			head = head->next;

			T value = node->value;
			delete node;
			return value;
		}

		// returns false if empty or true and writes to &val if not empty
		bool tryDequeue(T& val) {
			std::unique_lock<std::mutex> lock(mut);
			if (head != nullptr) {
				assert(last != nullptr && last->next == nullptr);

				auto node = head;
				if (last == head)
					last = nullptr;

				head = head->next;

				val = node->value;
				delete node;
				return true;
			}
			return false;
		}

		bool empty() {
			std::unique_lock<std::mutex> lock(mut);
			return head == nullptr;
		}

	};

	void to_usecs(double in, useconds_t &out);

}

#endif
