#ifndef SOLVER_NUMERICS_H
#define SOLVER_NUMERICS_H

#include <cmath>
#include <limits>

#include "solver/Exceptions.h"

namespace Numerics {
	inline static float next_float_above(float val) {
		return std::nextafterf(val, INFINITY);
	}
	inline static float next_float_below(float val) {
		return std::nextafterf(val, -INFINITY);
	}

	struct BoolRange {
	private:
		bool value, exact;

		BoolRange& make_top(void) {
			exact = false;
			value = true;
			return *this;
		}
		BoolRange& make_bottom(void) {
			exact = false;
			value = false;
			return *this;
		}
	public:
		bool is_top(void) const {
			return value && !exact;
		}
		bool is_bottom(void) const {
			return !value && !exact;
		}
		bool is_exact(void) const {
			return exact;
		}
		bool is_true(void) const {
			return value && exact;
		}
		bool is_false(void) const {
			return !value && exact;
		}
		bool may_be_true(void) const {
			return is_true() || is_top();
		}
		bool may_be_false(void) const {
			return is_false() || is_top();
		}

		BoolRange(void) : value(true), exact(false) { }
		BoolRange(bool val) : value(val), exact(true) { }

		BoolRange operator!(void) const {
			if (is_exact()) {
				return BoolRange(!value);
			}
			return BoolRange(*this);
		}
		BoolRange operator||(const BoolRange& rhs) const {
			if (is_bottom() || rhs.is_bottom()) {
				return BoolRange::bottom();
			} else if (is_true() || rhs.is_true()) {
				return BoolRange(true);
			} else if (is_false() && rhs.is_false()) {
				return BoolRange(false);
			} else {
				return BoolRange::top();
			}
		}
		BoolRange operator&&(const BoolRange& rhs) const {
			if (is_bottom() || rhs.is_bottom()) {
				return BoolRange::bottom();
			} else if (is_true() && rhs.is_true()) {
				return BoolRange(true);
			} else if (is_false() || rhs.is_false()) {
				return BoolRange(false);
			} else {
				return BoolRange::top();
			}
		}

		BoolRange& intersect(const BoolRange& rhs) {
			if (is_bottom() || rhs.is_bottom()) {
				make_bottom();
			} else if (rhs.is_exact()) {
				if (is_top()) {
					exact = true;
					value = rhs.value;
				} else if (value != rhs.value) {
					make_bottom();
				}
			}
			return *this;
		}
		BoolRange& join(const BoolRange& rhs) {
			if (is_top() || rhs.is_top()) {
				make_top();
			} else if (rhs.is_exact()) {
				if (is_bottom()) {
					exact = true;
					value = rhs.value;
				} else if (value != rhs.value) {
					make_top();
				}
			}
			return *this;
		}
		BoolRange& difference(const BoolRange& rhs) {
			if (is_bottom() || rhs.is_top()) {
				make_bottom();
			} else if (rhs.is_exact()) {
				if (is_top()) {
					exact = true;
					value = !rhs.value;
				} else if (value == rhs.value) {
					make_bottom();
				}
			}
			return *this;
		}


		bool intersects(const BoolRange& rhs) {
			if (is_bottom() || rhs.is_bottom()) {
				return false;
			} else if (is_top() || rhs.is_top()) {
				return true;
			}
			return value == rhs.value;
		}
		bool contains(const BoolRange& rhs) {
			if (is_top() || rhs.is_bottom()) {
				return true;
			} else if (is_bottom() || rhs.is_top()) {
				return false;
			}
			return value == rhs.value;
		}

		static BoolRange bottom(void) {
			return BoolRange().make_bottom();
		}
		static BoolRange top(void) {
			return BoolRange();
		}
		static BoolRange _true(void) {
			return BoolRange(true);
		}
		static BoolRange _false(void) {
			return BoolRange(false);
		}
	};

