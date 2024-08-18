#include "solver/Heuristic.h"

namespace Heuristic {
	int basic_heuristic(int dx, int dy) {
		int x_frames = (SDL_abs(dx) + MAX_X_SPEED - 1) / MAX_X_SPEED;
		int y_frames = (SDL_abs(dy) + MAX_Y_SPEED - 1) / MAX_Y_SPEED;
		return SDL_max(x_frames, y_frames);
	}
}