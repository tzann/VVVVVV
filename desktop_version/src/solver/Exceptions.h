#ifndef SOLVER_EXCEPTIONS_H
#define SOLVER_EXCEPTIONS_H

#include <SDL.h>

#include "Exit.h"

namespace Exceptions {
	static inline SDL_NORETURN void unimplemented(void) {
		VVV_exit(-1);
	}
	static inline SDL_NORETURN void todo(void) {
		VVV_exit(-1);
	}

	static inline SDL_NORETURN void error(void) {
		VVV_exit(1);
	}
	static inline SDL_NORETURN void invalid_argument(void) {
		VVV_exit(2);
	}
	static inline SDL_NORETURN void inadmissible_heuristic(void) {
		VVV_exit(10);
	}
	static inline void require(bool condition) {
		if (!condition) {
			invalid_argument();
		}
	}
	static inline void assert(bool condition) {
		if (!condition) {
			VVV_exit(3);
		}
	}
	static inline void unreachable(void) {
		assert(false);
	}
}

#endif /* SOLVER_EXCEPTIONS_H */