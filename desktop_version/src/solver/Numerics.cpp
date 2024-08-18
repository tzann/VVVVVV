#include "solver/Numerics.h"

namespace Numerics {
	bool SaturatingInt::operator==(const SaturatingInt& rhs) const {
		if (!is_bounded() || !rhs.is_bounded()) {
			return false;
		}
		return val == rhs.val;
	}
	bool SaturatingInt::operator!=(const SaturatingInt& rhs) const {
		assert_bounded();
		rhs.assert_bounded();
		return val != rhs.val;
	}
	bool SaturatingInt::operator>(const SaturatingInt& rhs) const {
		assert_valid();
		rhs.assert_valid();
		if (!is_bounded() && !rhs.is_bounded()) {
			if (is_pos_unbounded() && rhs.is_neg_unbounded()) {
				return true;
			} else if (is_neg_unbounded() && rhs.is_pos_unbounded()) {
				return false;
			} else {
				Exceptions::invalid_argument();
			}
		} else {
			return val > rhs.val;
		}
	}
	bool SaturatingInt::operator>=(const SaturatingInt& rhs) const {
		assert_valid();
		rhs.assert_valid();
		if (!is_bounded() && !rhs.is_bounded()) {
			if (is_pos_unbounded() && rhs.is_neg_unbounded()) {
				return true;
			} else if (is_neg_unbounded() && rhs.is_pos_unbounded()) {
				return false;
			} else {
				Exceptions::invalid_argument();
			}
		} else {
			return val >= rhs.val;
		}
	}
	bool SaturatingInt::operator<(const SaturatingInt& rhs) const {
		assert_valid();
		rhs.assert_valid();
		if (!is_bounded() && !rhs.is_bounded()) {
			if (is_pos_unbounded() && rhs.is_neg_unbounded()) {
				return false;
			} else if (is_neg_unbounded() && rhs.is_pos_unbounded()) {
				return true;
			} else {
				Exceptions::invalid_argument();
			}
		} else {
			return val < rhs.val;
		}
	}
	bool SaturatingInt::operator<=(const SaturatingInt& rhs) const {
		assert_valid();
		rhs.assert_valid();
		if (!is_bounded() && !rhs.is_bounded()) {
			if (is_pos_unbounded() && rhs.is_neg_unbounded()) {
				return false;
			} else if (is_neg_unbounded() && rhs.is_pos_unbounded()) {
				return true;
			} else {
				Exceptions::invalid_argument();
			}
		} else {
			return val <= rhs.val;
		}
	}

	bool SaturatingInt::operator==(const int rhs) const {
		return *this == SaturatingInt(rhs);
	}
	bool SaturatingInt::operator!=(const int rhs) const {
		return *this != SaturatingInt(rhs);
	}
	bool SaturatingInt::operator>(const int rhs) const {
		return *this > SaturatingInt(rhs);
	}
	bool SaturatingInt::operator>=(const int rhs) const {
		return *this >= SaturatingInt(rhs);
	}
	bool SaturatingInt::operator<(const int rhs) const {
		return *this < SaturatingInt(rhs);
	}
	bool SaturatingInt::operator<=(const int rhs) const {
		return *this <= SaturatingInt(rhs);
	}