	struct SaturatingInt {
	private:
		int val;
		// val == INT_MAX -> some large positive number (not infinity)
		// val == -INT_MAX -> some large negative number (not infinity)
		// val == INT_MIN -> invalid value
		void assert_valid(void) const {
			Exceptions::assert(is_valid());
		}
		void assert_bounded(void) const {
			Exceptions::assert(is_bounded());
		}
		int unwrap(void) const {
			assert_bounded();
			return val;
		}
		SaturatingInt& make_invalid(void) {
			val = INT_MIN;
			return *this;
		}
		SaturatingInt& make_pos_unbounded(void) {
			val = INT_MAX;
			return *this;
		}
		SaturatingInt& make_neg_unbounded(void) {
			val = -INT_MAX;
			return *this;
		}
	public:
		bool is_valid(void) const {
			return val != INT_MIN;
		}
		bool is_invalid(void) const {
			return val == INT_MIN;
		}
		bool is_bounded(void) const {
			return -INT_MAX < val && val < INT_MAX;
		}
		bool is_unbounded(void) const {
			return is_pos_unbounded() || is_neg_unbounded();
		}
		bool is_pos_unbounded(void) const {
			return val == INT_MAX;
		}
		bool is_neg_unbounded(void) const {
			return val == -INT_MAX;
		}
		bool is_positive(void) const {
			return is_valid() && val >= 0;
		}
		bool is_negative(void) const {
			return is_valid() && val <= 0;
		}
		bool is_zero(void) const {
			return val == 0;
		}

		SaturatingInt(void) : val(0) { }
		SaturatingInt(int val) : val(val) {
			Exceptions::require(is_bounded());
			assert_valid();
		}

		bool operator==(const SaturatingInt& rhs) const;
		bool operator!=(const SaturatingInt& rhs) const;
		bool operator>(const SaturatingInt& rhs) const;
		bool operator>=(const SaturatingInt& rhs) const;
		bool operator<(const SaturatingInt& rhs) const;
		bool operator<=(const SaturatingInt& rhs) const;
		bool operator==(const int rhs) const;
		bool operator!=(const int rhs) const;
		bool operator>(const int rhs) const;
		bool operator>=(const int rhs) const;
		bool operator<(const int rhs) const;
		bool operator<=(const int rhs) const;

		SaturatingInt& negate(void);
		SaturatingInt& operator+=(const SaturatingInt& rhs);
		SaturatingInt& operator-=(const SaturatingInt& rhs);
		SaturatingInt& operator*=(const SaturatingInt& rhs);
		SaturatingInt& operator/=(const SaturatingInt& rhs);

		SaturatingInt abs(void) const;
		SaturatingInt operator-(void) const;
		SaturatingInt operator+(const SaturatingInt& rhs) const;
		SaturatingInt operator-(const SaturatingInt& rhs) const;
		SaturatingInt operator*(const SaturatingInt& rhs) const;
		SaturatingInt operator/(const SaturatingInt& rhs) const;

		SaturatingInt& operator+=(const int rhs);
		SaturatingInt& operator-=(const int rhs);
		SaturatingInt& operator*=(const int rhs);
		SaturatingInt& operator/=(const int rhs);
		SaturatingInt operator+(const int rhs) const;
		SaturatingInt operator-(const int rhs) const;
		SaturatingInt operator*(const int rhs) const;
		SaturatingInt operator/(const int rhs) const;

		SaturatingInt& max(const SaturatingInt& rhs);
		SaturatingInt& min(const SaturatingInt& rhs);
		SaturatingInt& max(const int rhs);
		SaturatingInt& min(const int rhs);

		SaturatingInt& clamp(const SaturatingInt& min, const SaturatingInt& max);

		static SaturatingInt max(const SaturatingInt& lhs, const SaturatingInt& rhs);
		static SaturatingInt min(const SaturatingInt& lhs, const SaturatingInt& rhs);

		// "Important" values
		static SaturatingInt zero(void) {
			return SaturatingInt(0);
		}
		static SaturatingInt one(void) {
			return SaturatingInt(0);
		}
		static SaturatingInt pos_unbounded(void) {
			return SaturatingInt(INT_MAX);
		}
		static SaturatingInt neg_unbounded(void) {
			return SaturatingInt(-INT_MAX);
		}
		static SaturatingInt invalid(void) {
			return SaturatingInt(INT_MIN);
		}
	};
	SaturatingInt operator+(const int lhs, const SaturatingInt& rhs);
	SaturatingInt operator-(const int lhs, const SaturatingInt& rhs);
	SaturatingInt operator*(const int lhs, const SaturatingInt& rhs);
	SaturatingInt operator/(const int lhs, const SaturatingInt& rhs);

	struct SaturatingFloat {
	private:
		float val;

	public:

	};

	struct IntRange {
	private:
		SaturatingInt min, max;

		IntRange& make_top(void) {
			min = SaturatingInt::neg_unbounded();
			max = SaturatingInt::pos_unbounded();
			return *this;
		}
		IntRange& make_bottom(void) {
			min = SaturatingInt::pos_unbounded();
			max = SaturatingInt::neg_unbounded();
			return *this;
		}
		IntRange& regularize(void) {
			return is_bottom() ? make_bottom() : *this;
		}

