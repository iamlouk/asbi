#ifndef MEM_HH
#define MEM_HH

#include <cstdlib>

namespace asbi {
	class Context; // forward decl.

	class GCObj {
	public:
		GCObj(Context*, bool);
		virtual ~GCObj();
		virtual void gc_visit() const = 0;
		virtual std::size_t gc_size() const = 0;

		GCObj* gc_next = nullptr;
		mutable bool gc_inuse = false, gc_manage = true;
	};

}

#endif