	SaturatingInt& SaturatingInt::negate(void) {
		if (is_valid()) {
			val = -val;
		}
		return *this;
	}
	SaturatingInt& SaturatingInt::operator+=(const SaturatingInt& rhs) {
		if (!is_valid() || !rhs.is_valid()) {
			make_invalid();
		} else if (is_zero() || rhs.is_zero()) {
			val += rhs.val;
		} else if (is_unbounded() || rhs.is_unbounded()) {
			if (is_positive() && rhs.is_positive()) {
				make_pos_unbounded();
			} else if (is_negative() && rhs.is_negative()) {
				make_neg_unbounded();
			} else {
				// Not enough information
				make_invalid();
			}
		} else if (is_positive()) {
			if (rhs.is_positive() && (val + rhs.val <= val)) {
				// Overflow
				make_pos_unbounded();
			} else {
				// Can't overflow or underflow
				val += rhs.val;
			}
		} else {
			if (rhs.is_negative() && (val + rhs.val >= val)) {
				// Underflow
				make_neg_unbounded();
			} else {
				// Can't overflow or underflow
				val += rhs.val;
			}
		}

		return *this;
	}
	SaturatingInt& SaturatingInt::operator-=(const SaturatingInt& rhs) {
		if (!is_valid() || !rhs.is_valid()) {
			return make_invalid();
		}

		return this->operator+=(-rhs);
	}
	SaturatingInt& SaturatingInt::operator*=(const SaturatingInt& rhs) {
		if (!is_valid() || !rhs.is_valid()) {
			return make_invalid();
		} else if (is_zero() || rhs.is_zero()) {
			val = 0;
		} else if (rhs.is_negative()) {
			return this->operator*=(-rhs).negate();
		} else if (is_negative()) {
			return this->negate().operator*=(rhs).negate();
		} else {
			// Both positive
			if (is_unbounded() || rhs.is_unbounded()) {
				make_pos_unbounded();
			} else {
				// Check for overflow
				int res = val * rhs.val;
				if (res / rhs.val != val) {
					make_pos_unbounded();
				} else {
					val = res;
				}
			}
		}

		return *this;
	}

	SaturatingInt& SaturatingInt::operator/=(const SaturatingInt& rhs) {
		if (!is_valid() || !rhs.is_valid() || rhs.is_zero()) {
			return make_invalid();
		} else if (is_zero()) {
			val = 0;
		} else if (rhs.is_negative()) {
			return this->operator/=(-rhs).negate();
		} else if (is_negative()) {
			return this->negate().operator/=(rhs).negate();
		} else {
			// Both positive non-zero
			if (is_unbounded()) {
				// Not enough information
				make_invalid();
			} else if (rhs.is_unbounded()) {
				val = 0;
			} else {
				val /= rhs.val;
			}
		}

		return *this;
	}

	SaturatingInt SaturatingInt::abs(void) const {
		if (is_invalid()) {
			return SaturatingInt::invalid();
		} else if (is_negative()) {
			return SaturatingInt(*this).negate();
		} else {
			return *this;
		}
	}
	SaturatingInt SaturatingInt::operator-(void) const {
		return SaturatingInt(*this).negate();
	}
	SaturatingInt SaturatingInt::operator+(const SaturatingInt& rhs) const {
		return (SaturatingInt(*this)).operator+=(rhs);
	}
	SaturatingInt SaturatingInt::operator-(const SaturatingInt& rhs) const {
		return (SaturatingInt(*this)).operator-=(rhs);
	}
	SaturatingInt SaturatingInt::operator*(const SaturatingInt& rhs) const {
		return (SaturatingInt(*this)).operator*=(rhs);
	}
	SaturatingInt SaturatingInt::operator/(const SaturatingInt& rhs) const {
		return (SaturatingInt(*this)).operator/=(rhs);
	}

	SaturatingInt& SaturatingInt::operator+=(const int rhs) {
		return this->operator+=(SaturatingInt(rhs));
	}
	SaturatingInt& SaturatingInt::operator-=(const int rhs) {
		return this->operator-=(SaturatingInt(rhs));
	}
	SaturatingInt& SaturatingInt::operator*=(const int rhs) {
		return this->operator*=(SaturatingInt(rhs));
	}
	SaturatingInt& SaturatingInt::operator/=(const int rhs) {
		return this->operator/=(SaturatingInt(rhs));
	}
	SaturatingInt SaturatingInt::operator+(const int rhs) const {
		return this->operator+(SaturatingInt(rhs));
	}
	SaturatingInt SaturatingInt::operator-(const int rhs) const {
		return this->operator-(SaturatingInt(rhs));
	}
	SaturatingInt SaturatingInt::operator*(const int rhs) const {
		return this->operator*(SaturatingInt(rhs));
	}
	SaturatingInt SaturatingInt::operator/(const int rhs) const {
		return this->operator/(SaturatingInt(rhs));
	}

