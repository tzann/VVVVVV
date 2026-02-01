#ifndef SOLVER_GEOMETRY_H
#define SOLVER_GEOMETRY_H

#include <climits>
#include <limits>
#include <type_traits>
#include <algorithm>

#include <SDL.h>

#include "solver/Numerics.h"

namespace Geometry {
	constexpr long long INT_MIN_LL = (long long)-INT_MAX;
	constexpr long long INT_MAX_LL = (long long)INT_MAX;
	inline static int sat_ll(long long val) {
		return (int) std::max(INT_MIN_LL, std::min(INT_MAX_LL, val));
	}

	inline static int div_ceil(int x, int y) {
		return (x + y - 1) / y;
	}
	inline static int div_floor(int x, int y) {
		return (x - 1) / y;
	}

	struct IntInterval {
		int min, max;

		IntInterval& make_bottom(void) {
			min = INT_MAX;
			max = -INT_MAX;
			return *this;
		}
		IntInterval& make_top(void) {
			min = -INT_MAX;
			max = INT_MAX;
			return *this;
		}

		// Default constructor returns top
		IntInterval() {
			make_top();
		}
		IntInterval(int val) : min(val), max(val) {}
		IntInterval(int min, int max) : min(min), max(max) {}

		inline bool is_bottom(void) const {
			return max < min;
		}
		inline bool is_top(void) const {
			return min <= -INT_MAX && max >= INT_MAX;
		}
		inline bool has_lower_bound(void) const {
			return min > -INT_MAX;
		}
		inline bool has_upper_bound(void) const {
			return max < INT_MAX;
		}
		inline bool is_exact(void) const {
			return min == max && min > -INT_MAX && max < INT_MAX;
		}
		inline bool is_positive(void) const {
			return min >= 0 && max >= min;
		}
		inline bool is_negative(void) const {
			return min <= max && max <= 0;
		}
		inline bool is_positive_nz(void) const {
			return min > 0 && max >= min;
		}
		inline bool is_negative_nz(void) const {
			return min <= max && max < 0;
		}
		inline bool is_bounded(void) const {
			return has_lower_bound() && has_upper_bound();
		}
		inline bool intersects(IntInterval other) const {
			return std::max(min, other.min) <= std::min(max, other.max);
		}

		inline bool contains(int val) const {
			return min <= val && val <= max;
		}
		inline bool contains(IntInterval other) const {
			return min <= other.min && other.min <= other.max && other.max <= max;
		}

		inline IntInterval& negate(void) {
			int oldMax = max;
			max = min <= -INT_MAX ? INT_MAX : -min;
			min = max >= INT_MAX ? -INT_MAX : -oldMax;
			return *this;
		}
		inline IntInterval operator-(void) const {
			IntInterval res(*this);
			res.negate();
			return res;
		}
		inline IntInterval negated(void) {
			return -(*this);
		}

		inline IntInterval& operator+=(IntInterval rhs) {
			min = min <= -INT_MAX ? -INT_MAX : sat_ll((long long)min + rhs.min);
			max = max >= INT_MAX ? INT_MAX : sat_ll((long long)max + rhs.max);
			return (*this);
		}
		inline IntInterval operator+=(const int val) {
			min = min <= -INT_MAX ? -INT_MAX : sat_ll((long long)min + val);
			max = max >= INT_MAX ? INT_MAX : sat_ll((long long)max + val);
			return (*this);
		}
		inline IntInterval operator+(IntInterval other) const {
			IntInterval result(*this);
			result += other;
			return result;
		}
		inline IntInterval operator+(const int val) const {
			IntInterval result(*this);
			result += val;
			return result;
		}

