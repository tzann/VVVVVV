#ifndef SOLVER_GEOMETRY_H
#define SOLVER_GEOMETRY_H

#include <limits>

#include <SDL.h>

#include "solver/Numerics.h"

namespace Geometry {
	static int saturatingNegate(int x) {
		switch (x) {
			case INT_MAX:
				return INT_MIN;
			case INT_MIN:
			case -INT_MAX:
				return INT_MAX;
			default:
				return -x;
		}
	}
	static int saturatingAdd(int x, int y) {
		if (x == 0) {
			return y;
		} else if (y == 0) {
			return x;
		} else if (x == INT_MAX || x == INT_MIN) {
			return x;
		} else if (y == INT_MAX || y == INT_MIN) {
			return y;
		}

		if (y > 0 && x + y <= x) {
			return INT_MAX;
		} else if (y < 0 && x + y >= x) {
			return INT_MIN;
		}

		return x + y;
	}
	static int saturatingSub(int x, int y) {
		return saturatingAdd(x, saturatingNegate(y));
	}
	static int saturatingMul(int x, int y) {
		if (x == 0 || y == 0) {
			return 0;
		}
		// Enforce x and y to be positive
		if (x < 0) {
			return saturatingMul(saturatingNegate(x), saturatingNegate(y));
		} else if (y < 0) {
			return saturatingNegate(saturatingMul(x, saturatingNegate(y)));
		}

		if (x == INT_MAX || y == INT_MAX) {
			return INT_MAX;
		}

		// Assume no overflow for now
		return x * y;
	}
	static int saturatingDiv(int x, int y) {
		// Enforce x and y to be positive
		if (x < 0) {
			return saturatingDiv(saturatingNegate(x), saturatingNegate(y));
		} else if (y < 0) {
			return saturatingNegate(saturatingDiv(x, saturatingNegate(y)));
		}

		if (x == 0) {
			// We define 0 / 0 = 0
			return 0;
		} else if (y == 0) {
			return INT_MAX;
		} else if (x == INT_MAX) {
			return INT_MAX;
		} else if (y == INT_MAX) {
			return 0;
		}

		return x / y;
	}

	struct IntVector {
		int x, y;

		IntVector() {
			x = 0;
			y = 0;
		}

		IntVector(int x, int y) : x(x), y(y) { }

		bool operator== (const IntVector& other) const {
			return (x == other.x && y == other.y);
		}
		bool operator!= (const IntVector& other) const {
			return !(*this == other);
		}
		bool operator< (const IntVector& other) const {
			if (y != other.y) {
				return y < other.y;
			}
			else if (x != other.x) {
				return x < other.x;
			}
			// Equality
			return false;
		}
		bool operator<=(const IntVector& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const IntVector& other) const {
			return !(*this < other);
		}
		bool operator> (const IntVector& other) const {
			return !(*this <= other);
		}