		IntRange(SaturatingInt exact_value) : min(exact_value), max(exact_value) {
			regularize();
		}
		IntRange(SaturatingInt from, SaturatingInt to) : min(SaturatingInt::min(from, to)), max(SaturatingInt::max(from, to)) {
			regularize();
		}

		bool intersects(const SaturatingInt& other) const;
		IntRange& join(const SaturatingInt& rhs);
		IntRange& intersect(const SaturatingInt& rhs);
		IntRange& difference(const SaturatingInt& rhs);
	public:
		IntRange(void) : min(0), max(0) {
			make_top();
			regularize();
		}
		IntRange(int exact_value) : min(exact_value), max(exact_value) {
			regularize();
		}
		IntRange(int from, int to) : min(SDL_min(from, to)), max(SDL_max(from, to)) {
			regularize();
		}

		bool is_bottom(void) const {
			return min.is_invalid() || max.is_invalid() || max.is_neg_unbounded() || min.is_pos_unbounded() || max < min;
		}
		bool is_top(void) const {
			return min.is_neg_unbounded() && max.is_pos_unbounded();
		}
		bool has_lower_bound(void) const {
			return min.is_bounded();
		}
		bool has_upper_bound(void) const {
			return max.is_bounded();
		}
		bool is_exact(void) const {
			return min.is_bounded() && max.is_bounded() && min == max;
		}
		bool is_positive(void) const {
			return !is_bottom() && min.is_positive() && max.is_positive();
		}
		bool is_negative(void) const {
			return !is_bottom() && min.is_negative() && max.is_negative();
		}

		bool operator==(const IntRange& rhs) const;
		bool operator!=(const IntRange& rhs) const;
		bool operator>(const IntRange& rhs) const;
		bool operator>=(const IntRange& rhs) const;
		bool operator<(const IntRange& rhs) const;
		bool operator<=(const IntRange& rhs) const;
		bool operator==(const int rhs) const;
		bool operator!=(const int rhs) const;
		bool operator>(const int rhs) const;
		bool operator>=(const int rhs) const;
		bool operator<(const int rhs) const;
		bool operator<=(const int rhs) const;

		IntRange& negate(void);
		IntRange& operator+=(const IntRange& rhs);
		IntRange& operator-=(const IntRange& rhs);
		IntRange& operator*=(const IntRange& rhs);
		IntRange& operator/=(const IntRange& rhs);

		IntRange abs(void) const;
		IntRange operator-(void) const;
		IntRange operator+(const IntRange& rhs) const;
		IntRange operator-(const IntRange& rhs) const;
		IntRange operator*(const IntRange& rhs) const;
		IntRange operator/(const IntRange& rhs) const;

		IntRange& operator+=(const int rhs);
		IntRange& operator-=(const int rhs);
		IntRange& operator*=(const int rhs);
		IntRange& operator/=(const int rhs);
		IntRange operator+(const int rhs) const;
		IntRange operator-(const int rhs) const;
		IntRange operator*(const int rhs) const;
		IntRange operator/(const int rhs) const;

		IntRange& join(const IntRange& rhs);
		IntRange& intersect(const IntRange& rhs);
		IntRange& difference(const IntRange& rhs);

		IntRange& join(const int rhs);
		IntRange& intersect(const int rhs);
		IntRange& difference(const int rhs);

		bool intersects(const IntRange& other) const;
		bool intersects(const int other) const;

		IntRange& clamp(const int min, const int max) {
			this->min.clamp(min, max);
			this->max.clamp(min, max);
			return regularize();
		}
		IntRange& addUpperBound(const int limit) {
			return intersect(fromUpperBound(limit));
		}
		IntRange& addLowerBound(const int limit) {
			return intersect(fromLowerBound(limit));
		}
		IntRange& removeUpperBound(void) {
			if (!is_bottom()) {
				max = SaturatingInt::pos_unbounded();
			}
			return regularize();
		}
		IntRange& removeLowerBound(void) {
			if (!is_bottom()) {
				min = SaturatingInt::pos_unbounded();
			}
			return regularize();
		}