		inline IntInterval& operator-=(IntInterval rhs) {
			min = min <= -INT_MAX ? -INT_MAX : sat_ll((long long)min - rhs.max);
			max = max >= INT_MAX ? INT_MAX : sat_ll((long long)max - rhs.min);
			return (*this);
		}
		inline IntInterval& operator-=(const int val) {
			min = min <= -INT_MAX ? -INT_MAX : sat_ll((long long)min - val);
			max = max >= INT_MAX ? INT_MAX : sat_ll((long long)max - val);
			return (*this);
		}
		inline IntInterval operator-(IntInterval other) const {
			IntInterval result(*this);
			result -= other;
			return result;
		}
		inline IntInterval operator-(const int val) const {
			IntInterval result(*this);
			result -= val;
			return result;
		}

		inline IntInterval& operator*=(int val) {
			long long p1 = (long long)min * val;
			long long p2 = (long long)max * val;

			min = sat_ll(std::min(p1, p2));
			max = sat_ll(std::max(p1, p2));
			
			return (*this);
		}
		inline IntInterval operator*(const int val) const {
			IntInterval result(*this);
			result *= val;
			return result;
		}
		inline IntInterval& operator*=(IntInterval rhs) {
			long long p1 = (long long)min * rhs.min;
			long long p2 = (long long)min * rhs.max;
			long long p3 = (long long)max * rhs.min;
			long long p4 = (long long)max * rhs.max;

			min = sat_ll(std::min({ p1, p2, p3, p4 }));
			max = sat_ll(std::max({ p1, p2, p3, p4 }));
			return (*this);
		}
		inline IntInterval operator*(IntInterval other) const {
			IntInterval result(*this);
			result *= other;
			return result;
		}

		inline bool operator>(const int val) const {
			return val < min && min <= max;
		}
		inline bool operator>=(const int val) const {
			return val <= min && min <= max;
		}
		inline bool operator<(const int val) const {
			return min <= max && max < val;
		}
		inline bool operator<=(const int val) const {
			return min <= max && max <= val;
		}

		bool operator>(IntInterval other) const {
			return other.min <= other.max && other.max < min && min <= max;
		}
		bool operator>=(IntInterval other) const {
			return other.min <= other.max && other.max <= min && min <= max;
		}
		bool operator<(IntInterval other) const {
			return min <= max && max < other.min && other.min <= other.max;
		}
		bool operator<=(IntInterval other) const {
			return min <= max && max <= other.min && other.min <= other.max;
		}

		inline IntInterval abs(void) const {
			if (min > 0 || is_bottom()) {
				return IntInterval(*this);
			}
			else if (max < 0) {
				return -(*this);
			}
			else {
				return IntInterval(0, std::max((int)std::abs((long long)min), max));
			}
		}
		inline IntInterval& clamp(int cmin, int cmax) {
			if (this->is_bottom() || cmin > cmax) {
				make_bottom();
			} else {
				// Everything less than interval.min is set to interval.min
				// Everything greater than interval.max is set to interval.max
				// This is different from regular interval intersection!
				min = std::max(cmin, std::min(min, cmax));
				max = std::max(cmin, std::min(max, cmax));
			}
			return *this;
		}
		inline IntInterval& clampToInterval(IntInterval other) {
			if (this->is_bottom() || other.is_bottom()) {
				make_bottom();
			}
			else {
				// Everything less than interval.min is set to interval.min
				// Everything greater than interval.max is set to interval.max
				// This is different from regular interval intersection!
				min = std::max(other.min, std::min(min, other.max));
				max = std::max(other.min, std::min(max, other.max));
			}
			return *this;
		}

		inline IntInterval& addUpperBound(int limit) {
			max = std::min(max, limit);
			return (*this);
		}
		inline IntInterval& addLowerBound(int limit) {
			min = std::max(min, limit);
			return (*this);
		}
		inline IntInterval& removeUpperBound(void) {
			if (!is_bottom()) {
				max = INT_MAX;
			}
			return (*this);
		}
		inline IntInterval& removeLowerBound(void) {
			if (!is_bottom()) {
				min = INT_MIN;
			}
			return (*this);
		}