		IntVector operator-(void) const {
			return IntVector(-x, -y);
		}
		IntVector& operator+=(const IntVector& rhs) {
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		IntVector operator+(const IntVector& other) const {
			return IntVector(x + other.x, y + other.y);
		}
		IntVector& operator-=(const IntVector& rhs) {
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		IntVector operator-(const IntVector& other) const {
			return IntVector(x - other.x, y - other.y);
		}
		IntVector& operator*=(const IntVector& rhs) {
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}
		IntVector operator*(const IntVector& other) const {
			return IntVector(x * other.x, y * other.y);
		}
		IntVector& operator/=(const IntVector& rhs) {
			x /= rhs.x;
			y /= rhs.y;
			return *this;
		}
		IntVector operator/(const IntVector& other) const {
			return IntVector(x / other.x, y / other.y);
		}

		IntVector& operator*=(const int rhs) {
			x *= rhs;
			y *= rhs;
			return *this;
		}
		IntVector operator*(const int val) const {
			return IntVector(x * val, y * val);
		}
		IntVector& operator/=(const int rhs) {
			x /= rhs;
			y /= rhs;
			return *this;
		}
		IntVector operator/(const int val) const {
			return IntVector(x / val, y / val);
		}

		int length_squared(void) {
			return x * x + y * y;
		}
		int dot_product(const IntVector& other) {
			return x * other.x + y * other.y;
		}
		int cross_product(const IntVector& other) {
			return x * other.y - y * other.x;
		}

		static IntVector zero(void) {
			return IntVector(0, 0);
		}
	};

	struct IntInterval {
		int min, max;

		IntInterval& make_bottom(void) {
			min = INT_MAX;
			max = INT_MIN;
			return *this;
		}
		IntInterval& make_top(void) {
			min = INT_MIN;
			max = INT_MAX;
			return *this;
		}
		IntInterval& regularize(void) {
			if (this->is_bottom()) {
				return this->make_bottom();
			} else {
				return *this;
			}
		}

		// Default constructor returns top
		IntInterval() {
			make_top();
			regularize();
		}
		IntInterval(int val) : min(val), max(val) {
			regularize();
		}
		IntInterval(int min, int max) : min(min), max(max) {
			regularize();
		}

		bool is_bottom(void) const {
			return max < min;
		}
		bool is_top(void) const {
			return min == INT_MIN && max == INT_MAX;
		}
		bool has_lower_bound(void) const {
			return min > INT_MIN;
		}
		bool has_upper_bound(void) const {
			return max < INT_MAX;
		}
		bool is_exact(void) const {
			return min == max && min > INT_MIN && max < INT_MAX;
		}
		bool is_positive(void) const {
			return min >= 0 && max >= min;
		}
		bool is_negative(void) const {
			return min <= max && max <= 0;
		}
		bool intersects(const IntInterval& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}

			if (min > other.max || max < other.min) {
				return false;
			}
			return true;
		}

		bool contains(int val) const;
		bool contains(const IntInterval& other) const;

		int getUpperBound(void) const {
			return max;
		}
		int getLowerBound(void) const {
			return min;
		}

		IntInterval& negate(void) {
			if (!this->is_bottom()) {
				int oldMax = max;
				max = saturatingNegate(min);
				min = saturatingNegate(oldMax);
			}
			return regularize();
		}
		IntInterval operator-(void) const {
			if (this->is_bottom()) {
				return IntInterval::bottom();
			}
			return IntInterval(saturatingNegate(max), saturatingNegate(min));
		}

		IntInterval& operator+=(const IntInterval& rhs) {
			if (rhs.is_bottom() || this->is_bottom()) {
				// Propagate bottom
				make_bottom();
			} else {
				min = saturatingAdd(min, rhs.min);
				max = saturatingAdd(max, rhs.max);
			}
			return regularize();
		}
		IntInterval operator+=(const int val) {
			return this->operator+=(IntInterval(val));
		}
		IntInterval operator+(const IntInterval& other) const {
			IntInterval result(*this);
			result += other;
			return result;
		}
		IntInterval operator+(const int val) const {
			return this->operator+(IntInterval(val));
		}

		IntInterval& operator-=(const IntInterval& rhs) {
			return this->operator+=(-rhs);
		}
		IntInterval& operator-=(const int rhs) {
			return this->operator-=(IntInterval(rhs));
		}
		IntInterval operator-(const IntInterval& other) const {
			return this->operator+(-other);
		}
		IntInterval operator-(const int other) const {
			return this->operator-(IntInterval(other));
		}

		IntInterval& operator*=(int val) {
			if (!is_bottom()) {
				if (val < 0) {
					negate();
					val = -val;
				}
				min = saturatingMul(min, val);
				max = saturatingMul(max, val);
			}
			return regularize();
		}
		IntInterval operator*(const int val) const {
			IntInterval result(*this);
			result *= val;
			return result;
		}
		IntInterval& operator*=(const IntInterval& rhs) {
			if (rhs.is_bottom() || this->is_bottom()) {
				// Propagate bottom
				make_bottom();
			} else {
				int r1 = saturatingMul(min, rhs.min);
				int r2 = saturatingMul(min, rhs.max);
				int r3 = saturatingMul(max, rhs.min);
				int r4 = saturatingMul(max, rhs.max);

				min = SDL_min(SDL_min(r1, r2), SDL_min(r3, r4));
				max = SDL_max(SDL_max(r1, r2), SDL_max(r3, r4));
			}
			return regularize();
		}
		IntInterval operator*(const IntInterval& other) const {
			if (this->is_bottom() || other.is_bottom()) {
				return IntInterval::bottom();
			}
			IntInterval result(*this);
			result *= other;
			return result;
		}

		bool operator>(const int val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_lower_bound()) {
				return min > val;
			} else {
				return false;
			}
		}
		bool operator>=(const int val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_lower_bound()) {
				return min >= val;
			}
			else {
				return false;
			}
		}
		bool operator<(const int val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_upper_bound()) {
				return max < val;
			}
			else {
				return false;
			}
		}
		bool operator<=(const int val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_upper_bound()) {
				return max <= val;
			} else {
				return false;
			}
		}

		bool operator>(const IntInterval& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}

			if (has_lower_bound() && other.has_upper_bound()) {
				return min > other.max;
			} else {
				return false;
			}
		}
		bool operator>=(const IntInterval& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}

			if (has_lower_bound() && other.has_upper_bound()) {
				return min >= other.max;
			} else {
				return false;
			}
		}
		bool operator<(const IntInterval& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}

			if (has_upper_bound() && other.has_lower_bound()) {
				return max < other.min;
			} else {
				return false;
			}
		}
		bool operator<=(const IntInterval& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}

			if (has_upper_bound() && other.has_lower_bound()) {
				return max <= other.min;
			} else {
				return false;
			}
		}

		IntInterval abs(void) const {
			if (this->is_bottom()) {
				return IntInterval::bottom();
			}

			if (max <= 0) {
				return -(*this);
			}
			if (min >= 0) {
				return IntInterval(*this);
			}

			return IntInterval(0, SDL_max(saturatingNegate(min), max));
		}
		IntInterval& clamp(IntInterval interval) {
			if (this->is_bottom() || interval.is_bottom()) {
				make_bottom();
			} else {
				// Everything less than interval.min is set to interval.min
				// Everything greater than interval.max is set to interval.max
				// This is different from regular intersection!
				min = SDL_clamp(min, interval.min, interval.max);
				max = SDL_clamp(max, interval.min, interval.max);
			}
			return regularize();
		}

		IntInterval& addUpperBound(int limit) {
			max = SDL_min(max, limit);
			return regularize();
		}
		IntInterval& addLowerBound(int limit) {
			min = SDL_max(min, limit);
			return regularize();
		}
		IntInterval& removeUpperBound(void) {
			if (!is_bottom()) {
				max = INT_MAX;
			}
			return regularize();
		}
		IntInterval& removeLowerBound(void) {
			if (!is_bottom()) {
				min = -INT_MAX;
			}
			return regularize();
		}
		IntInterval& join(const IntInterval& other);
		IntInterval& intersect(const IntInterval& other);
		IntInterval& difference(const IntInterval& other);

		IntInterval getIntervalAbove(void) const {
			if (has_upper_bound()) {
				return IntInterval::fromLowerBound(max + 1);
			} else {
				return IntInterval::bottom();
			}
		}
		IntInterval getIntervalBelow(void) const {
			if (has_lower_bound()) {
				return IntInterval::fromUpperBound(min - 1);
			} else {
				return IntInterval::bottom();
			}
		}

		static IntInterval bottom(void) {
			return IntInterval(INT_MAX, INT_MIN);
		}
		static IntInterval top(void) {
			return IntInterval(INT_MIN, INT_MAX);
		}
		static IntInterval fromLowerBound(int min) {
			return IntInterval(min, INT_MAX);
		}
		static IntInterval fromUpperBound(int max) {
			return IntInterval(INT_MIN, max);
		}
		static IntInterval positive(void) {
			return fromLowerBound(0);
		}
		static IntInterval negative(void) {
			return fromUpperBound(0);
		}
		static IntInterval join(const IntInterval& a, const IntInterval& b);
		static IntInterval intersect(const IntInterval& a, const IntInterval& b);
		static IntInterval pos_div(const IntInterval& a, const IntInterval& b);
	};



	struct Region {
		IntInterval x;
		IntInterval y;

		Region& make_bottom(void) {
			x.make_bottom();
			y.make_bottom();
			return *this;
		}
		Region& regularize(void) {
			if (this->is_bottom()) {
				return this->make_bottom();
			} else {
				return *this;
			}
		}

		// Default constructor returns top
		Region() : x(), y() {
			regularize();
		}
		Region(const IntInterval& x, const IntInterval& y) : x(x), y(y) {
			regularize();
		}
		Region(const IntVector& from, const IntVector& to): x(SDL_min(from.x, to.x), SDL_max(from.x, to.x)), y(SDL_min(from.y, to.y), SDL_max(from.y, to.y)) {
			regularize();
		}

		bool is_bottom(void) const {
			return x.is_bottom() || y.is_bottom();
		}
		bool is_top(void) const {
			return x.is_top() && y.is_top();
		}
		bool is_exact(void) const {
			return x.is_exact() && y.is_exact();
		}
		bool is_bounded(void) const {
			return x.has_lower_bound() && x.has_upper_bound() && y.has_lower_bound() && y.has_upper_bound();
		}
		bool intersects(const Region& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}

			return x.intersects(other.x) && y.intersects(other.y);
		}

		bool contains(const Region& other) const;
		bool contains(const IntVector& val) const;
		bool contains(int x, int y) const;

		IntVector getMin(void) const {
			return IntVector(x.min, y.min);
		}
		IntVector getMax(void) const {
			return IntVector(x.max, y.max);
		}

		Region& addXUpperBound(int limit) {
			x.addUpperBound(limit);
			return regularize();
		}
		Region& addXLowerBound(int limit) {
			x.addLowerBound(limit);
			return regularize();
		}
		Region& addYUpperBound(int limit) {
			y.addUpperBound(limit);
			return regularize();
		}
		Region& addYLowerBound(int limit) {
			y.addLowerBound(limit);
			return regularize();
		}
		Region& removeXUpperBound(void) {
			x.removeUpperBound();
			return regularize();
		}
		Region& removeXLowerBound(void) {
			x.removeUpperBound();
			return regularize();
		}
		Region& removeYUpperBound(void) {
			y.removeUpperBound();
			return regularize();
		}
		Region& removeYLowerBound(void) {
			y.removeLowerBound();
			return regularize();
		}
		Region& join(const Region& other);
		Region& intersect(const Region& other);
		Region& difference(const Region& other);

		static Region bottom(void) {
			return Region(IntInterval::bottom(), IntInterval::bottom());
		}
		static Region top(void) {
			return Region(IntInterval::top(), IntInterval::top());
		}
		static Region fromXInterval(const IntInterval& x) {
			return Region(x, IntInterval::top());
		}
		static Region fromYInterval(const IntInterval& y) {
			return Region(IntInterval::top(), y);
		}
		static Region fromXLowerBound(int x_min) {
			return Region::fromXInterval(IntInterval::fromLowerBound(x_min));
		}
		static Region fromXUpperBound(int x_max) {
			return Region::fromXInterval(IntInterval::fromUpperBound(x_max));
		}
		static Region fromYLowerBound(int y_min) {
			return Region::fromYInterval(IntInterval::fromLowerBound(y_min));
		}
		static Region fromYUpperBound(int y_max) {
			return Region::fromYInterval(IntInterval::fromUpperBound(y_max));
		}
		static Region join(const Region& a, const Region& b);
		static Region intersect(const Region& a, const Region& b);
	};



	struct FloatInterval {
		float min, max;

		FloatInterval& make_bottom(void) {
			min = INFINITY;
			max = -INFINITY;
			return *this;
		}
		FloatInterval& make_top(void) {
			min = -INFINITY;
			max = INFINITY;
			return *this;
		}
		FloatInterval& make_exact(float val) {
			min = val;
			max = val;
			return regularize();
		}
		FloatInterval& regularize(void) {
			if (this->is_bottom()) {
				return this->make_bottom();
			}
			else {
				return *this;
			}
		}

		FloatInterval() : min(-INFINITY), max(INFINITY) {
			regularize();
		}
		FloatInterval(float val) : min(val), max(val) {
			regularize();
		}
		FloatInterval(float min, float max) : min(min), max(max) {
			regularize();
		}

		IntInterval toIntInterval(void) const {
			if (is_bottom()) {
				return IntInterval::bottom();
			} else if (is_top()) {
				return IntInterval::top();
			}

			// Most conservative rounding
			int intMin = has_lower_bound() ? ((int)SDL_floorf(min)) : (INT_MIN);
			int intMax = has_upper_bound() ? ((int)SDL_ceilf(max)) : (INT_MAX);

			return IntInterval(intMin, intMax);
		}

		bool is_bottom(void) const {
			return max < min || _isnan(max) || _isnan(min);
		}
		bool is_top(void) const {
			return min == -INFINITY && max == INFINITY;
		}
		bool has_lower_bound(void) const {
			return !is_bottom() && min > -INFINITY;
		}
		bool has_upper_bound(void) const {
			return !is_bottom() && max < INFINITY;
		}
		bool is_exact(void) const {
			return min == max && has_lower_bound() && has_upper_bound();
		}
		bool is_positive(void) const {
			return !is_bottom() && min >= 0 && max >= min;
		}
		bool is_negative(void) const {
			return !is_bottom() && min <= max && max <= 0;
		}
		bool intersects(const FloatInterval& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}

			if (min > other.max || max < other.min) {
				return false;
			}
			return true;
		}

		FloatInterval& join(const FloatInterval& other);
		FloatInterval& intersect(const FloatInterval& other);

		FloatInterval operator-(void) const {
			if (this->is_bottom()) {
				return FloatInterval::bottom();
			}
			return FloatInterval(-max, -min);
		}
		FloatInterval operator+(const FloatInterval& rhs) const {
			if (rhs.is_bottom() || this->is_bottom()) {
				// Propagate bottom
				return FloatInterval::bottom();
			} else {
				return FloatInterval(min + rhs.min, max + rhs.max);
			}
		}
		FloatInterval operator+(const float val) const {
			return this->operator+(FloatInterval(val));
		}
		FloatInterval& operator+=(const FloatInterval& rhs) {
			if (rhs.is_bottom() || this->is_bottom()) {
				// Propagate bottom
				make_bottom();
			} else {
				min += rhs.min;
				max += rhs.max;
			}
			return regularize();
		}
		FloatInterval& operator+=(const float val) {
			return this->operator+=(FloatInterval(val));
		}
		FloatInterval operator-(const FloatInterval& rhs) const {
			if (rhs.is_bottom() || this->is_bottom()) {
				// Propagate bottom
				return FloatInterval::bottom();
			} else {
				return FloatInterval(min - rhs.max, max - rhs.min);
			}
		}
		FloatInterval operator-(const float val) const {
			return this->operator-(FloatInterval(val));
		}
		FloatInterval& operator-=(const FloatInterval& rhs) {
			if (rhs.is_bottom() || this->is_bottom()) {
				// Propagate bottom
				make_bottom();
			} else {
				min -= rhs.min;
				max -= rhs.max;
			}
			return regularize();
		}
		FloatInterval& operator-=(const float val) {
			return this->operator-=(FloatInterval(val));
		}


		bool operator>(const float val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_lower_bound()) {
				return min > val;
			} else {
				return false;
			}
		}
		bool operator>=(const float val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_lower_bound()) {
				return min >= val;
			}
			else {
				return false;
			}
		}
		bool operator<(const float val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_upper_bound()) {
				return max < val;
			} else {
				return false;
			}
		}
		bool operator<=(const float val) const {
			if (is_bottom()) {
				return false;
			}

			if (has_upper_bound()) {
				return max <= val;
			} else {
				return false;
			}
		}

		float getUpperBound(void) const {
			if (is_bottom()) {
				return -INFINITY;
			} else if (!has_upper_bound()) {
				return INFINITY;
			} else {
				return max;
			}
		}
		float getLowerBound(void) const {
			if (is_bottom()) {
				return INFINITY;
			} else if (!has_lower_bound()) {
				return -INFINITY;
			} else {
				return min;
			}
		}

		FloatInterval abs(void) const {
			if (this->is_bottom()) {
				return FloatInterval::bottom();
			} else if (max <= 0) {
				return -(*this);
			} else if (min >= 0) {
				return FloatInterval(*this);
			}

			return FloatInterval(0, SDL_max(-min, max));
		}
		FloatInterval positivePart(void) const {
			FloatInterval result = FloatInterval::positive();
			result.intersect(*this);
			return result;
		}
		FloatInterval negativePart(void) const {
			FloatInterval result = FloatInterval::negative();
			result.intersect(*this);
			return result;
		}
		void splitAt(FloatInterval& above, FloatInterval& below, float bound, bool boundIsIncludedAbove) const {
			above.make_top();
			above.addUpperBound(getUpperBound());
			above.addLowerBound(boundIsIncludedAbove ? bound : Numerics::next_float_above(bound));

			below.make_top();
			below.addUpperBound(boundIsIncludedAbove ? Numerics::next_float_below(bound) : bound);
			below.addLowerBound(getLowerBound());
		}

		FloatInterval getIntervalBelow(void) const {
			if (is_bottom() || !has_lower_bound()) {
				return FloatInterval::bottom();
			} else {
				return FloatInterval::fromUpperBound(Numerics::next_float_below(getLowerBound()));
			}
		}
		FloatInterval getIntervalAbove(void) const {
			if (is_bottom() || !has_upper_bound()) {
				return FloatInterval::bottom();
			} else {
				return FloatInterval::fromLowerBound(Numerics::next_float_above(getUpperBound()));
			}
		}


		FloatInterval& clamp(FloatInterval interval) {
			if (this->is_bottom() || interval.is_bottom()) {
				make_bottom();
			} else {
				// Everything less than interval.min is set to interval.min
				// Everything greater than interval.max is set to interval.max
				// This is different from regular intersection!
				min = SDL_clamp(min, interval.min, interval.max);
				max = SDL_clamp(max, interval.min, interval.max);
			}
			return regularize();
		}

		FloatInterval& addUpperBound(float limit) {
			if (!is_bottom()) {
				max = SDL_min(max, limit);
			}
			return regularize();
		}
		FloatInterval& addLowerBound(float limit) {
			if (!is_bottom()) {
				min = SDL_max(min, limit);
			}
			return regularize();
		}

		static FloatInterval bottom(void) {
			return FloatInterval(NAN);
		}
		static FloatInterval top(void) {
			return FloatInterval();
		}
		static FloatInterval fromLowerBound(float min) {
			return FloatInterval(min, INFINITY);
		}
		static FloatInterval fromUpperBound(float max) {
			return FloatInterval(-INFINITY, max);
		}
		static FloatInterval positive(void) {
			return fromLowerBound(0.0f);
		}
		static FloatInterval negative(void) {
			return fromUpperBound(0.0f);
		}

		static FloatInterval join(const FloatInterval& a, const FloatInterval& b);
		static FloatInterval intersect(const FloatInterval& a, const FloatInterval& b);
		static FloatInterval fromIntInterval(const IntInterval& a);
	};
}

#endif /* SOLVER_GEOMETRY_H */