	SaturatingInt operator+(const int lhs, const SaturatingInt& rhs) {
		return SaturatingInt(lhs) + rhs;
	}
	SaturatingInt operator-(const int lhs, const SaturatingInt& rhs) {
		return SaturatingInt(lhs) - rhs;
	}
	SaturatingInt operator*(const int lhs, const SaturatingInt& rhs) {
		return SaturatingInt(lhs) * rhs;
	}
	SaturatingInt operator/(const int lhs, const SaturatingInt& rhs) {
		return SaturatingInt(lhs) / rhs;
	}

	SaturatingInt& SaturatingInt::max(const SaturatingInt& rhs) {
		if (is_invalid() || rhs.is_invalid()) {
			make_invalid();
		} else if (is_pos_unbounded() || rhs.is_pos_unbounded()) {
			make_pos_unbounded();
		} else if (is_neg_unbounded() || !rhs.is_neg_unbounded() && val < rhs.val) {
			val = rhs.val;
		}
		return *this;
	}
	SaturatingInt& SaturatingInt::min(const SaturatingInt& rhs) {
		if (is_invalid() || rhs.is_invalid()) {
			make_invalid();
		} else if (is_neg_unbounded() || rhs.is_neg_unbounded()) {
			make_neg_unbounded();
		} else if (is_pos_unbounded() || !rhs.is_pos_unbounded() && val > rhs.val) {
			val = rhs.val;
		}
		return *this;
	}
	SaturatingInt& SaturatingInt::max(const int rhs) {
		return this->max(SaturatingInt(rhs));
	}
	SaturatingInt& SaturatingInt::min(const int rhs) {
		return this->min(SaturatingInt(rhs));
	}

	SaturatingInt SaturatingInt::max(const SaturatingInt& lhs, const SaturatingInt& rhs) {
		return SaturatingInt(lhs).max(rhs);
	}
	SaturatingInt SaturatingInt::min(const SaturatingInt& lhs, const SaturatingInt& rhs) {
		return SaturatingInt(lhs).min(rhs);
	}

	SaturatingInt& SaturatingInt::clamp(const SaturatingInt& min, const SaturatingInt& max) {
		if (is_invalid() || min.is_invalid() || max.is_invalid() || min.is_pos_unbounded() || max.is_neg_unbounded() || min > max) {
			make_invalid();
		}
		return this->min(min).max(max);
	}

	// --------------------
	// IntRange
	// --------------------
	bool IntRange::operator==(const IntRange& rhs) const {
		return is_exact() && rhs.is_exact() && min == rhs.min;
	}
	bool IntRange::operator!=(const IntRange& rhs) const {
		return !is_bottom() && !rhs.is_bottom() && !intersects(rhs);
	}
	bool IntRange::operator>(const IntRange& rhs) const {
		return !is_bottom() && !rhs.is_bottom() && min > rhs.max;
	}
	bool IntRange::operator>=(const IntRange& rhs) const {
		return !is_bottom() && !rhs.is_bottom() && min >= rhs.max;
	}
	bool IntRange::operator<(const IntRange& rhs) const {
		return !is_bottom() && !rhs.is_bottom() && max < rhs.min;
	}
	bool IntRange::operator<=(const IntRange& rhs) const {
		return !is_bottom() && !rhs.is_bottom() && max <= rhs.min;
	}
	bool IntRange::operator==(const int rhs) const {
		return this->operator==(IntRange(rhs));
	}
	bool IntRange::operator!=(const int rhs) const {
		return this->operator!=(IntRange(rhs));
	}
	bool IntRange::operator>(const int rhs) const {
		return this->operator>(IntRange(rhs));
	}
	bool IntRange::operator>=(const int rhs) const {
		return this->operator>=(IntRange(rhs));
	}
	bool IntRange::operator<(const int rhs) const {
		return this->operator<(IntRange(rhs));
	}
	bool IntRange::operator<=(const int rhs) const {
		return this->operator<=(IntRange(rhs));
	}

