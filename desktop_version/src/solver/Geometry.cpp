#include "solver/Geometry.h"

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
	IntInterval& IntInterval::difference(const IntInterval& other) {
		if (!is_bottom()) {
			if (other.contains(min)) {
				min = saturatingAdd(other.max, 1);
			}
			if (other.contains(max)) {
				max = saturatingSub(other.min, 1);
			}
		}
		return regularize();
	}

	bool IntInterval::contains(int val) const {
		return val >= min && val <= max;
	}
	bool IntInterval::contains(const IntInterval& other) const {
		return other.min >= min && other.max <= max;
	}

	IntInterval IntInterval::pos_div(const IntInterval& a, const IntInterval& b) {
		IntInterval t = IntInterval::positive();

		IntInterval pos_a = IntInterval::positive();
		IntInterval neg_a = IntInterval::negative();

		IntInterval pos_b = IntInterval::positive();
		IntInterval neg_b = IntInterval::negative();

		pos_a.intersect(a);
		neg_a.intersect(a);
		neg_a.negate();

		pos_b.intersect(b);
		neg_b.intersect(b);
		neg_b.negate();

		IntInterval pos_quotient = IntInterval::positive();
		pos_quotient.addUpperBound(saturatingDiv(pos_a.max, pos_b.min));
		pos_quotient.addLowerBound(saturatingDiv(pos_a.min, pos_b.max));

		IntInterval neg_quotient = IntInterval::positive();
		neg_quotient.addUpperBound(saturatingDiv(neg_a.max, neg_b.min));
		neg_quotient.addLowerBound(saturatingDiv(neg_a.min, neg_b.max));

		return IntInterval::join(pos_quotient, neg_quotient);
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
	Region& Region::difference(const Region& other) {
		if (!is_bottom()) {
			if (other.y.contains(y)) {
				x.difference(other.x);
			}
			if (other.x.contains(x)) {
				y.difference(other.y);
			}
		}
		return regularize();
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


	FloatInterval FloatInterval::join(const FloatInterval& a, const FloatInterval& b) {
		if (b.is_bottom()) {
			return FloatInterval(a);
		} else if (a.is_bottom()) {
			return FloatInterval(b);
		}

		int min = SDL_min(a.min, b.min);
		int max = SDL_max(a.max, b.max);
		return FloatInterval(min, max);
	}
	FloatInterval FloatInterval::intersect(const FloatInterval& a, const FloatInterval& b) {
		if (a.is_bottom() || b.is_bottom()) {
			return FloatInterval::bottom();
		}

		int min = SDL_max(a.min, b.min);
		int max = SDL_min(a.max, b.max);
		return FloatInterval(min, max);
	}
	FloatInterval& FloatInterval::join(const FloatInterval& other) {
		if (!other.is_bottom()) {
			if (is_bottom()) {
				min = other.min;
				max = other.max;
			} else {
				min = SDL_min(min, other.min);
				max = SDL_max(max, other.max);
			}
		}
		return regularize();
	}
	FloatInterval& FloatInterval::intersect(const FloatInterval& other) {
		if (is_bottom() || other.is_bottom()) {
			make_bottom();
		} else {
			min = SDL_max(min, other.min);
			max = SDL_min(max, other.max);
		}
		return regularize();
	}
	FloatInterval FloatInterval::fromIntInterval(const IntInterval& i) {
		if (i.is_bottom()) {
			return FloatInterval::bottom();
		} else if (i.is_top()) {
			return FloatInterval::top();
		} else if (!i.has_lower_bound()) {
			return FloatInterval::fromUpperBound((float) i.getUpperBound());
		} else if (!i.has_upper_bound()) {
			return FloatInterval::fromLowerBound((float) i.getLowerBound());
		} else {
			return FloatInterval((float) i.getLowerBound(), (float) i.getUpperBound());
		}
	}
}