		inline IntInterval getIntervalAbove(void) const {
			if (is_bottom()) {
				return IntInterval::top();
			}
			else if (max >= INT_MAX - 1) {
				return IntInterval::bottom();
			}
			else {
				return IntInterval::fromLowerBound(max + 1);
			}
		}
		inline IntInterval getIntervalBelow(void) const {
			if (is_bottom()) {
				return IntInterval::top();
			} else if (min <= 1 - INT_MAX) {
				return IntInterval::bottom();
			} else {
				return IntInterval::fromUpperBound(min - 1);
			}
		}

		inline static IntInterval bottom(void) {
			return IntInterval(INT_MAX, INT_MIN);
		}
		inline static IntInterval top(void) {
			return IntInterval(INT_MIN, INT_MAX);
		}
		inline static IntInterval fromLowerBound(int min) {
			return IntInterval(min, INT_MAX);
		}
		inline static IntInterval fromUpperBound(int max) {
			return IntInterval(INT_MIN, max);
		}
		inline static IntInterval positive(void) {
			return fromLowerBound(0);
		}
		inline static IntInterval negative(void) {
			return fromUpperBound(0);
		}
		inline static IntInterval positive_nz(void) {
			return fromLowerBound(1);
		}
		inline static IntInterval negative_nz(void) {
			return fromUpperBound(-1);
		}


		inline IntInterval& join(IntInterval other) {
			if (other.is_bottom()) {
				return *this;
			}
			else if (is_bottom()) {
				min = other.min;
				max = other.max;
			}
			else {
				min = std::min(min, other.min);
				max = std::max(max, other.max);
			}
			return *this;
		}
		inline IntInterval& intersect(IntInterval other) {
			min = std::max(min, other.min);
			max = std::min(max, other.max);
			return *this;
		}
		inline IntInterval& difference(IntInterval other) {
			if (other.min <= std::max(min, -INT_MAX)) {
				min = std::max(min, sat_ll((long long) other.max + 1));
			}
			if (other.max >= std::min(max, INT_MAX)) {
				max = std::min(max, sat_ll((long long) other.min - 1));
			}
			return *this;
		}
		/// Essentially returns top().difference(this), i.e. an interval that contains at least everything not contained in this one
		inline IntInterval& invert() {
			int old_min = min;
			int old_max = max;
			if (is_top()) {
				make_bottom();
			} else if (!has_lower_bound()) {
				min = sat_ll((long long)old_max + 1);
				max = INT_MAX;
			} else if (!has_upper_bound()) {
				min = -INT_MAX;
				max = sat_ll((long long)old_min - 1);
			}
			else {
				make_top();
			}
			return *this;
		}

		inline static IntInterval join(IntInterval a, IntInterval b) {
			IntInterval res(a);
			res.join(b);
			return res;
		}
		inline static IntInterval intersect(IntInterval a, IntInterval b) {
			IntInterval res(a);
			res.intersect(b);
			return res;
		}
		inline static IntInterval difference(IntInterval a, IntInterval b) {
			IntInterval res(a);
			res.difference(b);
			return res;
		}
		inline static IntInterval inverse(IntInterval a) {
			IntInterval res(a);
			res.invert();
			return res;
		}

		inline static int min_abs_diff(IntInterval a, IntInterval b) {
			int dmin = a.min <= -INT_MAX ? -INT_MAX : sat_ll((long long)a.min - b.max);
			int dmax = a.max >= INT_MAX ? INT_MAX : sat_ll((long long) a.max - b.min);

			if (dmin > 0) {
				return dmin;
			}
			else if (dmax < 0) {
				return -dmax;
			}
			return 0;
		}

		inline static int min_diff_signed(IntInterval a, IntInterval b) {
			int dmin = a.min <= -INT_MAX ? -INT_MAX : sat_ll((long long)a.min - b.max);
			int dmax = a.max >= INT_MAX ? INT_MAX : sat_ll((long long)a.max - b.min);

			if (dmin > 0) {
				return dmin;
			}
			else if (dmax < 0) {
				return dmax;
			}
			return 0;
		}
	};


	struct IntVector {
		int x, y;

