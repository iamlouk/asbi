#ifndef PROCENV_HH
#define PROCENV_HH

#include "context.hh"

namespace asbi {

	// arguments ignored by this interpreter and ment for asbi: [scriptArgs:argc]
	void load_procenv(Context*, int argc, const char* argv[], int scriptArgs);

}

#endif
