#include <iostream>
#include <cassert>
#include "include/mem.hh"
#include "include/context.hh"

using namespace asbi;

asbi::GCObj::GCObj(Context* ctx, bool gc){
	gc_inuse = false;
	gc_manage = gc;
	if (gc_manage) {
		this->gc_next = ctx->heap_head;
		ctx->heap_head = this;
	}
}

asbi::GCObj::~GCObj(){
	assert(!gc_inuse || !gc_manage);
}

void asbi::Context::check_gc(std::shared_ptr<Env> env){
	// TODO: heap_size und heap_max
	if (heap_size >= heap_max) {
		gc(env);
		heap_max = heap_size * 2;
	}
}

void asbi::Context::gc(std::shared_ptr<Env> env){
	// if (env != nullptr) std::cerr << "gc..." << '\n';

	if (env != nullptr)
		env->gc_visit();

	for (auto elm: stack)
		elm.gc_visit();

	auto p = &heap_head;
	while (*p != nullptr) {
		auto obj = *p;
		// assert(obj->gc_manage); //
		if (!obj->gc_inuse && obj->gc_manage) {
			*p = obj->gc_next;
			heap_size -= obj->gc_size();
			delete obj;
		} else {
			obj->gc_inuse = false;
			p = &obj->gc_next;
		}
	}

	if (env != nullptr)
		env->gc_unvisit();
}