		static IntRange bottom(void) {
			return IntRange().make_bottom();
		}
		static IntRange top(void) {
			return IntRange();
		}
		static IntRange zero(void) {
			return IntRange(0);
		}
		static IntRange positive(void) {
			return fromLowerBound(0);
		}
		static IntRange negative(void) {
			return fromUpperBound(0);
		}
		static IntRange fromLowerBound(const int limit) {
			return IntRange(limit).removeUpperBound();
		}
		static IntRange fromUpperBound(const int limit) {
			return IntRange(limit).removeLowerBound();
		}
	};
	IntRange operator+(const int lhs, const IntRange& rhs);
	IntRange operator-(const int lhs, const IntRange& rhs);
	IntRange operator*(const int lhs, const IntRange& rhs);
	IntRange operator/(const int lhs, const IntRange& rhs);

	struct FloatRange {
	private:
		float min, max;
	public:
		FloatRange& make_top(void) {
			min = -INFINITY;
			max = INFINITY;
			return *this;
		}
		FloatRange& make_bottom(void) {
			min = INFINITY;
			max = -INFINITY;
			return *this;
		}
		FloatRange& regularize(void) {
			return is_bottom() ? make_bottom() : *this;
		}

		FloatRange(void) : min(0), max(0) {
			make_top();
			regularize();
		}
		FloatRange(float exact_value) : min(exact_value), max(exact_value) {
			regularize();
		}
		FloatRange(float from, float to) : min(SDL_min(from, to)), max(SDL_max(from, to)) {
			regularize();
		}

		bool is_bottom(void) const {
			return isnan(min) || isnan(max) || max == -INFINITY || min == INFINITY || max < min;
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
			return has_lower_bound() && has_upper_bound() && min == max;
		}
		bool is_positive(void) const {
			return !is_bottom() && min >= 0 && max >= 0;
		}
		bool is_negative(void) const {
			return !is_bottom() && min <= 0 && max <= 0;
		}

		float getUpperBound(void) const {
			if (is_bottom()) {
				return NAN;
			} else if (!has_upper_bound()) {
				return INFINITY;
			} else {
				return max;
			}
		}
		float getLowerBound(void) const {
			if (is_bottom()) {
				return NAN;
			} else if (!has_lower_bound()) {
				return -INFINITY;
			} else {
				return min;
			}
		}

		BoolRange operator==(const FloatRange& rhs) const;
		BoolRange operator!=(const FloatRange& rhs) const;
		BoolRange operator>(const FloatRange& rhs) const;
		BoolRange operator>=(const FloatRange& rhs) const;
		BoolRange operator<(const FloatRange& rhs) const;
		BoolRange operator<=(const FloatRange& rhs) const;

		FloatRange& join(const FloatRange& other) {
			if (is_top() || other.is_top()) {
				return make_top();
			} else if (other.is_bottom()) {
				return regularize();
			}
			if (has_upper_bound()) {
				if (!other.has_upper_bound()) {
					removeUpperBound();
				} else if (max < other.max) {
					max = other.max;
				}
			}
			if (has_lower_bound()) {
				if (!other.has_lower_bound()) {
					removeLowerBound();
				} else if (min > other.min) {
					min = other.min;
				}
			}
			return regularize();
		}
		FloatRange& intersect(const FloatRange& other) {
			if (is_bottom() || other.is_bottom()) {
				return make_bottom();
			} else if (other.is_top()) {
				return regularize();
			}
			if (other.has_upper_bound()) {
				addUpperBound(other.getUpperBound());
			}
			if (other.has_lower_bound()) {
				addLowerBound(other.getLowerBound());
			}
			return regularize();
		}
		FloatRange& difference(const FloatRange& other) {
			if (other.contains(*this)) {
				return make_bottom();
			} else if (contains(other)) {
				return regularize();
			}
			if (contains(other.getLowerBound())) {
				addUpperBound(std::nextafterf(other.getLowerBound(), -INFINITY));
			}
			if (contains(other.getUpperBound())) {
				addLowerBound(std::nextafterf(other.getUpperBound(), INFINITY));
			}
			return regularize();
		}

		bool intersects(const FloatRange& other) const {
			// TODO make more efficient
			return !FloatRange(*this).intersect(other).is_bottom();
		}
		bool contains(const FloatRange& other) const {
			if (is_top() || other.is_bottom()) {
				return true;
			}
			if (has_lower_bound()) {
				if (!other.has_lower_bound()) {
					return false;
				} else if (min > other.min) {
					return false;
				}
			}
			if (has_upper_bound()) {
				if (!other.has_upper_bound()) {
					return false;
				}
				else if (max < other.max) {
					return false;
				}
			}
			return true;
		}