	IntRange& IntRange::negate(void) {
		SaturatingInt tmp = min;
		min = -max;
		max = -tmp;
		return regularize();
	}
	IntRange& IntRange::operator+=(const IntRange& rhs) {
		if (is_bottom() || rhs.is_bottom()) {
			make_bottom();
		} else {
			min += rhs.min;
			max += rhs.max;
		}
		return regularize();
	}
	IntRange& IntRange::operator-=(const IntRange& rhs) {
		return this->operator+=(-rhs);
	}
	IntRange& IntRange::operator*=(const IntRange& rhs) {
		if (is_bottom() || rhs.is_bottom()) {
			make_bottom();
		} else {
			SaturatingInt r1 = min * rhs.min;
			SaturatingInt r2 = min * rhs.max;
			SaturatingInt r3 = max * rhs.min;
			SaturatingInt r4 = max * rhs.max;
			make_bottom();
			if (r1.is_valid() && r2.is_invalid() && r3.is_invalid() && r4.is_invalid()) {
				join(r1);
				join(r2);
				join(r3);
				join(r4);
			}
		}
		return regularize();
	}
	IntRange& IntRange::operator/=(const IntRange& rhs) {
		if (is_bottom() || rhs.is_bottom() || rhs.intersects(0)) {
			make_bottom();
		} else {
			SaturatingInt r1 = min / rhs.min;
			SaturatingInt r2 = min / rhs.max;
			SaturatingInt r3 = max / rhs.min;
			SaturatingInt r4 = max / rhs.max;
			make_bottom();
			if (r1.is_valid() && r2.is_invalid() && r3.is_invalid() && r4.is_invalid()) {
				join(r1);
				join(r2);
				join(r3);
				join(r4);
			}
		}
		return regularize();
	}

	IntRange IntRange::abs(void) const {
		if (is_bottom()) {
			return IntRange::bottom();
		} else if (is_positive()) {
			return IntRange(*this);
		} else if (is_negative()) {
			return IntRange(*this).negate();
		} else {
			SaturatingInt newMax = SaturatingInt::max(min.abs(), max.abs());
			return IntRange(0, newMax);
		}
	}
	IntRange IntRange::operator-(void) const {
		return IntRange(*this).negate();
	}
	IntRange IntRange::operator+(const IntRange& rhs) const {
		return IntRange(*this).operator+(rhs);
	}
	IntRange IntRange::operator-(const IntRange& rhs) const {
		return IntRange(*this).operator-(rhs);
	}
	IntRange IntRange::operator*(const IntRange& rhs) const {
		return IntRange(*this).operator*(rhs);
	}
	IntRange IntRange::operator/(const IntRange& rhs) const {
		return IntRange(*this).operator/(rhs);
	}

	IntRange& IntRange::operator+=(const int rhs) {
		return this->operator+=(IntRange(rhs));
	}
	IntRange& IntRange::operator-=(const int rhs) {
		return this->operator-=(IntRange(rhs));
	}
	IntRange& IntRange::operator*=(const int rhs) {
		return this->operator*=(IntRange(rhs));
	}
	IntRange& IntRange::operator/=(const int rhs) {
		return this->operator/=(IntRange(rhs));
	}
	IntRange IntRange::operator+(const int rhs) const {
		return this->operator+(IntRange(rhs));
	}
	IntRange IntRange::operator-(const int rhs) const {
		return this->operator-(IntRange(rhs));
	}
	IntRange IntRange::operator*(const int rhs) const {
		return this->operator*(IntRange(rhs));
	}
	IntRange IntRange::operator/(const int rhs) const {
		return this->operator/(IntRange(rhs));
	}

	IntRange operator+(const int lhs, const IntRange& rhs) {
		return IntRange(lhs).operator+(rhs);
	}
	IntRange operator-(const int lhs, const IntRange& rhs) {
		return IntRange(lhs).operator-(rhs);
	}
	IntRange operator*(const int lhs, const IntRange& rhs) {
		return IntRange(lhs).operator*(rhs);
	}
	IntRange operator/(const int lhs, const IntRange& rhs) {
		return IntRange(lhs).operator/(rhs);
	}

