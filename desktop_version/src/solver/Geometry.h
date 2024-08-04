#ifndef SOLVER_GEOMETRY_H
#define SOLVER_GEOMETRY_H

#include <limits>

#include <SDL.h>

namespace Geometry {
	int safeNegate(int x) {
		switch (x) {
			case INT_MAX:
				return INT_MIN;
			case INT_MIN:
				return INT_MAX;
			default:
				return -x;
		}
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

		IntVector& operator+=(const IntVector& rhs) {
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
		IntVector operator+(const IntVector& other) {
			return IntVector(x + other.x, y + other.y);
		}
		IntVector& operator-=(const IntVector& rhs) {
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}
		IntVector operator-(const IntVector& other) {
			return IntVector(x - other.x, y - other.y);
		}
		IntVector& operator*=(const IntVector& rhs) {
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}
		IntVector operator*(const IntVector& other) {
			return IntVector(x * other.x, y * other.y);
		}
		IntVector& operator/=(const IntVector& rhs) {
			x /= rhs.x;
			y /= rhs.y;
			return *this;
		}
		IntVector operator/(const IntVector& other) {
			return IntVector(x / other.x, y / other.y);
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
		IntInterval& regularize(void) {
			if (this->is_bottom()) {
				return this->make_bottom();
			} else {
				return *this;
			}
		}

		// Default constructor returns top
		IntInterval() {
			min = INT_MIN;
			max = INT_MAX;
			regularize();
		}
		IntInterval(int val) : min(val), max(val) {
			regularize();
		}
		IntInterval(int min, int max) : min(min), max(max) {
			regularize();
		}

		bool is_bottom(void) const {
			return max > min;
		}
		bool is_top(void) const {
			return min == INT_MIN && max == INT_MAX;
		}
		bool is_exact(void) const {
			return min == max && min > INT_MIN && max < INT_MAX;
		}

		bool contains(int val) const;
		bool contains(const IntInterval& other) const;

		IntInterval& negate(void) {
			if (!this->is_bottom()) {
				int oldMax = max;
				max = safeNegate(min);
				min = safeNegate(oldMax);
			}
			return regularize();
		}
		IntInterval operator-(void) const {
			if (this->is_bottom()) {
				return IntInterval::bottom();
			}
			return IntInterval(-max, -min);
		}
		IntInterval& operator+=(const IntInterval& rhs) {
			if (rhs.is_bottom() || this->is_bottom()) {
				// Propagate bottom
				min = INT_MAX;
				max = INT_MIN;
			} else {
				min += rhs.min;
				max += rhs.max;
			}
			return regularize();
		}
		IntInterval operator+(const IntInterval& other) const {
			if (this->is_bottom() || other.is_bottom()) {
				return IntInterval::bottom();
			}
			return IntInterval(min + other.min, max + other.max);
		}
		IntInterval& operator-=(const IntInterval& rhs) {
			return this->operator+=(-rhs);
		}
		IntInterval operator-(const IntInterval& other) const {
			return this->operator+(-other);
		}

		IntInterval& limitMax(int limit) {
			min = SDL_max(min, limit);
			return regularize();
		}
		IntInterval& limitMin(int limit) {
			max = SDL_min(max, limit);
			return regularize();
		}
		IntInterval& join(const IntInterval& other);
		IntInterval& intersect(const IntInterval& other);

		static IntInterval bottom(void) {
			return IntInterval(INT_MAX, INT_MIN);
		}
		static IntInterval top(void) {
			return IntInterval(INT_MIN, INT_MAX);
		}
		static IntInterval fromMin(int min) {
			return IntInterval(min, INT_MAX);
		}
		static IntInterval fromMax(int max) {
			return IntInterval(INT_MIN, max);
		}
		static IntInterval join(const IntInterval& a, const IntInterval& b);
		static IntInterval intersect(const IntInterval& a, const IntInterval& b);
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

		bool contains(const Region& other) const;
		bool contains(const IntVector& val) const;
		bool contains(int x, int y) const;

		Region& limitXBelow(int limit) {
			x.limitMax(limit);
			return regularize();
		}
		Region& limitXAbove(int limit) {
			x.limitMin(limit);
			return regularize();
		}
		Region& limitYBelow(int limit) {
			y.limitMax(limit);
			return regularize();
		}
		Region& limitYAbove(int limit) {
			y.limitMin(limit);
			return regularize();
		}
		Region& join(const Region& other);
		Region& intersect(const Region& other);

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
		static Region fromXMin(int x_min) {
			return Region::fromXInterval(IntInterval::fromMin(x_min));
		}
		static Region fromXMax(int x_max) {
			return Region::fromXInterval(IntInterval::fromMax(x_max));
		}
		static Region fromYMin(int y_min) {
			return Region::fromYInterval(IntInterval::fromMin(y_min));
		}
		static Region fromYMax(int y_max) {
			return Region::fromYInterval(IntInterval::fromMax(y_max));
		}
		static Region join(const Region& a, const Region& b);
		static Region intersect(const Region& a, const Region& b);
	};


}

#endif /* SOLVER_GEOMETRY_H */