		IntRange toIntRange(void) const {
			if (is_bottom()) {
				return IntRange::bottom();
			}

			IntRange result;
			if (has_upper_bound()) {
				result.addUpperBound(SDL_ceilf(max));
			}
			if (has_lower_bound()) {
				result.addLowerBound(SDL_floorf(min));
			}
			return result;
		}

		FloatRange& addUpperBound(const float limit) {
			return intersect(fromUpperBound(limit));
		}
		FloatRange& addLowerBound(const float limit) {
			return intersect(fromLowerBound(limit));
		}
		FloatRange& removeUpperBound(void) {
			if (!is_bottom()) {
				max = INFINITY;
			}
			return regularize();
		}
		FloatRange& removeLowerBound(void) {
			if (!is_bottom()) {
				min = -INFINITY;
			}
			return regularize();
		}

		static FloatRange bottom(void) {
			return FloatRange().make_bottom();
		}
		static FloatRange top(void) {
			return FloatRange().make_top();
		}
		static FloatRange fromLowerBound(const float limit) {
			return FloatRange(limit).removeUpperBound();
		}
		static FloatRange fromUpperBound(const float limit) {
			return FloatRange(limit).removeLowerBound();
		}
	};

	FloatRange abs(const FloatRange& lhs);
	FloatRange operator-(const FloatRange& lhs);
	FloatRange operator+(const FloatRange& lhs, const FloatRange& rhs);
	FloatRange operator-(const FloatRange& lhs, const FloatRange& rhs);
	FloatRange operator*(const FloatRange& lhs, const FloatRange& rhs);
	FloatRange operator/(const FloatRange& lhs, const FloatRange& rhs);

	struct IntVector {

	};

	struct PosRange {

	};

	struct IntVectorRange {

	};

	struct FloatVectorRange {

	};

