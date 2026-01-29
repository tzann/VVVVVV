#include "solver/Geometry.h"

namespace Geometry {
	IntInterval IntInterval::join(const IntInterval& a, const IntInterval& b) {
		IntInterval res(a);
		res.join(b);
		return res;
	}
	IntInterval IntInterval::intersect(const IntInterval& a, const IntInterval& b) {
		IntInterval res(a);
		res.intersect(b);
		return res;
	}
	IntInterval IntInterval::difference(const IntInterval& a, const IntInterval& b) {
		IntInterval res(a);
		res.difference(b);
		return res;
	}
	IntInterval IntInterval::inverse(const IntInterval& a) {
		IntInterval res(a);
		res.invert();
		return res;
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
		if (!other.has_lower_bound() || other.contains(min)) {
			addLowerBound(saturatingAdd(other.max, 1));
		}
		if (!other.has_upper_bound() || other.contains(max)) {
			addUpperBound(saturatingSub(other.min, 1));
		}

		return regularize();
	}
	IntInterval& IntInterval::invert() {
		int lb = INT_MIN;
		int ub = INT_MAX;
		if (!has_lower_bound()) {
			lb = saturatingAdd(max, 1);
		}
		if (!has_upper_bound()) {
			ub = saturatingSub(min, 1);
		}
		max = ub;
		min = lb;
		return regularize();
	}

	bool IntInterval::contains(int val) const {
		return val >= min && val <= max;
	}
	bool IntInterval::contains(const IntInterval& other) const {
		if (is_top() || other.is_bottom()) {
			return true;
		}
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

		float min = SDL_min(a.min, b.min);
		float max = SDL_max(a.max, b.max);
		return FloatInterval(min, max);
	}
	FloatInterval FloatInterval::intersect(const FloatInterval& a, const FloatInterval& b) {
		if (a.is_bottom() || b.is_bottom()) {
			return FloatInterval::bottom();
		}

		float min = SDL_max(a.min, b.min);
		float max = SDL_min(a.max, b.max);
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