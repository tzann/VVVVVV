#include "Geometry.h"

namespace Geometry {
	IntInterval IntInterval::join(const IntInterval& a, const IntInterval& b) {
		int min = SDL_min(a.min, b.min);
		int max = SDL_max(a.max, b.max);
		return IntInterval(min, max);
	}
	IntInterval IntInterval::intersect(const IntInterval& a, const IntInterval& b) {
		int min = SDL_max(a.min, b.min);
		int max = SDL_min(a.max, b.max);
		return IntInterval(min, max);
	}
	IntInterval& IntInterval::join(const IntInterval& other) {
		min = SDL_min(min, other.min);
		max = SDL_max(max, other.max);
		return regularize();
	}
	IntInterval& IntInterval::intersect(const IntInterval& other) {
		min = SDL_max(min, other.min);
		max = SDL_min(max, other.max);
		return regularize();
	}

	bool IntInterval::contains(int val) const {
		return val >= min && val <= max;
	}
	bool IntInterval::contains(const IntInterval& other) const {
		return other.min >= min && other.max <= max;
	}

	Region Region::join(const Region& a, const Region& b) {
		IntInterval x_interval = IntInterval::join(a.x, b.x);
		IntInterval y_interval = IntInterval::join(a.y, b.y);
		return Region(x_interval, y_interval);
	}
	Region Region::intersect(const Region& a, const Region& b) {
		IntInterval x_interval = IntInterval::intersect(a.x, b.x);
		IntInterval y_interval = IntInterval::intersect(a.y, b.y);
		return Region(x_interval, y_interval);
	}

	bool Region::contains(const Region& other) const {
		return x.contains(other.x) && y.contains(other.y);
	}
	bool Region::contains(const IntVector& val) const {
		return x.contains(val.x) && y.contains(val.y);
	}
	bool Region::contains(int x_val, int y_val) const {
		return x.contains(x_val) && y.contains(y_val);
	}

	Region& Region::join(const Region& other) {
		x.join(other.x);
		y.join(other.y);
		return regularize();
	}
	Region& Region::intersect(const Region& other) {
		x.intersect(other.x);
		y.intersect(other.y);
		return regularize();
	}

}