	// Constraint solving
	namespace Constraints {
		static void EnforceNegConstraint(FloatRange& a, FloatRange& neg_a) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				FloatRange new_a = -neg_a;
				if (!new_a.contains(a)) {
					anythingChanged = true;
					a.intersect(new_a);
				}
				FloatRange new_neg_a = -a;
				if (!new_neg_a.contains(neg_a)) {
					anythingChanged = true;
					neg_a.intersect(new_neg_a);
				}
			}
		}
		static void EnforceAbsConstraint(FloatRange& a, FloatRange& abs_a) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				FloatRange newAbs = abs(a);
				if (!newAbs.contains(abs_a)) {
					anythingChanged = true;
					abs_a.intersect(newAbs);
				}

				FloatRange newA = FloatRange(a).intersect(abs_a).join(FloatRange(a).intersect(-abs_a));
				if (!newA.contains(a)) {
					anythingChanged = true;
					a.intersect(newA);
				}
			}
		}
		static void EnforceAddConstraint(FloatRange& lhs, FloatRange& rhs, FloatRange& sum) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				FloatRange newSum = lhs + rhs;
				if (!newSum.contains(sum)) {
					anythingChanged = true;
					sum.intersect(newSum);
				}
				FloatRange newRhs = sum - lhs;
				if (!newRhs.contains(rhs)) {
					anythingChanged = true;
					rhs.intersect(newRhs);
				}
				FloatRange newLhs = sum - rhs;
				if (!newLhs.contains(lhs)) {
					anythingChanged = true;
					lhs.intersect(newLhs);
				}
			}
		}
		static void EnforceSubConstraint(FloatRange& lhs, FloatRange& rhs, FloatRange& diff) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				FloatRange newDiff = lhs - rhs;
				if (!newDiff.contains(diff)) {
					anythingChanged = true;
					diff.intersect(newDiff);
				}
				FloatRange newRhs = lhs - diff;
				if (!newRhs.contains(rhs)) {
					anythingChanged = true;
					rhs.intersect(newRhs);
				}
				FloatRange newLhs = diff + rhs;
				if (!newLhs.contains(lhs)) {
					anythingChanged = true;
					lhs.intersect(newLhs);
				}
			}
		}
		static void EnforceMulConstraint(FloatRange& lhs, FloatRange& rhs, FloatRange& prod) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				FloatRange newProd = lhs * rhs;
				if (!newProd.contains(prod)) {
					anythingChanged = true;
					prod.intersect(newProd);
				}
				FloatRange newRhs = prod / lhs;
				if (!newRhs.contains(rhs)) {
					anythingChanged = true;
					rhs.intersect(newRhs);
				}
				FloatRange newLhs = prod / rhs;
				if (!newLhs.contains(lhs)) {
					anythingChanged = true;
					lhs.intersect(newLhs);
				}
			}
		}
		static void EnforceDivConstraint(FloatRange& lhs, FloatRange& rhs, FloatRange& quot) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				FloatRange newQuot = lhs / rhs;
				if (!newQuot.contains(quot)) {
					anythingChanged = true;
					quot.intersect(newQuot);
				}
				FloatRange newRhs = lhs / quot;
				if (!newRhs.contains(rhs)) {
					anythingChanged = true;
					rhs.intersect(newRhs);
				}
				FloatRange newLhs = rhs * quot;
				if (!newLhs.contains(lhs)) {
					anythingChanged = true;
					lhs.intersect(newLhs);
				}
			}
		}
		static void EnforceGtConstraint(FloatRange& lhs, FloatRange& rhs, BoolRange& cmp) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				BoolRange newCmp = lhs > rhs;
				if (!newCmp.contains(cmp)) {
					anythingChanged = true;
					cmp.intersect(newCmp);
				}

				FloatRange newRhs = FloatRange::bottom();
				if (cmp.may_be_false()) {
					newRhs.join(FloatRange::fromLowerBound(lhs.getLowerBound()));
				}
				if (cmp.may_be_true()) {
					newRhs.join(FloatRange::fromUpperBound(std::nextafterf(lhs.getUpperBound(), -INFINITY)));
				}
				if (!newRhs.contains(rhs)) {
					anythingChanged = true;
					rhs.intersect(newRhs);
				}

				FloatRange newLhs = FloatRange::bottom();
				if (cmp.may_be_false()) {
					newLhs.join(FloatRange(lhs).addUpperBound(rhs.getUpperBound()));
				}
				if (cmp.may_be_true()) {
					newLhs.join(FloatRange(lhs).addLowerBound(std::nextafterf(rhs.getLowerBound(), INFINITY)));
				}
				if (!newLhs.contains(lhs)) {
					anythingChanged = true;
					lhs.intersect(newLhs);
				}
			}
		}
		static void EnforceGtEqConstraint(FloatRange& lhs, FloatRange& rhs, BoolRange& cmp) {
			// lhs >= rhs -> !(rhs > lhs)
			BoolRange notCmp = !cmp;
			EnforceGtConstraint(rhs, lhs, notCmp);
			cmp.intersect(!notCmp);
		}
		static void EnforceLtConstraint(FloatRange& lhs, FloatRange& rhs, BoolRange& cmp) {
			// lhs < rhs -> rhs > lhs
			EnforceGtConstraint(rhs, lhs, cmp);
		}
		static void EnforceLtEqConstraint(FloatRange& lhs, FloatRange& rhs, BoolRange& cmp) {
			// lhs <= rhs -> !(lhs > rhs)
			BoolRange notCmp = !cmp;
			EnforceGtConstraint(lhs, rhs, notCmp);
			cmp.intersect(!notCmp);
		}
		static void EnforceEqConstraint(FloatRange& lhs, FloatRange& rhs, BoolRange& cmp) {
			bool anythingChanged = true;
			while (anythingChanged) {
				anythingChanged = false;

				BoolRange newCmp = lhs == rhs;
				if (!newCmp.contains(cmp)) {
					anythingChanged = true;
					cmp.intersect(newCmp);
				}

				FloatRange newRhs = FloatRange::bottom();
				if (cmp.may_be_false()) {
					newRhs.join(FloatRange(rhs));
				}
				if (cmp.may_be_true()) {
					newRhs.join(FloatRange(rhs).intersect(lhs));
				}
				if (!newRhs.contains(rhs)) {
					anythingChanged = true;
					rhs.intersect(newRhs);
				}

				FloatRange newLhs = FloatRange::bottom();
				if (cmp.may_be_false()) {
					newLhs.join(FloatRange(lhs));
				}
				if (cmp.may_be_true()) {
					newLhs.join(FloatRange(lhs).intersect(rhs));
				}
				if (!newLhs.contains(lhs)) {
					anythingChanged = true;
					lhs.intersect(newLhs);
				}
			}
		}
		static void EnforceNeqConstraint(FloatRange& lhs, FloatRange& rhs, BoolRange& cmp) {
			// lhs != rhs -> !(lhs == rhs)
			BoolRange notCmp = !cmp;
			EnforceEqConstraint(lhs, rhs, notCmp);
			cmp.intersect(!notCmp);
		}
	};
}

#endif /* SOLVER_NUMERICS_H */