#ifndef TYPES_HH
#define TYPES_HH

#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "mem.hh"
#include "cstdint"

namespace asbi {
	class Context;         // forward decl.
	class Env;             // forward decl.
	class DictContainer;   // forward decl.
	enum OpCode: uint64_t; // forward decl.

	enum class type_t {
		Bool, Number, Nil, Symbol, String, Lambda, Dict, Macro, StackPlaceholder
	};

	class StringContainer: public GCObj {
		friend Context;
	private:
		StringContainer(std::string &data, std::size_t hash, Context*, bool gc);
		~StringContainer();
	public:
		struct hashStruct {
			std::size_t operator()(const StringContainer &sc) const { return sc.hash; }
		};
		struct equalStruct {
			bool operator()(const StringContainer &a, const StringContainer &b) const { return a.data == b.data; }
		};
		struct hashStructPointer {
			std::size_t operator()(StringContainer*const &sc) const { return sc->hash; }
		};
		struct equalStructPointer {
			bool operator()(StringContainer*const &a, StringContainer*const &b) const { return a->data == b->data; }
		};
		std::size_t hash;
		std::string data;

		void gc_visit() const override;
		std::size_t gc_size() const override;
	};

	class LambdaContainer: public GCObj {
	public:
		LambdaContainer(std::shared_ptr<Env>, std::vector<OpCode>*, std::vector<StringContainer*>, Context*, bool gc);
		~LambdaContainer();

		std::shared_ptr<Env> env;
		std::vector<OpCode>* ops;
		std::vector<StringContainer*> argnames;

		void gc_visit() const override;
		std::size_t gc_size() const override;
	};

	struct Value {
		using macro_t = Value(*)(int, Context*, std::shared_ptr<Env>);

		type_t type;
		union {
			bool _boolean;
			double _number;
			StringContainer* _string;
			LambdaContainer* _lambda;
			DictContainer* _dict;
			macro_t _macro;
		};

		Value(): type(type_t::Nil), _string(nullptr) {}

		static inline Value number(double num) {
			Value val;
			val.type = type_t::Number;
			val._number = num;
			return val;
		}

		static inline Value boolean(bool boolean) {
			Value val;
			val.type = type_t::Bool;
			val._boolean = boolean;
			return val;
		}

		static inline Value nil() {
			Value val;
			val.type = type_t::Nil;
			return val;
		}

		static inline Value symbol(StringContainer* sc) {
			Value val;
			val.type = type_t::Symbol;
			val._string = sc;
			return val;
		}

		static Value symbol(const char*, Context*);

		static inline Value string(StringContainer* sc) {
			Value val;
			val.type = type_t::String;
			val._string = sc;
			return val;
		}

		static Value string(const char*, Context*);

		static inline Value lambda(LambdaContainer* lc) {
			Value val;
			val.type = type_t::Lambda;
			val._lambda = lc;
			return val;
		}

		static inline Value dict(DictContainer* dc) {
			Value val;
			val.type = type_t::Dict;
			val._dict = dc;
			return val;
		}

		static inline Value macro(macro_t fnptr) {
			Value val;
			val.type = type_t::Macro;
			val._macro = fnptr;
			return val;
		}

		static inline Value stackplaceholder(StringContainer* sc) {
			Value val;
			val.type = type_t::StackPlaceholder;
			val._string = sc;
			return val;
		}

		struct valueHashStruct {
			std::size_t operator()(const Value &a) const { return a.hash(); }
		};
		struct valueEqualStruct {
			bool operator()(const Value &a, const Value &b) const { return a == b; }
		};

		bool operator==(const Value &rhs) const;
		std::size_t hash() const;
		std::string to_string(bool debug) const;
		void gc_visit() const;
		inline bool asUint(unsigned int*) const;
	};

	class DictContainer: public GCObj {
	public:
		std::unordered_map<
			Value,
			Value,
			Value::valueHashStruct,
			Value::valueEqualStruct
		> data;
		std::vector<Value> vecdata;

		DictContainer(Context*);

		void gc_visit() const override;
		std::size_t gc_size() const override;

		void set(Value key, Value val);
		Value get(Value key);
	};

}

#endif