	IntRange& IntRange::join(const IntRange& rhs) {
		if (!rhs.is_bottom()) {
			if (is_bottom()) {
				min = rhs.min;
				max = rhs.max;
			} else {
				min.min(rhs.min);
				max.max(rhs.max);
			}
		}
		return regularize();
	}
	IntRange& IntRange::intersect(const IntRange& rhs) {
		if (is_bottom() || rhs.is_bottom()) {
			make_bottom();
		} else {
			min.max(rhs.min);
			max.min(rhs.max);
		}
		return regularize();
	}
	IntRange& IntRange::difference(const IntRange& rhs) {
		if (!is_bottom() && !rhs.is_bottom()) {
			if (rhs.intersects(min)) {
				min = rhs.max + 1;
			}
			if (rhs.intersects(max)) {
				max = rhs.min - 1;
			}
		}
		return regularize();
	}

	IntRange& IntRange::join(const SaturatingInt& rhs) {
		return this->join(IntRange(rhs));
	}
	IntRange& IntRange::intersect(const SaturatingInt& rhs) {
		return this->intersect(IntRange(rhs));
	}
	IntRange& IntRange::difference(const SaturatingInt& rhs) {
		return this->difference(IntRange(rhs));
	}
	IntRange& IntRange::join(const int rhs) {
		return this->join(IntRange(rhs));
	}
	IntRange& IntRange::intersect(const int rhs) {
		return this->intersect(IntRange(rhs));
	}
	IntRange& IntRange::difference(const int rhs) {
		return this->difference(IntRange(rhs));
	}

	bool IntRange::intersects(const IntRange& other) const {
		return !IntRange(*this).intersect(other).is_bottom();
	}
	bool IntRange::intersects(const SaturatingInt& other) const {
		return this->intersects(IntRange(other));
	}
	bool IntRange::intersects(const int other) const {
		return this->intersects(IntRange(other));
	}


	// ---------------
	// Float Range
	// ---------------

	BoolRange FloatRange::operator==(const FloatRange& rhs) const {
		if (is_bottom() || rhs.is_bottom()) {
			return BoolRange::bottom();
		}
		
		if (intersects(rhs)) {
			if (is_exact() && rhs.is_exact()) {
				return BoolRange(true);
			}
			return BoolRange::top();
		} else {
			return BoolRange(false);
		}
	}
	BoolRange FloatRange::operator!=(const FloatRange& rhs) const {
		return !this->operator==(rhs);
	}
	BoolRange FloatRange::operator>(const FloatRange& rhs) const {
		if (is_bottom() || rhs.is_bottom()) {
			return BoolRange::bottom();
		}

		if (min > rhs.max) {
			return BoolRange(true);
		} else if (max > rhs.min) {
			return BoolRange::top();
		} else {
			return BoolRange(false);
		}
	}
	BoolRange FloatRange::operator<(const FloatRange& rhs) const {
		if (is_bottom() || rhs.is_bottom()) {
			return BoolRange::bottom();
		}

		if (max < rhs.min) {
			return BoolRange(true);
		} else if (min < rhs.max) {
			return BoolRange::top();
		} else {
			return BoolRange(false);
		}
	}
	BoolRange FloatRange::operator>=(const FloatRange& rhs) const {
		return !this->operator<(rhs);
	}
	BoolRange FloatRange::operator<=(const FloatRange& rhs) const {
		return !this->operator>(rhs);
	}


	// TODO: fix these
	FloatRange abs(const FloatRange& lhs) {
		return FloatRange();
	}
	FloatRange operator-(const FloatRange& lhs) {
		return FloatRange();
	}
	FloatRange operator+(const FloatRange& lhs, const FloatRange& rhs) {
		return FloatRange();
	}
	FloatRange operator-(const FloatRange& lhs, const FloatRange& rhs) {
		return FloatRange();
	}
	FloatRange operator*(const FloatRange& lhs, const FloatRange& rhs) {
		return FloatRange();
	}
	FloatRange operator/(const FloatRange& lhs, const FloatRange& rhs) {
		return FloatRange();
	}
}