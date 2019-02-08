#ifndef CONTEXT_HH
#define CONTEXT_HH

#include <unordered_map>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <memory>
#include "mem.hh"
#include "types.hh"
#include "../events/loop.hh"

namespace asbi {
	enum OpCode: uint64_t; // forward decl.

	// TODO: gc_reset() um gc_inuse zurueck zu setzten bzw. gc_visit() gc_inuse setzten lassen
	class Env {
		friend Context;
		friend Value;
		friend Value execute(std::vector<OpCode>, std::shared_ptr<Env>, Context*);
	public:
		Env(Context*, std::shared_ptr<Env>);
		~Env();
		Value lookup(StringContainer*);
		Value lookup(Context*, const char*);
		void decl(StringContainer*, Value);
		void decl(Context*, const char*, Value);
		void set(StringContainer*, Value);
		void gc_visit() const;
		Value to_map(Context*) const;
	private:
		std::shared_ptr<Env> outer;
		std::shared_ptr<Env> caller;
		std::unordered_map<
			StringContainer*,
			Value,
			StringContainer::hashStructPointer,
			StringContainer::equalStructPointer
		> vars;

		mutable bool gc_visited = false;
		void gc_unvisit() const;
	public:
		std::shared_ptr<Env> getOuter() { return outer; }
	};

	class Context {
		friend GCObj;
		friend Value execute(std::vector<OpCode>, std::shared_ptr<Env>, Context*);
	private:
		// std::vector<StringContainer*> stringconstants; // TODO: vector durch map ersetzen?
		std::vector<StringContainer*> strconsts[32];

		void load_macros();

		GCObj* heap_head = nullptr;
		std::size_t heap_size = 0, heap_max = 128 * 2 * 2;
	public:
		Context();
		~Context();

		Value run(const std::string&);
		Value run(const std::string&, std::shared_ptr<Env>);

		evts::Loop evtloop;
		std::shared_ptr<Env> global_env;

		std::vector<Value> stack;
		void push(Value);
		Value pop();

		std::vector<LambdaContainer*> lambdas; // nur da um die opcodes aller lambdas zu speichern, nicht die lambdas selbst

		struct {
			StringContainer* __file;
			StringContainer* __main;
			StringContainer* __imports;
			StringContainer* exports;
			StringContainer* ok;
			StringContainer* err;
		} names;
		StringContainer* new_stringconstant(const char*);
		StringContainer* new_stringconstant(std::string&);
		StringContainer* new_string(const char*);
		StringContainer* new_string(std::string&);

		void check_gc(std::shared_ptr<Env>);
		void gc(std::shared_ptr<Env>);
	};

}

#endif
