//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2019 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <cstdint>
#include <cmath>
#include <type_traits>

namespace z8
{

struct fix32
{
    inline fix32() = default;

    /* Convert from/to double */
    inline fix32(double d)
      : m_bits((int32_t)std::floor(d * 65536.0))
    {}

    inline operator double() const
    {
        return (double)m_bits * (1.0 / 65536.0); 
    }


    /* Directly initialise bits */
    static inline fix32 frombits(int32_t x)
    {
        fix32 ret; ret.m_bits = x; return ret;
    }

    inline int32_t bits() const { return m_bits; }
	inline int32_t toInt() const {return m_bits >> 16;}

    /* Comparisons */
    bool operator ==(fix32 x) const { return m_bits == x.m_bits; }
    bool operator !=(fix32 x) const { return m_bits != x.m_bits; }
    bool operator  <(fix32 x) const { return m_bits  < x.m_bits; }
    bool operator  >(fix32 x) const { return m_bits  > x.m_bits; }
    bool operator <=(fix32 x) const { return m_bits <= x.m_bits; }
    bool operator >=(fix32 x) const { return m_bits >= x.m_bits; }

    /* Increments */
    fix32& operator ++() { m_bits += 0x10000; return *this; }
    fix32& operator --() { m_bits -= 0x10000; return *this; }
    fix32 operator ++(int) { fix32 ret = *this; ++*this; return ret; }
    fix32 operator --(int) { fix32 ret = *this; --*this; return ret; }

    /* Math operations */
    fix32 const &operator +() const { return *this; }
    fix32 operator -() const { return frombits(-m_bits); }
    fix32 operator ~() const { return frombits(~m_bits); }
    fix32 operator +(fix32 x) const { return frombits(m_bits + x.m_bits); }
	fix32 operator+(int x) const { return frombits(m_bits + 0x10000*x);	}
	fix32 operator -(fix32 x) const { return frombits(m_bits - x.m_bits); }
    fix32 operator &(fix32 x) const { return frombits(m_bits & x.m_bits); }

	fix32 operator |(fix32 x) const { return frombits(m_bits | x.m_bits); }
    fix32 operator ^(fix32 x) const { return frombits(m_bits ^ x.m_bits); }

    fix32 operator *(fix32 x) const
    {
        return frombits((int64_t)m_bits * x.m_bits / 0x10000);
    }

    fix32 operator /(fix32 x) const
    {
        /* XXX: PICO-8 returns 0x8000.0001 instead of 0x8000.0000 */
        if (x.m_bits == 0)
            return frombits(m_bits >= 0 ? 0x7fffffff : 0x80000001);
        return frombits((int64_t)m_bits * 0x10000 / x.m_bits);
    }

    inline fix32& operator +=(fix32 x) { return *this = *this + x; }
    inline fix32& operator -=(fix32 x) { return *this = *this - x; }
    inline fix32& operator &=(fix32 x) { return *this = *this & x; }
    inline fix32& operator |=(fix32 x) { return *this = *this | x; }
    inline fix32& operator ^=(fix32 x) { return *this = *this ^ x; }
    inline fix32& operator *=(fix32 x) { return *this = *this * x; }
    inline fix32& operator /=(fix32 x) { return *this = *this / x; }

    /* Free functions */
    static fix32 abs(fix32 a) { return a.m_bits > 0 ? a : -a; }
    static fix32 min(fix32 a, fix32 b) { return a < b ? a : b; }
    static fix32 max(fix32 a, fix32 b) { return a > b ? a : b; }

    static fix32 ceil(fix32 x) { return -floor(-x); }
    static fix32 floor(fix32 x) { return frombits(x.m_bits & 0xffff0000); }

    static fix32 pow(fix32 x, fix32 y) { return fix32(std::pow((double)x, (double)y)); }
	static fix32 fabs(fix32 x) { return fix32(std::fabs((double)x));	}
	static fix32 sin(fix32 x) {	return fix32(std::sin((double)x));}
	static fix32 sinh(fix32 x) { return fix32(std::sinh((double)x)); }
	
	static fix32 asin(fix32 x) { return fix32(std::asin((double)x)); }

	static fix32 cos(fix32 x) {	return fix32(std::cos((double)x)); }
	static fix32 cosh(fix32 x) { return fix32(std::cosh((double)x)); }
	static fix32 acos(fix32 x) { return fix32(std::acos((double)x)); }
	static fix32 tan(fix32 x) { return fix32(std::tan((double)x)); }
	static fix32 tanh(fix32 x) { return fix32(std::tanh((double)x)); }
	static fix32 atan(fix32 x) { return fix32(std::atan((double)x)); }
	static fix32 atan2(fix32 x, fix32 y) { return fix32(std::atan2((double)x, (double)y)); }
	static fix32 fmod(fix32 x, fix32 y) { return fix32(std::fmod((double)x, (double)y)); }
	static fix32 modf(fix32 x, fix32* y) { 
		double dy;
		fix32 r = fix32(std::modf((double)x, &dy));
		*y = fix32(dy);
		return r;
	}

	static fix32 sqrt(fix32 x) { return fix32(std::sqrt((double)x)); }
	static fix32 log(fix32 x) {	return fix32(std::log((double)x)); }
	static fix32 log10(fix32 x) { return fix32(std::log10((double)x)); }
	static fix32 exp(fix32 x) {	return fix32(std::exp((double)x)); }
	static fix32 frexp(fix32 x, int* i) { return fix32(std::frexp((double)x, i)); }
	static fix32 ldexp(fix32 x, int i) { return fix32(std::ldexp((double)x, i)); }


private:
    int32_t m_bits;
};

}