		IntVector() {
			x = 0;
			y = 0;
		}

		IntVector(int x, int y) : x(x), y(y) { }

		bool operator== (IntVector other) const {
			return (x == other.x && y == other.y);
		}
		bool operator!= (IntVector other) const {
			return !(*this == other);
		}
		bool operator< (IntVector other) const {
			if (y != other.y) {
				return y < other.y;
			}
			else if (x != other.x) {
				return x < other.x;
			}
			// Equality
			return false;
		}
		bool operator<=(IntVector other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(IntVector other) const {
			return !(*this < other);
		}
		bool operator> (IntVector other) const {
			return !(*this <= other);
		}

		IntVector operator-(void) const {
			return IntVector(-x, -y);
		}
		IntVector& operator+=(IntVector rhs) {
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		IntVector operator+(IntVector other) const {
			return IntVector(x + other.x, y + other.y);
		}
		IntVector& operator-=(IntVector rhs) {
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		IntVector operator-(IntVector other) const {
			return IntVector(x - other.x, y - other.y);
		}
		IntVector& operator*=(IntVector rhs) {
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}
		IntVector operator*(IntVector other) const {
			return IntVector(x * other.x, y * other.y);
		}
		IntVector& operator/=(IntVector rhs) {
			x /= rhs.x;
			y /= rhs.y;
			return *this;
		}
		IntVector operator/(IntVector other) const {
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
		int dot_product(IntVector other) {
			return x * other.x + y * other.y;
		}
		int cross_product(IntVector other) {
			return x * other.y - y * other.x;
		}

		static IntVector zero(void) {
			return IntVector(0, 0);
		}
	};

	struct Region {
		IntInterval x;
		IntInterval y;

		inline Region& make_bottom(void) {
			x.make_bottom();
			y.make_bottom();
			return *this;
		}
		inline Region& make_top(void) {
			x.make_top();
			y.make_top();
			return *this;
		}

		// Default constructor returns top
		Region() : x(), y() {}
		Region(IntInterval x, IntInterval y) : x(x), y(y) {}
		Region(IntVector point) : x(point.x), y(point.y) {}
		Region(IntVector from, IntVector to): x(SDL_min(from.x, to.x), SDL_max(from.x, to.x)), y(SDL_min(from.y, to.y), SDL_max(from.y, to.y)) {}

		inline bool is_bottom(void) const {
			return x.is_bottom() || y.is_bottom();
		}
		inline bool is_top(void) const {
			return x.is_top() && y.is_top();
		}
		inline bool is_exact(void) const {
			return x.is_exact() && y.is_exact();
		}
		inline bool is_bounded(void) const {
			return x.has_lower_bound() && x.has_upper_bound() && y.has_lower_bound() && y.has_upper_bound();
		}
		inline bool intersects(const Region& other) const {
			return x.intersects(other.x) && y.intersects(other.y);
		}

		inline bool contains(const Region& other) const {
			return x.contains(other.x) && y.contains(other.y);
		}
		inline bool contains(IntVector val) const {
			return x.contains(val.x) && y.contains(val.y);
		}
		inline bool contains(int x, int y) const {
			return this->x.contains(x) && this->y.contains(y);
		}

		inline bool exactly_equals(const Region& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}
			return x.min == other.x.min && x.max == other.x.max && y.min == other.y.min && y.max == other.y.max;
		}

		inline IntVector getMin(void) const {
			return IntVector(x.min, y.min);
		}
		inline IntVector getMax(void) const {
			return IntVector(x.max, y.max);
		}

		inline Region& addXUpperBound(int limit) {
			x.addUpperBound(limit);
			return *this;
		}
		inline Region& addXLowerBound(int limit) {
			x.addLowerBound(limit);
			return *this;
		}
		inline Region& addYUpperBound(int limit) {
			y.addUpperBound(limit);
			return *this;
		}
		inline Region& addYLowerBound(int limit) {
			y.addLowerBound(limit);
			return *this;
		}
		inline Region& removeXUpperBound(void) {
			x.removeUpperBound();
			return *this;
		}
		inline Region& removeXLowerBound(void) {
			x.removeLowerBound();
			return *this;
		}
		inline Region& removeYUpperBound(void) {
			y.removeUpperBound();
			return *this;
		}
		inline Region& removeYLowerBound(void) {
			y.removeLowerBound();
			return *this;
		}

		inline static Region bottom(void) {
			return Region(IntInterval::bottom(), IntInterval::bottom());
		}
		inline static Region top(void) {
			return Region(IntInterval::top(), IntInterval::top());
		}
		inline static Region fromXInterval(IntInterval x) {
			return Region(x, IntInterval::top());
		}
		inline static Region fromYInterval(IntInterval y) {
			return Region(IntInterval::top(), y);
		}
		inline static Region fromXLowerBound(int x_min) {
			return Region::fromXInterval(IntInterval::fromLowerBound(x_min));
		}
		inline static Region fromXUpperBound(int x_max) {
			return Region::fromXInterval(IntInterval::fromUpperBound(x_max));
		}
		inline static Region fromYLowerBound(int y_min) {
			return Region::fromYInterval(IntInterval::fromLowerBound(y_min));
		}
		inline static Region fromYUpperBound(int y_max) {
			return Region::fromYInterval(IntInterval::fromUpperBound(y_max));
		}
		
		inline Region& join(const Region& other) {
			x.join(other.x);
			y.join(other.y);
			return *this;
		}
		inline Region& intersect(const Region& other) {
			x.intersect(other.x);
			y.intersect(other.y);
			return *this;
		}
		inline Region& difference(const Region& other) {
			if (!is_bottom()) {
				if (other.y.contains(y)) {
					x.difference(other.x);
				}
				if (other.x.contains(x)) {
					y.difference(other.y);
				}
			}
			return *this;
		}

		inline static Region join(const Region& a, const Region& b) {
			Region res(a);
			res.join(b);
			return res;
		}
		inline static Region intersect(const Region& a, const Region& b) {
			Region res(a);
			res.intersect(b);
			return res;
		}
	};

	struct FloatInterval {
		float min, max;

		inline FloatInterval& make_bottom(void) {
			min = INFINITY;
			max = -INFINITY;
			return *this;
		}
		inline FloatInterval& make_top(void) {
			min = -INFINITY;
			max = INFINITY;
			return *this;
		}
		inline FloatInterval& make_exact(float val) {
			min = val;
			max = val;
			return *this;
		}

		FloatInterval() : min(-INFINITY), max(INFINITY) {}
		FloatInterval(float val) : min(val), max(val) {}
		FloatInterval(float min, float max) : min(min), max(max) {}

		inline IntInterval toIntInterval(void) const {
			if (is_bottom()) return IntInterval::bottom();

			static constexpr float MAX_F = (float)(INT_MAX - 1);
			static constexpr float MIN_F = (float)(1 - INT_MAX);

			// Most conservative rounding
			float f_min = SDL_max(min, MIN_F);
			float f_max = SDL_min(max, MAX_F);

			int i_min = (int)f_min;
			// If we truncated, subtract 1 to correct for it
			i_min -= (f_min < i_min);
			int i_max = (int)f_max;
			// If we truncated, add 1 to correct for it
			i_max += (f_max > i_max);

			return IntInterval(i_min, i_max);
		}

		inline bool is_bottom(void) const {
			return max < min;
		}
		inline bool is_top(void) const {
			return min == -INFINITY && max == INFINITY;
		}
		inline bool has_lower_bound(void) const {
			return min > -INFINITY;
		}
		inline bool has_upper_bound(void) const {
			return max < INFINITY;
		}
		inline bool is_exact(void) const {
			return min == max && has_lower_bound() && has_upper_bound();
		}
		inline bool is_positive(void) const {
			return min >= 0 && max >= min;
		}
		inline bool is_negative(void) const {
			return min <= max && max <= 0;
		}
		inline bool intersects(FloatInterval other) const {
			return SDL_max(min, other.min) <= SDL_min(max, other.max);
		}

		inline bool exactly_equals(FloatInterval other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			}
			return min == other.min && max == other.max;
		}

		inline FloatInterval operator-(void) const {
			return FloatInterval(-max, -min);
		}
		inline FloatInterval operator+(FloatInterval rhs) const {
			return FloatInterval(min + rhs.min, max + rhs.max);
		}
		inline FloatInterval operator+(const float val) const {
			return FloatInterval(min + val, max + val);
		}
		inline FloatInterval& operator+=(FloatInterval rhs) {
			min += rhs.min;
			max += rhs.max;
			return (*this);
		}
		inline FloatInterval& operator+=(const float val) {
			min += val;
			max += val;
			return (*this);
		}
		inline FloatInterval operator-(FloatInterval rhs) const {
			return FloatInterval(min - rhs.max, max - rhs.min);
		}
		inline FloatInterval operator-(const float val) const {
			return FloatInterval(min - val, max - val);
		}
		inline FloatInterval& operator-=(FloatInterval rhs) {
			min -= rhs.min;
			max -= rhs.max;
			return (*this);
		}
		inline FloatInterval& operator-=(const float val) {
			min -= val;
			max -= val;
			return (*this);
		}

		inline FloatInterval operator+(IntInterval rhs) const {
			return FloatInterval(min + rhs.min, max + rhs.max);
		}
		inline FloatInterval operator+(const int val) const {
			return FloatInterval(min + val, max + val);
		}
		inline FloatInterval& operator+=(IntInterval rhs) {
			min += rhs.min;
			max += rhs.max;
			return (*this);
		}
		inline FloatInterval& operator+=(const int val) {
			min += val;
			max += val;
			return (*this);
		}
		inline FloatInterval operator-(IntInterval rhs) const {
			return FloatInterval(min - rhs.max, max - rhs.min);
		}
		inline FloatInterval operator-(const int val) const {
			return FloatInterval(min - val, max - val);
		}
		inline FloatInterval& operator-=(IntInterval rhs) {
			min -= rhs.min;
			max -= rhs.max;
			return (*this);
		}
		inline FloatInterval& operator-=(const int val) {
			min -= val;
			max -= val;
			return (*this);
		}


		inline bool operator>(const float val) const {
			return val < min && min <= max;
		}
		inline bool operator>=(const float val) const {
			return val <= min && min <= max;
		}
		inline bool operator<(const float val) const {
			return min <= max && max < val;
		}
		inline bool operator<=(const float val) const {
			return min <= max && max <= val;
		}

		inline bool operator>(FloatInterval other) const {
			return other.min <= other.max && other.max < min && min <= max;
		}
		inline bool operator>=(FloatInterval other) const {
			return other.min <= other.max && other.max <= min && min <= max;
		}
		inline bool operator<(FloatInterval other) const {
			return min <= max && max < other.min && other.min <= other.max;
		}
		inline bool operator<=(FloatInterval other) const {
			return min <= max && max <= other.min && other.min <= other.max;
		}

		inline FloatInterval abs(void) const {
			if (min > 0 || is_bottom()) {
				return FloatInterval(*this);
			}
			else if (max < 0) {
				return -(*this);
			}
			else {
				return FloatInterval(0, SDL_max(-min, max));
			}
		}
		inline FloatInterval positivePart(void) const {
			FloatInterval result = FloatInterval(*this);
			result.addLowerBound(0);
			return result;
		}
		inline FloatInterval negativePart(void) const {
			FloatInterval result = FloatInterval(*this);
			result.addUpperBound(0);
			return result;
		}

		inline void splitAt(FloatInterval& above, FloatInterval& below, float limit, bool includeLimitAbove) {
			if (is_bottom()) {
				above.make_bottom();
				below.make_bottom();
			}
			else {
				below.min = min;
				below.max = includeLimitAbove ? Numerics::next_float_below(limit) : limit;
				above.min = includeLimitAbove ? limit : Numerics::next_float_above(limit);
				above.max = max;
			}
		}

		inline FloatInterval getIntervalBelow(void) const {
			if (is_bottom()) {
				return FloatInterval::top();
			}
			else if (!has_lower_bound()) {
				return FloatInterval::bottom();
			}
			else {
				return FloatInterval::fromUpperBound(Numerics::next_float_below(min));
			}
		}
		inline FloatInterval getIntervalAbove(void) const {
			if (is_bottom()) {
				return FloatInterval::top();
			}
			else if (!has_upper_bound()) {
				return FloatInterval::bottom();
			}
			else {
				return FloatInterval::fromUpperBound(Numerics::next_float_above(max));
			}
		}

		inline FloatInterval& clamp(float cmin, float cmax) {
			if (this->is_bottom() || !(cmax >= cmin)) {
				make_bottom();
			} else {
				// Everything less than interval.min is set to interval.min
				// Everything greater than interval.max is set to interval.max
				// This is different from regular intersection!
				min = SDL_clamp(min, cmin, cmax);
				max = SDL_clamp(max, cmin, cmax);
			}
			return *this;
		}

		inline FloatInterval& addUpperBound(float limit) {
			max = SDL_min(max, limit);
			return (*this);
		}
		inline FloatInterval& addLowerBound(float limit) {
			min = SDL_max(min, limit);
			return (*this);
		}

		inline static FloatInterval bottom(void) {
			return FloatInterval(INFINITY, -INFINITY);
		}
		inline static FloatInterval top(void) {
			return FloatInterval();
		}
		inline static FloatInterval fromLowerBound(float min) {
			return FloatInterval(min, INFINITY);
		}
		inline static FloatInterval fromUpperBound(float max) {
			return FloatInterval(-INFINITY, max);
		}
		inline static FloatInterval positive(void) {
			return fromLowerBound(0.0f);
		}
		inline static FloatInterval negative(void) {
			return fromUpperBound(0.0f);
		}

		inline FloatInterval& join(float val) {
			if (is_bottom()) {
				min = val;
				max = val;
				return *this;
			}
			else {
				min = SDL_min(min, val);
				max = SDL_max(max, val);
			}
			return *this;
		}
		inline FloatInterval& join(FloatInterval other) {
			if (other.is_bottom()) {
				return *this;
			}
			else if (is_bottom()) {
				min = other.min;
				max = other.max;
				return *this;
			}
			else {
				min = SDL_min(min, other.min);
				max = SDL_max(max, other.max);
			}
			return *this;
		}
		inline FloatInterval& join(IntInterval other) {
			if (other.is_bottom()) {
				return *this;
			}
			else if (is_bottom()) {
				min = (float)other.min;
				max = (float)other.max;
				return *this;
			}
			else {
				min = SDL_min(min, (float)other.min);
				max = SDL_max(max, (float)other.max);
			}
			return *this;
		}
		inline FloatInterval& intersect(FloatInterval other) {
			min = SDL_max(min, other.min);
			max = SDL_min(max, other.max);
			return *this;
		}
		inline FloatInterval& intersect(IntInterval other) {
			min = SDL_max(min, (float)other.min);
			max = SDL_min(max, (float)other.max);
			return *this;
		}

		inline static FloatInterval join(FloatInterval a, FloatInterval b) {
			FloatInterval res(a);
			res.join(b);
			return res;
		}
		inline static FloatInterval intersect(FloatInterval a, FloatInterval b) {
			FloatInterval res(a);
			res.intersect(b);
			return res;
		}

		inline static FloatInterval fromIntInterval(IntInterval a) {
			if (a.is_bottom()) {
				return FloatInterval::bottom();
			}
			
			float min = a.has_lower_bound() ? a.min : -INFINITY;
			float max = a.has_upper_bound() ? a.max : INFINITY;

			return FloatInterval(min, max);
		}
	};

}

#endif /* SOLVER_GEOMETRY_H */