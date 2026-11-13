#include "Math5D.h"
#include <glm/common.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

namespace m5
{
	inline float det3(
		float a00, float a01, float a02,
		float a10, float a11, float a12,
		float a20, float a21, float a22)
	{
		return a00 * (a11 * a22 - a12 * a21)
			- a01 * (a10 * a22 - a12 * a20)
			+ a02 * (a10 * a21 - a11 * a20);
	}
	inline float det4(
		float a00, float a01, float a02, float a03,
		float a10, float a11, float a12, float a13,
		float a20, float a21, float a22, float a23,
		float a30, float a31, float a32, float a33)
	{
		return
			a00 * det3(a11, a12, a13, a21, a22, a23, a31, a32, a33)
			- a01 * det3(a10, a12, a13, a20, a22, a23, a30, a32, a33)
			+ a02 * det3(a10, a11, a13, a20, a21, a23, a30, a31, a33)
			- a03 * det3(a10, a11, a12, a20, a21, a22, a30, a31, a32);
	}
	inline float det4_cols(const vec5& r0, const vec5& r1, const vec5& r2, const vec5& r3, int c0, int c1, int c2, int c3)
	{
		return det4(
			r0[c0], r0[c1], r0[c2], r0[c3],
			r1[c0], r1[c1], r1[c2], r1[c3],
			r2[c0], r2[c1], r2[c2], r2[c3],
			r3[c0], r3[c1], r3[c2], r3[c3]
		);
	}

	vec5 cross(const vec5& a, const vec5& b, const vec5& c, const vec5& d)
	{
		float ca = det4_cols(a, b, c, d, 1, 2, 3, 4);
		float cb = -det4_cols(a, b, c, d, 0, 2, 3, 4);
		float cc = det4_cols(a, b, c, d, 0, 1, 3, 4);
		float cd = -det4_cols(a, b, c, d, 0, 1, 2, 4);
		float ce = det4_cols(a, b, c, d, 0, 1, 2, 3);
		return vec5{ ca, cb, cc, cd, ce };
	}

	glm::vec4 cross(const glm::vec4& u, const glm::vec4& v, const glm::vec4& w)
	{
		// intermediate values
		float a = (v.x * w.y) - (v.y * w.x);
		float b = (v.x * w.z) - (v.z * w.x);
		float c = (v.x * w.w) - (v.w * w.x);
		float d = (v.y * w.z) - (v.z * w.y);
		float e = (v.y * w.w) - (v.w * w.y);
		float f = (v.z * w.w) - (v.w * w.z);

		// result vector
		glm::vec4 res;

		res.x = (u.y * f) - (u.z * e) + (u.w * d);
		res.y = -(u.x * f) + (u.z * c) - (u.w * b);
		res.z = (u.x * e) - (u.y * c) + (u.w * a);
		res.w = -(u.x * d) + (u.y * b) - (u.z * a);

		return res;
	}

	bivec5::bivec5(const nlohmann::json& j)
	{
		ab = j[0].get<float>();
		ac = j[1].get<float>();
		ad = j[2].get<float>();
		ae = j[3].get<float>();
		bc = j[4].get<float>();
		bd = j[5].get<float>();
		be = j[6].get<float>();
		cd = j[7].get<float>();
		ce = j[8].get<float>();
		de = j[9].get<float>();
	}

	nlohmann::json bivec5::toJson() const
	{
		return { ab, ac, ad, ae, bc, bd, be, cd, ce, de };
	}

	bivec5::bivec5(const bivec5& other)
		: xy(other.xy), xz(other.xz), xw(other.xw), xv(other.xv), yz(other.yz), yw(other.yw), yv(other.yv), zw(other.zw), zv(other.zv), wv(other.wv) {
	}
	bivec5::bivec5(bivec5&& other) noexcept
		: xy(other.xy), xz(other.xz), xw(other.xw), xv(other.xv), yz(other.yz), yw(other.yw), yv(other.yv), zw(other.zw), zv(other.zv), wv(other.wv)
	{
		other.xy = 0;
		other.xz = 0;
		other.xw = 0;
		other.xv = 0;
		other.yz = 0;
		other.yw = 0;
		other.yv = 0;
		other.zw = 0;
		other.zv = 0;
		other.wv = 0;
	}
	bivec5& bivec5::operator=(const bivec5& other)
	{
		this->xy = other.xy;
		this->xz = other.xz;
		this->xw = other.xw;
		this->xv = other.xv;
		this->yz = other.yz;
		this->yw = other.yw;
		this->yv = other.yv;
		this->zw = other.zw;
		this->zv = other.zv;
		this->wv = other.wv;

		return *this;
	}
	bivec5& bivec5::operator=(bivec5&& other) noexcept
	{
		if (this != &other)
		{
			this->xy = other.xy;
			this->xz = other.xz;
			this->xw = other.xw;
			this->xv = other.xv;
			this->yz = other.yz;
			this->yw = other.yw;
			this->yv = other.yv;
			this->zw = other.zw;
			this->zv = other.zv;
			this->wv = other.wv;

			other.xy = 0;
			other.xz = 0;
			other.xw = 0;
			other.xv = 0;
			other.yz = 0;
			other.yw = 0;
			other.yv = 0;
			other.zw = 0;
			other.zv = 0;
			other.wv = 0;
		}

		return *this;
	}

	bivec5 bivec5::operator+(const bivec5& other) const
	{
		return bivec5
		{
			this->xy + other.xy,
			this->xz + other.xz,
			this->xw + other.xw,
			this->xv + other.xv,
			this->yz + other.yz,
			this->yw + other.yw,
			this->yv + other.yv,
			this->zw + other.zw,
			this->zv + other.zv,
			this->wv + other.wv
		};
	}
	bivec5 bivec5::operator+(float v) const
	{
		return bivec5
		{
			this->xy + v,
			this->xz + v,
			this->xw + v,
			this->xv + v,
			this->yz + v,
			this->yw + v,
			this->yv + v,
			this->zw + v,
			this->zv + v,
			this->wv + v
		};
	}
	bivec5 bivec5::operator-(const bivec5& other) const
	{
		return bivec5
		{
			this->xy - other.xy,
			this->xz - other.xz,
			this->xw - other.xw,
			this->xv - other.xv,
			this->yz - other.yz,
			this->yw - other.yw,
			this->yv - other.yv,
			this->zw - other.zw,
			this->zv - other.zv,
			this->wv - other.wv
		};
	}
	bivec5 bivec5::operator-(float v) const
	{
		return bivec5
		{
			this->xy - v,
			this->xz - v,
			this->xw - v,
			this->xv - v,
			this->yz - v,
			this->yw - v,
			this->yv - v,
			this->zw - v,
			this->zv - v,
			this->wv - v
		};
	}
	bivec5 bivec5::operator-() const
	{
		return bivec5
		{
			-this->xy,
			-this->xz,
			-this->xw,
			-this->xv,
			-this->yz,
			-this->yw,
			-this->yv,
			-this->zw,
			-this->zv,
			-this->wv
		};
	}
	bivec5 bivec5::operator*(const bivec5& other) const
	{
		return bivec5
		{
			this->xy * other.xy,
			this->xz * other.xz,
			this->xw * other.xw,
			this->xv * other.xv,
			this->yz * other.yz,
			this->yw * other.yw,
			this->yv * other.yv,
			this->zw * other.zw,
			this->zv * other.zv,
			this->wv * other.wv
		};
	}
	bivec5 bivec5::operator*(float v) const
	{
		return bivec5
		{
			this->xy * v,
			this->xz * v,
			this->xw * v,
			this->xv * v,
			this->yz * v,
			this->yw * v,
			this->yv * v,
			this->zw * v,
			this->zv * v,
			this->wv * v
		};
	}
	bivec5 bivec5::operator/(const bivec5& other) const
	{
		return bivec5
		{
			this->xy / other.xy,
			this->xz / other.xz,
			this->xw / other.xw,
			this->xv / other.xv,
			this->yz / other.yz,
			this->yw / other.yw,
			this->yv / other.yv,
			this->zw / other.zw,
			this->zv / other.zv,
			this->wv / other.wv
		};
	}
	bivec5 bivec5::operator/(float v) const
	{
		float invV = 1.0f / v;
		return bivec5
		{
			this->xy * invV,
			this->xz * invV,
			this->xw * invV,
			this->xv * invV,
			this->yz * invV,
			this->yw * invV,
			this->yv * invV,
			this->zw * invV,
			this->zv * invV,
			this->wv * invV
		};
	}

	bivec5& bivec5::operator+=(const bivec5& other) { return *this = *this + other; }
	bivec5& bivec5::operator+=(float v) { return *this = *this + v; }
	bivec5& bivec5::operator-=(const bivec5& other) { return *this = *this - other; }
	bivec5& bivec5::operator-=(float v) { return *this = *this - v; }
	bivec5& bivec5::operator*=(const bivec5& other) { return *this = *this * other; }
	bivec5& bivec5::operator*=(float v) { return *this = *this * v; }
	bivec5& bivec5::operator/=(const bivec5& other) { return *this = *this / other; }
	bivec5& bivec5::operator/=(float v) { return *this = *this / v; }

	bool bivec5::operator==(const bivec5& other) const
	{
		return this->xy == other.xy && this->xz == other.xz && this->xw == other.xw && this->xv == other.xv &&
			this->yz == other.yz && this->yw == other.yw && this->yv == other.yv && this->zw == other.zw && this->zv == other.zv && this->wv == other.wv;
	}

	bivec5 bivec5::normalized() const
	{
		bivec5 v = *this;
		v.normalize();
		return v;
	}
	bivec5& bivec5::normalize()
	{
		float len = length2(*this);
		if (len <= glm::epsilon<float>() * glm::epsilon<float>())
		{
			return *this;
		}
		return *this /= glm::sqrt(len);
	}
	float dot(const bivec5& a, const bivec5& b)
	{
		return
			a.xy * b.xy + a.xz * b.xz + a.xw * b.xw + a.xv * b.xv +
			a.yz * b.yz + a.yw * b.yw + a.yv * b.yv +
			a.zw * b.zw + a.zv * b.zv +
			a.wv * b.wv;
	}
	bivec5 normalize(const bivec5& v)
	{
		return v.normalized();
	}
	float length(const bivec5& v)
	{
		return glm::sqrt(length2(v));
	}
	float length2(const bivec5& v)
	{
		return dot(v, v);
	}

	bivec5 wedge(const vec5& a, const vec5& b)
	{
		return bivec5{
			a.a * b.b - a.b * b.a,
			a.a * b.c - a.c * b.a,
			a.a * b.d - a.d * b.a,
			a.a * b.e - a.e * b.a,
			a.b * b.c - a.c * b.b,
			a.b * b.d - a.d * b.b,
			a.b * b.e - a.e * b.b,
			a.c * b.d - a.d * b.c,
			a.c * b.e - a.e * b.c,
			a.d * b.e - a.e * b.d,
		}.normalized();
	}

	rotor5::rotor5(const vec5& from, const vec5& to)
	{
		q = {};

		float d = glm::clamp(dot(from, to), -1.0f, 1.0f);
		if (d == 1.0f)
		{
			s = 1.0f;
			b = {};
			return;
		}

		float radians = glm::acos(d);
		s = glm::cos(radians / 2.0f);
		float sina = glm::sin(radians / 2.0f);

		b = wedge(to, from);
		b.ab *= sina;
		b.ac *= sina;
		b.ad *= sina;
		b.ae *= sina;
		b.bc *= sina;
		b.bd *= sina;
		b.be *= sina;
		b.cd *= sina;
		b.ce *= sina;
		b.de *= sina;

		normalize();
	}

	rotor5::rotor5(const bivec5& plane, float radians)
	{
		float sina = glm::sin(radians / 2.0f);
		s = glm::cos(radians / 2.0f);
		b.ab = -sina * plane.ab;
		b.ac = -sina * plane.ac;
		b.ad = -sina * plane.ad;
		b.ae = -sina * plane.ae;
		b.bc = -sina * plane.bc;
		b.bd = -sina * plane.bd;
		b.be = -sina * plane.be;
		b.cd = -sina * plane.cd;
		b.ce = -sina * plane.ce;
		b.de = -sina * plane.de;
		q = {};
	}

	rotor5::rotor5(const rotor5& from, const rotor5& to, float t)
	{
		// dot product
		float d =
			from.s * to.s +
			from.b.ab * to.b.ab +
			from.b.ac * to.b.ac +
			from.b.ad * to.b.ad +
			from.b.ae * to.b.ae +
			from.b.bc * to.b.bc +
			from.b.bd * to.b.bd +
			from.b.be * to.b.be +
			from.b.cd * to.b.cd +
			from.b.ce * to.b.ce +
			from.b.de * to.b.de +
			from.q.abcd * to.q.abcd +
			from.q.abce * to.q.abce +
			from.q.abde * to.q.abde +
			from.q.acde * to.q.acde +
			from.q.bcde * to.q.bcde;

		// flip dst if "dot" is negative
		rotor5 dst = to;
		if (d < 0.0f)
		{
			dst.s *= -1.0f;
			dst.b.ab *= -1.0f;
			dst.b.ac *= -1.0f;
			dst.b.ad *= -1.0f;
			dst.b.ae *= -1.0f;
			dst.b.bc *= -1.0f;
			dst.b.bd *= -1.0f;
			dst.b.be *= -1.0f;
			dst.b.cd *= -1.0f;
			dst.b.ce *= -1.0f;
			dst.b.de *= -1.0f;
			dst.q.abcd *= -1.0f;
			dst.q.abce *= -1.0f;
			dst.q.abde *= -1.0f;
			dst.q.acde *= -1.0f;
			dst.q.bcde *= -1.0f;

			d *= -1.0f;
		}

		if (d > 0.99995f)
		{
			// nlerp if the rotors are too close
			s = glm::mix(from.s, dst.s, t);
			b.ab = glm::mix(from.b.ab, dst.b.ab, t);
			b.ac = glm::mix(from.b.ac, dst.b.ac, t);
			b.ad = glm::mix(from.b.ad, dst.b.ad, t);
			b.ae = glm::mix(from.b.ae, dst.b.ae, t);
			b.bc = glm::mix(from.b.bc, dst.b.bc, t);
			b.bd = glm::mix(from.b.bd, dst.b.bd, t);
			b.be = glm::mix(from.b.be, dst.b.be, t);
			b.cd = glm::mix(from.b.cd, dst.b.cd, t);
			b.ce = glm::mix(from.b.ce, dst.b.ce, t);
			b.de = glm::mix(from.b.de, dst.b.de, t);
			q.abcd = glm::mix(from.q.abcd, dst.q.abcd, t);
			q.abce = glm::mix(from.q.abce, dst.q.abce, t);
			q.abde = glm::mix(from.q.abde, dst.q.abde, t);
			q.acde = glm::mix(from.q.acde, dst.q.acde, t);
			q.bcde = glm::mix(from.q.bcde, dst.q.bcde, t);

			normalize();
			return;
		}

		// assume that "from" and "dst" both have a magnitude of 1,
		// i.e. they are the product of two unit vectors,
		// then cos(theta) = dot(from, dst)
		const float theta = glm::acos(d);
		const float fromFactor = glm::sin((1.0f - t) * theta) / glm::sin(theta);
		const float toFactor = glm::sin(t * theta) / glm::sin(theta);

		s = fromFactor * from.s + toFactor * dst.s;
		b.ab = fromFactor * from.b.ab + toFactor * dst.b.ab;
		b.ac = fromFactor * from.b.ac + toFactor * dst.b.ac;
		b.ad = fromFactor * from.b.ad + toFactor * dst.b.ad;
		b.ae = fromFactor * from.b.ae + toFactor * dst.b.ae;
		b.bc = fromFactor * from.b.bc + toFactor * dst.b.bc;
		b.bd = fromFactor * from.b.bd + toFactor * dst.b.bd;
		b.be = fromFactor * from.b.be + toFactor * dst.b.be;
		b.cd = fromFactor * from.b.cd + toFactor * dst.b.cd;
		b.ce = fromFactor * from.b.ce + toFactor * dst.b.ce;
		b.de = fromFactor * from.b.de + toFactor * dst.b.de;
		q.abcd = fromFactor * from.q.abcd + toFactor * dst.q.abcd;
		q.abce = fromFactor * from.q.abce + toFactor * dst.q.abce;
		q.abde = fromFactor * from.q.abde + toFactor * dst.q.abde;
		q.acde = fromFactor * from.q.acde + toFactor * dst.q.acde;
		q.bcde = fromFactor * from.q.bcde + toFactor * dst.q.bcde;
	}

	rotor5::rotor5(const rotor5& other)
		: s(other.s), b(other.b), q(other.q) {
	}

	rotor5::rotor5(rotor5&& other) noexcept
		: s(other.s), b(other.b), q(other.q)
	{
		other.s = 1.0f;
		other.b = 0.0f;
		other.q = {};
	}

	rotor5& rotor5::operator=(const rotor5& other)
	{
		this->s = other.s;
		this->b = other.b;
		this->q = other.q;
		return *this;
	}

	rotor5& rotor5::operator=(rotor5&& other) noexcept
	{
		if (this != &other)
		{
			this->s = other.s;
			this->b = other.b;
			this->q = other.q;

			other.s = 1.0f;
			other.b = 0.0f;
			other.q = {};
		}
		return *this;
	}

	rotor5::rotor5(const nlohmann::json& j)
	{
		s = j[0].get<float>();
		b.ab = j[1].get<float>();
		b.ac = j[2].get<float>();
		b.ad = j[3].get<float>();
		b.ae = j[4].get<float>();
		b.bc = j[5].get<float>();
		b.bd = j[6].get<float>();
		b.be = j[7].get<float>();
		b.cd = j[8].get<float>();
		b.ce = j[9].get<float>();
		b.de = j[10].get<float>();
		q.abcd = j[11].get<float>();
		q.abce = j[12].get<float>();
		q.abde = j[13].get<float>();
		q.acde = j[14].get<float>();
		q.bcde = j[15].get<float>();
	}

	nlohmann::json rotor5::toJson() const
	{
		return { s, b.ab, b.ac, b.ad, b.ae, b.bc, b.bd, b.be, b.cd, b.ce, b.de, q.abcd, q.abce, q.abde, q.acde, q.bcde };
	}

	rotor5 rotor5::operator*(const rotor5& r) const
	{
		return
		{
			-b.ab * r.b.ab - b.ac * r.b.ac - b.ad * r.b.ad - b.ae * r.b.ae - b.bc * r.b.bc - b.bd * r.b.bd - b.be * r.b.be - b.cd * r.b.cd - b.ce * r.b.ce - b.de * r.b.de + q.abcd * r.q.abcd + q.abce * r.q.abce + q.abde * r.q.abde + q.acde * r.q.acde + q.bcde * r.q.bcde + s * r.s,
			bivec5{
				b.ab * r.s - b.ac * r.b.bc - b.ad * r.b.bd - b.ae * r.b.be + b.bc * r.b.ac + b.bd * r.b.ad + b.be * r.b.ae - b.cd * r.q.abcd - b.ce * r.q.abce - b.de * r.q.abde - q.abcd * r.b.cd - q.abce * r.b.ce - q.abde * r.b.de + q.acde * r.q.bcde - q.bcde * r.q.acde + s * r.b.ab,
				b.ab * r.b.bc + b.ac * r.s - b.ad * r.b.cd - b.ae * r.b.ce - b.bc * r.b.ab + b.bd * r.q.abcd + b.be * r.q.abce + b.cd * r.b.ad + b.ce * r.b.ae - b.de * r.q.acde + q.abcd * r.b.bd + q.abce * r.b.be - q.abde * r.q.bcde - q.acde * r.b.de + q.bcde * r.q.abde + s * r.b.ac,
				b.ab * r.b.bd + b.ac * r.b.cd + b.ad * r.s - b.ae * r.b.de - b.bc * r.q.abcd - b.bd * r.b.ab + b.be * r.q.abde - b.cd * r.b.ac + b.ce * r.q.acde + b.de * r.b.ae - q.abcd * r.b.bc + q.abce * r.q.bcde + q.abde * r.b.be + q.acde * r.b.ce - q.bcde * r.q.abce + s * r.b.ad,
				b.ab * r.b.be + b.ac * r.b.ce + b.ad * r.b.de + b.ae * r.s - b.bc * r.q.abce - b.bd * r.q.abde - b.be * r.b.ab - b.cd * r.q.acde - b.ce * r.b.ac - b.de * r.b.ad - q.abcd * r.q.bcde - q.abce * r.b.bc - q.abde * r.b.bd - q.acde * r.b.cd + q.bcde * r.q.abcd + s * r.b.ae,
				-b.ab * r.b.ac + b.ac * r.b.ab - b.ad * r.q.abcd - b.ae * r.q.abce + b.bc * r.s - b.bd * r.b.cd - b.be * r.b.ce + b.cd * r.b.bd + b.ce * r.b.be - b.de * r.q.bcde - q.abcd * r.b.ad - q.abce * r.b.ae + q.abde * r.q.acde - q.acde * r.q.abde - q.bcde * r.b.de + s * r.b.bc,
				-b.ab * r.b.ad + b.ac * r.q.abcd + b.ad * r.b.ab - b.ae * r.q.abde + b.bc * r.b.cd + b.bd * r.s - b.be * r.b.de - b.cd * r.b.bc + b.ce * r.q.bcde + b.de * r.b.be + q.abcd * r.b.ac - q.abce * r.q.acde - q.abde * r.b.ae + q.acde * r.q.abce + q.bcde * r.b.ce + s * r.b.bd,
				-b.ab * r.b.ae + b.ac * r.q.abce + b.ad * r.q.abde + b.ae * r.b.ab + b.bc * r.b.ce + b.bd * r.b.de + b.be * r.s - b.cd * r.q.bcde - b.ce * r.b.bc - b.de * r.b.bd + q.abcd * r.q.acde + q.abce * r.b.ac + q.abde * r.b.ad - q.acde * r.q.abcd - q.bcde * r.b.cd + s * r.b.be,
				-b.ab * r.q.abcd - b.ac * r.b.ad + b.ad * r.b.ac - b.ae * r.q.acde - b.bc * r.b.bd + b.bd * r.b.bc - b.be * r.q.bcde + b.cd * r.s - b.ce * r.b.de + b.de * r.b.ce - q.abcd * r.b.ab + q.abce * r.q.abde - q.abde * r.q.abce - q.acde * r.b.ae - q.bcde * r.b.be + s * r.b.cd,
				-b.ab * r.q.abce - b.ac * r.b.ae + b.ad * r.q.acde + b.ae * r.b.ac - b.bc * r.b.be + b.bd * r.q.bcde + b.be * r.b.bc + b.cd * r.b.de + b.ce * r.s - b.de * r.b.cd - q.abcd * r.q.abde - q.abce * r.b.ab + q.abde * r.q.abcd + q.acde * r.b.ad + q.bcde * r.b.bd + s * r.b.ce,
				-b.ab * r.q.abde - b.ac * r.q.acde - b.ad * r.b.ae + b.ae * r.b.ad - b.bc * r.q.bcde - b.bd * r.b.be + b.be * r.b.bd - b.cd * r.b.ce + b.ce * r.b.cd + b.de * r.s + q.abcd * r.q.abce - q.abce * r.q.abcd - q.abde * r.b.ab - q.acde * r.b.ac - q.bcde * r.b.bc + s * r.b.de,
			},
			quadvec5{
				b.ab * r.b.cd - b.ac * r.b.bd + b.ad * r.b.bc - b.ae * r.q.bcde + b.bc * r.b.ad - b.bd * r.b.ac + b.be * r.q.acde + b.cd * r.b.ab - b.ce * r.q.abde + b.de * r.q.abce + q.abcd * r.s - q.abce * r.b.de + q.abde * r.b.ce - q.acde * r.b.be + q.bcde * r.b.ae + s * r.q.abcd,
				b.ab * r.b.ce - b.ac * r.b.be + b.ad * r.q.bcde + b.ae * r.b.bc + b.bc * r.b.ae - b.bd * r.q.acde - b.be * r.b.ac + b.cd * r.q.abde + b.ce * r.b.ab - b.de * r.q.abcd + q.abcd * r.b.de + q.abce * r.s - q.abde * r.b.cd + q.acde * r.b.bd - q.bcde * r.b.ad + s * r.q.abce,
				b.ab * r.b.de - b.ac * r.q.bcde - b.ad * r.b.be + b.ae * r.b.bd + b.bc * r.q.acde + b.bd * r.b.ae - b.be * r.b.ad - b.cd * r.q.abce + b.ce * r.q.abcd + b.de * r.b.ab - q.abcd * r.b.ce + q.abce * r.b.cd + q.abde * r.s - q.acde * r.b.bc + q.bcde * r.b.ac + s * r.q.abde,
				b.ab * r.q.bcde + b.ac * r.b.de - b.ad * r.b.ce + b.ae * r.b.cd - b.bc * r.q.abde + b.bd * r.q.abce - b.be * r.q.abcd + b.cd * r.b.ae - b.ce * r.b.ad + b.de * r.b.ac + q.abcd * r.b.be - q.abce * r.b.bd + q.abde * r.b.bc + q.acde * r.s - q.bcde * r.b.ab + s * r.q.acde,
				-b.ab * r.q.acde + b.ac * r.q.abde - b.ad * r.q.abce + b.ae * r.q.abcd + b.bc * r.b.de - b.bd * r.b.ce + b.be * r.b.cd + b.cd * r.b.be - b.ce * r.b.bd + b.de * r.b.bc - q.abcd * r.b.ae + q.abce * r.b.ad - q.abde * r.b.ac + q.acde * r.b.ab + q.bcde * r.s + s * r.q.bcde,
			},
		};
	}

	rotor5 rotor5::operator*=(const rotor5& r)
	{
		*this = *this * r;
		return *this;
	}

	bool rotor5::operator==(const rotor5& other) const
	{
		return this->s == other.s && this->b == other.b && this->q == other.q;
	}

	rotor5 rotor5::reverse() const
	{
		return { s, -b, q };
	}

	rotor5 rotor5::rotate(const rotor5& r) const
	{
		rotor5 l = *this;
		l *= r;
		return l;
	}

	vec5 rotor5::rotate(const vec5& v) const
	{
		// TODO: optimize?
		return
		{
			-b.ab * b.ab * v.a + 2 * b.ab * b.bc * v.c + 2 * b.ab * b.bd * v.d + 2 * b.ab * b.be * v.e + 2 * b.ab * s * v.b - b.ac * b.ac * v.a - 2 * b.ac * b.bc * v.b + 2 * b.ac * b.cd * v.d + 2 * b.ac * b.ce * v.e + 2 * b.ac * s * v.c - b.ad * b.ad * v.a - 2 * b.ad * b.bd * v.b - 2 * b.ad * b.cd * v.c + 2 * b.ad * b.de * v.e + 2 * b.ad * s * v.d - b.ae * b.ae * v.a - 2 * b.ae * b.be * v.b - 2 * b.ae * b.ce * v.c - 2 * b.ae * b.de * v.d + 2 * b.ae * s * v.e + b.bc * b.bc * v.a + 2 * b.bc * q.abcd * v.d + 2 * b.bc * q.abce * v.e + b.bd * b.bd * v.a - 2 * b.bd * q.abcd * v.c + 2 * b.bd * q.abde * v.e + b.be * b.be * v.a - 2 * b.be * q.abce * v.c - 2 * b.be * q.abde * v.d + b.cd * b.cd * v.a + 2 * b.cd * q.abcd * v.b + 2 * b.cd * q.acde * v.e + b.ce * b.ce * v.a + 2 * b.ce * q.abce * v.b - 2 * b.ce * q.acde * v.d + b.de * b.de * v.a + 2 * b.de * q.abde * v.b + 2 * b.de * q.acde * v.c - q.abcd * q.abcd * v.a + 2 * q.abcd * q.bcde * v.e - q.abce * q.abce * v.a - 2 * q.abce * q.bcde * v.d - q.abde * q.abde * v.a + 2 * q.abde * q.bcde * v.c - q.acde * q.acde * v.a - 2 * q.acde * q.bcde * v.b + q.bcde * q.bcde * v.a + s * s * v.a,
			-b.ab * b.ab * v.b - 2 * b.ab * b.ac * v.c - 2 * b.ab * b.ad * v.d - 2 * b.ab * b.ae * v.e - 2 * b.ab * s * v.a + b.ac * b.ac * v.b - 2 * b.ac * b.bc * v.a - 2 * b.ac * q.abcd * v.d - 2 * b.ac * q.abce * v.e + b.ad * b.ad * v.b - 2 * b.ad * b.bd * v.a + 2 * b.ad * q.abcd * v.c - 2 * b.ad * q.abde * v.e + b.ae * b.ae * v.b - 2 * b.ae * b.be * v.a + 2 * b.ae * q.abce * v.c + 2 * b.ae * q.abde * v.d - b.bc * b.bc * v.b + 2 * b.bc * b.cd * v.d + 2 * b.bc * b.ce * v.e + 2 * b.bc * s * v.c - b.bd * b.bd * v.b - 2 * b.bd * b.cd * v.c + 2 * b.bd * b.de * v.e + 2 * b.bd * s * v.d - b.be * b.be * v.b - 2 * b.be * b.ce * v.c - 2 * b.be * b.de * v.d + 2 * b.be * s * v.e + b.cd * b.cd * v.b - 2 * b.cd * q.abcd * v.a + 2 * b.cd * q.bcde * v.e + b.ce * b.ce * v.b - 2 * b.ce * q.abce * v.a - 2 * b.ce * q.bcde * v.d + b.de * b.de * v.b - 2 * b.de * q.abde * v.a + 2 * b.de * q.bcde * v.c - q.abcd * q.abcd * v.b - 2 * q.abcd * q.acde * v.e - q.abce * q.abce * v.b + 2 * q.abce * q.acde * v.d - q.abde * q.abde * v.b - 2 * q.abde * q.acde * v.c + q.acde * q.acde * v.b - 2 * q.acde * q.bcde * v.a - q.bcde * q.bcde * v.b + s * s * v.b,
			b.ab * b.ab * v.c - 2 * b.ab * b.ac * v.b + 2 * b.ab * b.bc * v.a + 2 * b.ab * q.abcd * v.d + 2 * b.ab * q.abce * v.e - b.ac * b.ac * v.c - 2 * b.ac * b.ad * v.d - 2 * b.ac * b.ae * v.e - 2 * b.ac * s * v.a + b.ad * b.ad * v.c - 2 * b.ad * b.cd * v.a - 2 * b.ad * q.abcd * v.b - 2 * b.ad * q.acde * v.e + b.ae * b.ae * v.c - 2 * b.ae * b.ce * v.a - 2 * b.ae * q.abce * v.b + 2 * b.ae * q.acde * v.d - b.bc * b.bc * v.c - 2 * b.bc * b.bd * v.d - 2 * b.bc * b.be * v.e - 2 * b.bc * s * v.b + b.bd * b.bd * v.c - 2 * b.bd * b.cd * v.b + 2 * b.bd * q.abcd * v.a - 2 * b.bd * q.bcde * v.e + b.be * b.be * v.c - 2 * b.be * b.ce * v.b + 2 * b.be * q.abce * v.a + 2 * b.be * q.bcde * v.d - b.cd * b.cd * v.c + 2 * b.cd * b.de * v.e + 2 * b.cd * s * v.d - b.ce * b.ce * v.c - 2 * b.ce * b.de * v.d + 2 * b.ce * s * v.e + b.de * b.de * v.c - 2 * b.de * q.acde * v.a - 2 * b.de * q.bcde * v.b - q.abcd * q.abcd * v.c + 2 * q.abcd * q.abde * v.e - q.abce * q.abce * v.c - 2 * q.abce * q.abde * v.d + q.abde * q.abde * v.c - 2 * q.abde * q.acde * v.b + 2 * q.abde * q.bcde * v.a - q.acde * q.acde * v.c - q.bcde * q.bcde * v.c + s * s * v.c,
			b.ab * b.ab * v.d - 2 * b.ab * b.ad * v.b + 2 * b.ab * b.bd * v.a - 2 * b.ab * q.abcd * v.c + 2 * b.ab * q.abde * v.e + b.ac * b.ac * v.d - 2 * b.ac * b.ad * v.c + 2 * b.ac * b.cd * v.a + 2 * b.ac * q.abcd * v.b + 2 * b.ac * q.acde * v.e - b.ad * b.ad * v.d - 2 * b.ad * b.ae * v.e - 2 * b.ad * s * v.a + b.ae * b.ae * v.d - 2 * b.ae * b.de * v.a - 2 * b.ae * q.abde * v.b - 2 * b.ae * q.acde * v.c + b.bc * b.bc * v.d - 2 * b.bc * b.bd * v.c + 2 * b.bc * b.cd * v.b - 2 * b.bc * q.abcd * v.a + 2 * b.bc * q.bcde * v.e - b.bd * b.bd * v.d - 2 * b.bd * b.be * v.e - 2 * b.bd * s * v.b + b.be * b.be * v.d - 2 * b.be * b.de * v.b + 2 * b.be * q.abde * v.a - 2 * b.be * q.bcde * v.c - b.cd * b.cd * v.d - 2 * b.cd * b.ce * v.e - 2 * b.cd * s * v.c + b.ce * b.ce * v.d - 2 * b.ce * b.de * v.c + 2 * b.ce * q.acde * v.a + 2 * b.ce * q.bcde * v.b - b.de * b.de * v.d + 2 * b.de * s * v.e - q.abcd * q.abcd * v.d - 2 * q.abcd * q.abce * v.e + q.abce * q.abce * v.d - 2 * q.abce * q.abde * v.c + 2 * q.abce * q.acde * v.b - 2 * q.abce * q.bcde * v.a - q.abde * q.abde * v.d - q.acde * q.acde * v.d - q.bcde * q.bcde * v.d + s * s * v.d,
			b.ab * b.ab * v.e - 2 * b.ab * b.ae * v.b + 2 * b.ab * b.be * v.a - 2 * b.ab * q.abce * v.c - 2 * b.ab * q.abde * v.d + b.ac * b.ac * v.e - 2 * b.ac * b.ae * v.c + 2 * b.ac * b.ce * v.a + 2 * b.ac * q.abce * v.b - 2 * b.ac * q.acde * v.d + b.ad * b.ad * v.e - 2 * b.ad * b.ae * v.d + 2 * b.ad * b.de * v.a + 2 * b.ad * q.abde * v.b + 2 * b.ad * q.acde * v.c - b.ae * b.ae * v.e - 2 * b.ae * s * v.a + b.bc * b.bc * v.e - 2 * b.bc * b.be * v.c + 2 * b.bc * b.ce * v.b - 2 * b.bc * q.abce * v.a - 2 * b.bc * q.bcde * v.d + b.bd * b.bd * v.e - 2 * b.bd * b.be * v.d + 2 * b.bd * b.de * v.b - 2 * b.bd * q.abde * v.a + 2 * b.bd * q.bcde * v.c - b.be * b.be * v.e - 2 * b.be * s * v.b + b.cd * b.cd * v.e - 2 * b.cd * b.ce * v.d + 2 * b.cd * b.de * v.c - 2 * b.cd * q.acde * v.a - 2 * b.cd * q.bcde * v.b - b.ce * b.ce * v.e - 2 * b.ce * s * v.c - b.de * b.de * v.e - 2 * b.de * s * v.d + q.abcd * q.abcd * v.e - 2 * q.abcd * q.abce * v.d + 2 * q.abcd * q.abde * v.c - 2 * q.abcd * q.acde * v.b + 2 * q.abcd * q.bcde * v.a - q.abce * q.abce * v.e - q.abde * q.abde * v.e - q.acde * q.acde * v.e - q.bcde * q.bcde * v.e + s * s * v.e,
		};
		// grade-5 multivector should always be zero
		//-2 * b.ab * b.cd * v.e + 2 * b.ab * b.ce * v.d - 2 * b.ab * b.de * v.c + 2 * b.ab * q.acde * v.a + 2 * b.ab * q.bcde * v.b + 2 * b.ac * b.bd * v.e - 2 * b.ac * b.be * v.d + 2 * b.ac * b.de * v.b - 2 * b.ac * q.abde * v.a + 2 * b.ac * q.bcde * v.c - 2 * b.ad * b.bc * v.e + 2 * b.ad * b.be * v.c - 2 * b.ad * b.ce * v.b + 2 * b.ad * q.abce * v.a + 2 * b.ad * q.bcde * v.d + 2 * b.ae * b.bc * v.d - 2 * b.ae * b.bd * v.c + 2 * b.ae * b.cd * v.b - 2 * b.ae * q.abcd * v.a + 2 * b.ae * q.bcde * v.e - 2 * b.bc * b.de * v.a - 2 * b.bc * q.abde * v.b - 2 * b.bc * q.acde * v.c + 2 * b.bd * b.ce * v.a + 2 * b.bd * q.abce * v.b - 2 * b.bd * q.acde * v.d - 2 * b.be * b.cd * v.a - 2 * b.be * q.abcd * v.b - 2 * b.be * q.acde * v.e + 2 * b.cd * q.abce * v.c + 2 * b.cd * q.abde * v.d - 2 * b.ce * q.abcd * v.c + 2 * b.ce * q.abde * v.e - 2 * b.de * q.abcd * v.d - 2 * b.de * q.abce * v.e + 2 * q.abcd * s * v.e - 2 * q.abce * s * v.d + 2 * q.abde * s * v.c - 2 * q.acde * s * v.b + 2 * q.bcde * s * v.a;
	}

	rotor5 rotor5::normalized() const
	{
		rotor5 v = *this;
		v.normalize();
		return v;
	}

	rotor5& rotor5::normalize()
	{
		float len = length2(*this);
		if (len <= glm::epsilon<float>() * glm::epsilon<float>())
		{
			return *this;
		}
		len = 1.0f / glm::sqrt(len);
		this->s *= len;
		this->b *= len;
		this->q = this->q * len;
		return *this;
	}

	rotor5::operator mat5() const
	{
		return mat5
		{
			rotate({ 1, 0, 0, 0, 0 }),
			rotate({ 0, 1, 0, 0, 0 }),
			rotate({ 0, 0, 1, 0, 0 }),
			rotate({ 0, 0, 0, 1, 0 }),
			rotate({ 0, 0, 0, 0, 1 }),
		};
	}

	rotor5 normalize(const rotor5& v)
	{
		return v.normalized();
	}
	float length(const rotor5& v)
	{
		return glm::sqrt(length2(v));
	}
	float length2(const rotor5& v)
	{
		return v.s * v.s + length2(v.b) + length2(v.q);
	}

	mat5::mat5(float x)
	{
		value[0][0] =
			value[1][1] =
			value[2][2] =
			value[3][3] =
			value[4][4] = x;
	}
	mat5::mat5(const glm::mat4& m)
	{
		value[0] = m[0];
		value[1] = m[1];
		value[2] = m[2];
		value[3] = m[3];
		value[4][4] = 1.0f;
	}

	mat5::mat5(const std::array<vec5, 5>& value)
		: value(value)
	{
	}
	mat5::mat5(const vec5& m0, const vec5& m1, const vec5& m2, const vec5& m3, const vec5& m4)
	{
		value[0] = m0;
		value[1] = m1;
		value[2] = m2;
		value[3] = m3;
		value[4] = m4;
	}

	mat5::mat5(const mat5& other)
	{
		this->value = other.value;
	}
	mat5::mat5(mat5&& other) noexcept
	{
		this->value = std::move(other.value);
	}
	mat5& mat5::operator=(const mat5& other)
	{
		this->value = other.value;

		return *this;
	}
	mat5& mat5::operator=(mat5&& other) noexcept
	{
		this->value = std::move(other.value);

		return *this;
	}

	mat5 mat5::operator*(const mat5& other) const
	{
		mat5 result;

		for (int col = 0; col < 5; ++col)
		{
			for (int row = 0; row < 5; ++row)
			{
				for (int k = 0; k < 5; ++k)
				{
					result.value[col][row] += value[k][row] * other.value[col][k];
				}
			}
		}

		return result;
	}
	mat5& mat5::operator*=(const mat5& other)
	{
		return *this = *this * other;
	}

	vec5 mat5::multiply(const vec5& v) const
	{
		return vec5
		{
			value[0][0] * v[0] + value[1][0] * v[1] + value[2][0] * v[2] + value[3][0] * v[3] + value[4][0] * v[4],
			value[0][1] * v[0] + value[1][1] * v[1] + value[2][1] * v[2] + value[3][1] * v[3] + value[4][1] * v[4],
			value[0][2] * v[0] + value[1][2] * v[1] + value[2][2] * v[2] + value[3][2] * v[3] + value[4][2] * v[4],
			value[0][3] * v[0] + value[1][3] * v[1] + value[2][3] * v[2] + value[3][3] * v[3] + value[4][3] * v[4],
			value[0][4] * v[0] + value[1][4] * v[1] + value[2][4] * v[2] + value[3][4] * v[3] + value[4][4] * v[4]
		};
	}
	glm::vec4 mat5::multiply(const glm::vec4& v, float finalComp) const
	{
		return multiply(vec5{ v, finalComp }).xyzw();
	}
	vec5 mat5::operator*(const vec5& v) const
	{
		return multiply(v);
	}
	glm::vec4 mat5::operator*(const glm::vec4& v) const
	{
		return multiply(v, 1.0f);
	}

	void mat5::translate(const glm::vec4& v)
	{
		value[4][0] += (value[0][0] * v.x) + (value[1][0] * v.y) + (value[2][0] * v.z) + (value[3][0] * v.w);
		value[4][1] += (value[0][1] * v.x) + (value[1][1] * v.y) + (value[2][1] * v.z) + (value[3][1] * v.w);
		value[4][2] += (value[0][2] * v.x) + (value[1][2] * v.y) + (value[2][2] * v.z) + (value[3][2] * v.w);
		value[4][3] += (value[0][3] * v.x) + (value[1][3] * v.y) + (value[2][3] * v.z) + (value[3][3] * v.w);
	}
	void mat5::translate(float x, float y, float z, float w)
	{
		translate({ x,y,z,w });
	}
	void mat5::translate(float a)
	{
		translate(a, a, a, a);
	}
	void mat5::scale(const glm::vec4& s)
	{
		for (int row = 0; row < 5; ++row)
			for (int col = 0; col < 4; ++col)
				value[col][row] *= s[col];
	}
	void mat5::scale(float x, float y, float z, float w)
	{
		scale({ x,y,z,w });
	}
	void mat5::scale(float s)
	{
		scale(s, s, s, s);
	}

	mat5 mat5::transpose(const mat5& m)
	{
		return mat5
		{
			{ m[0][0],m[1][0],m[2][0],m[3][0],m[4][0] },
			{ m[0][1],m[1][1],m[2][1],m[3][1],m[4][1] },
			{ m[0][2],m[1][2],m[2][2],m[3][2],m[4][2] },
			{ m[0][3],m[1][3],m[2][3],m[3][3],m[4][3] },
			{ m[0][4],m[1][4],m[2][4],m[3][4],m[4][4] }
		};
	}
	mat5 mat5::inverse(const mat5& m)
	{
		mat5 s{ 1 };
		mat5 t{ m };

		// Forward elimination
		for (int i = 0; i < 5 - 1; ++i)
		{
			int pivot = i;
			float pivotsize = t[i][i];

			for (int j = i + 1; j < 5; ++j)
			{
				float tmp = t[j][i];
				if (fabs(tmp) > fabs(pivotsize))
				{
					pivotsize = tmp;
					pivot = j;
				}
			}

			if (pivotsize == 0.0f) return s;

			if (pivot != i)
			{
				for (int j = 0; j < 5; ++j)
				{
					float tmp;
					tmp = t[i][j];
					t[i][j] = t[pivot][j];
					t[pivot][j] = tmp;
					tmp = s[i][j];
					s[i][j] = s[pivot][j];
					s[pivot][j] = tmp;
				}
			}

			for (int j = i + 1; j < 5; ++j)
			{
				float f = t[j][i] / t[i][i];
				for (int k = i + 1; k < 5; ++k)
				{
					t[j][k] -= f * t[i][k];
				}
				for (int k = 0; k < 5; ++k)
				{
					s[j][k] -= f * s[i][k];
				}
				t[j][i] = 0.0f;
			}
		}

		// Backward substitution
		for (int i = 0; i < 5; ++i)
		{
			float f = t[i][i];
			if (f == 0.0f) return mat5{ 1 };

			for (int j = 0; j < 5; ++j)
			{
				t[i][j] /= f;
				s[i][j] /= f;
			}

			for (int j = 0; j < i; ++j)
			{
				f = t[j][i];
				for (int k = 0; k < 5; ++k)
				{
					t[j][k] -= f * t[i][k];
					s[j][k] -= f * s[i][k];
				}
			}
		}

		return s;
	}
	mat5::operator glm::mat4() const
	{
		return glm::mat4
		{
			value[0].xyzw(),
			value[1].xyzw(),
			value[2].xyzw(),
			value[3].xyzw()
		};
	}

	mat5 createCamera4D(const glm::vec4& eye, const glm::vec4& left, const glm::vec4& up, const glm::vec4& forward, const glm::vec4& over)
	{
		mat5 cameraD{ -left, up, forward, over, {} };
		cameraD = mat5::transpose(cameraD);

		mat5 cameraP{ {}, {}, {}, {}, -eye };

		return cameraD * cameraP;
	}

	mat5::mat5(const nlohmann::json& j)
	{
		for (int col = 0; col < 5; ++col)
		{
			for (int row = 0; row < 5; ++row)
			{
				value[col][row] = j.at(col).at(row).get<float>();
			}
		}
	}

	nlohmann::json mat5::toJson() const
	{
		return {
			{ value[0][0], value[0][1], value[0][2], value[0][3], value[0][4] },
			{ value[1][0], value[1][1], value[1][2], value[1][3], value[1][4] },
			{ value[2][0], value[2][1], value[2][2], value[2][3], value[2][4] },
			{ value[3][0], value[3][1], value[3][2], value[3][3], value[3][4] },
			{ value[4][0], value[4][1], value[4][2], value[4][3], value[4][4] },
		};
	}

	mat6::mat6(float x)
	{
		value[0][0] =
			value[1][1] =
			value[2][2] =
			value[3][3] =
			value[4][4] =
			value[5][5] = x;
	}
	mat6::mat6(const mat5& m)
	{
		value[0] = m[0];
		value[1] = m[1];
		value[2] = m[2];
		value[3] = m[3];
		value[4] = m[4];
		value[5][5] = 1.0f;
	}

	mat6::mat6(const std::array<vec6, 6>& value)
		: value(value)
	{
	}
	mat6::mat6(const vec6& m0, const vec6& m1, const vec6& m2, const vec6& m3, const vec6& m4, const vec6& m5)
	{
		value[0] = m0;
		value[1] = m1;
		value[2] = m2;
		value[3] = m3;
		value[4] = m4;
		value[5] = m5;
	}

	mat6::mat6(const mat6& other)
	{
		this->value = other.value;
	}
	mat6::mat6(mat6&& other) noexcept
	{
		this->value = std::move(other.value);
	}
	mat6& mat6::operator=(const mat6& other)
	{
		this->value = other.value;

		return *this;
	}
	mat6& mat6::operator=(mat6&& other) noexcept
	{
		this->value = std::move(other.value);

		return *this;
	}

	mat6 mat6::operator*(const mat6& other) const
	{
		mat6 result;

		for (int col = 0; col < 6; ++col)
		{
			for (int row = 0; row < 6; ++row)
			{
				for (int k = 0; k < 6; ++k)
				{
					result.value[col][row] += value[k][row] * other.value[col][k];
				}
			}
		}

		return result;
	}
	mat6& mat6::operator*=(const mat6& other)
	{
		return *this = *this * other;
	}

	vec6 mat6::multiply(const vec6& v) const
	{
		return vec6
		{
			value[0][0] * v[0] + value[1][0] * v[1] + value[2][0] * v[2] + value[3][0] * v[3] + value[4][0] * v[4] + value[5][0] * v[5],
			value[0][1] * v[0] + value[1][1] * v[1] + value[2][1] * v[2] + value[3][1] * v[3] + value[4][1] * v[4] + value[5][1] * v[5],
			value[0][2] * v[0] + value[1][2] * v[1] + value[2][2] * v[2] + value[3][2] * v[3] + value[4][2] * v[4] + value[5][2] * v[5],
			value[0][3] * v[0] + value[1][3] * v[1] + value[2][3] * v[2] + value[3][3] * v[3] + value[4][3] * v[4] + value[5][3] * v[5],
			value[0][4] * v[0] + value[1][4] * v[1] + value[2][4] * v[2] + value[3][4] * v[3] + value[4][4] * v[4] + value[5][4] * v[5],
			value[0][5] * v[0] + value[1][5] * v[1] + value[2][5] * v[2] + value[3][5] * v[3] + value[4][5] * v[4] + value[5][5] * v[5]
		};
	}
	vec6 mat6::operator*(const vec6& v) const
	{
		return multiply(v);
	}

	void mat6::translate(const vec5& v)
	{
		value[5][0] += (value[0][0] * v.x) + (value[1][0] * v.y) + (value[2][0] * v.z) + (value[3][0] * v.w) + (value[4][0] * v.v);
		value[5][1] += (value[0][1] * v.x) + (value[1][1] * v.y) + (value[2][1] * v.z) + (value[3][1] * v.w) + (value[4][1] * v.v);
		value[5][2] += (value[0][2] * v.x) + (value[1][2] * v.y) + (value[2][2] * v.z) + (value[3][2] * v.w) + (value[4][2] * v.v);
		value[5][3] += (value[0][3] * v.x) + (value[1][3] * v.y) + (value[2][3] * v.z) + (value[3][3] * v.w) + (value[4][3] * v.v);
		value[5][4] += (value[0][4] * v.x) + (value[1][4] * v.y) + (value[2][4] * v.z) + (value[3][4] * v.w) + (value[4][4] * v.v);
	}
	void mat6::translate(float x, float y, float z, float w, float v)
	{
		translate({ x,y,z,w,v });
	}
	void mat6::translate(float a)
	{
		translate(a, a, a, a, a);
	}
	void mat6::scale(const vec5& s)
	{
		for (int row = 0; row < 6; ++row)
			for (int col = 0; col < 5; ++col)
				value[col][row] *= s[col];
	}
	void mat6::scale(float x, float y, float z, float w, float v)
	{
		scale({ x,y,z,w,v });
	}
	void mat6::scale(float s)
	{
		scale(s, s, s, s, s);
	}
#define rotMat(mat, x, y, cos, sin) \
{ \
	mat5 rot{ 1 }; \
	rot[x][x] = (cos); \
	rot[y][x] = -(sin); \
	rot[x][y] = (sin); \
	rot[y][y] = (cos); \
	mat *= rot; \
}

	void mat6::rotate(const bivec5& planes, float radians)
	{
		float cos = glm::cos(radians);
		float sin = glm::sin(radians);

		mat5 tRot{ 1 };
		rotMat(tRot, 0, 1, planes[0] * cos, planes[0] * sin);
		rotMat(tRot, 0, 2, planes[1] * cos, planes[1] * sin);
		rotMat(tRot, 0, 3, planes[2] * cos, planes[2] * sin);
		rotMat(tRot, 0, 4, planes[3] * cos, planes[3] * sin);
		rotMat(tRot, 1, 2, planes[4] * cos, planes[4] * sin);
		rotMat(tRot, 1, 3, planes[5] * cos, planes[5] * sin);
		rotMat(tRot, 1, 4, planes[6] * cos, planes[6] * sin);
		rotMat(tRot, 2, 3, planes[7] * cos, planes[7] * sin);
		rotMat(tRot, 2, 4, planes[8] * cos, planes[8] * sin);
		rotMat(tRot, 3, 4, planes[9] * cos, planes[9] * sin);

		*this *= tRot;
	}
	void mat6::rotateXY(float radians) { rotMat(*this, 0, 1, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateXZ(float radians) { rotMat(*this, 0, 2, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateXW(float radians) { rotMat(*this, 0, 3, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateXV(float radians) { rotMat(*this, 0, 4, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateYZ(float radians) { rotMat(*this, 1, 2, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateYW(float radians) { rotMat(*this, 1, 3, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateYV(float radians) { rotMat(*this, 1, 4, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateZW(float radians) { rotMat(*this, 2, 3, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateZV(float radians) { rotMat(*this, 2, 4, glm::cos(radians), glm::sin(radians)); }
	void mat6::rotateWV(float radians) { rotMat(*this, 3, 4, glm::cos(radians), glm::sin(radians)); }

	mat6 mat6::transpose(const mat6& m)
	{
		return mat6
		{
			{ m[0][0],m[1][0],m[2][0],m[3][0],m[4][0],m[5][0] },
			{ m[0][1],m[1][1],m[2][1],m[3][1],m[4][1],m[5][1] },
			{ m[0][2],m[1][2],m[2][2],m[3][2],m[4][2],m[5][2] },
			{ m[0][3],m[1][3],m[2][3],m[3][3],m[4][3],m[5][3] },
			{ m[0][4],m[1][4],m[2][4],m[3][4],m[4][4],m[5][4] },
			{ m[0][5],m[1][5],m[2][5],m[3][5],m[4][5],m[5][5] }
		};
	}
	mat6 mat6::inverse(const mat6& m)
	{
		mat6 s{ 1 };
		mat6 t{ m };

		// Forward elimination
		for (int i = 0; i < 6 - 1; ++i)
		{
			int pivot = i;
			float pivotsize = t[i][i];

			for (int j = i + 1; j < 6; ++j)
			{
				float tmp = t[j][i];
				if (fabs(tmp) > fabs(pivotsize))
				{
					pivotsize = tmp;
					pivot = j;
				}
			}

			if (pivotsize == 0.0f) return s;

			if (pivot != i)
			{
				for (int j = 0; j < 6; ++j)
				{
					float tmp;
					tmp = t[i][j];
					t[i][j] = t[pivot][j];
					t[pivot][j] = tmp;
					tmp = s[i][j];
					s[i][j] = s[pivot][j];
					s[pivot][j] = tmp;
				}
			}

			for (int j = i + 1; j < 6; ++j)
			{
				float f = t[j][i] / t[i][i];
				for (int k = i + 1; k < 6; ++k)
				{
					t[j][k] -= f * t[i][k];
				}
				for (int k = 0; k < 6; ++k)
				{
					s[j][k] -= f * s[i][k];
				}
				t[j][i] = 0.0f;
			}
		}

		// Backward substitution
		for (int i = 0; i < 6; ++i)
		{
			float f = t[i][i];
			if (f == 0.0f) return mat6{ 1 };

			for (int j = 0; j < 6; ++j)
			{
				t[i][j] /= f;
				s[i][j] /= f;
			}

			for (int j = 0; j < i; ++j)
			{
				f = t[j][i];
				for (int k = 0; k < 6; ++k)
				{
					t[j][k] -= f * t[i][k];
					s[j][k] -= f * s[i][k];
				}
			}
		}

		return s;
	}
	mat6::operator mat5() const
	{
		return mat5
		{
			value[0].xyzwv(),
			value[1].xyzwv(),
			value[2].xyzwv(),
			value[3].xyzwv(),
			value[4].xyzwv()
		};
	}

	// yonder - at some distance in the direction indicated; over there
	mat6 createCamera5D(const vec5& eye, const vec5& left, const vec5& up, const vec5& forward, const vec5& over, const vec5& yonder)
	{
		mat6 cameraD{ -left, up, forward, over, yonder, {} };
		cameraD = mat6::transpose(cameraD);

		mat6 cameraP{ {}, {}, {}, {}, {}, -eye };

		return cameraD * cameraP;
	}

	mat6::mat6(const nlohmann::json& j)
	{
		for (int col = 0; col < 6; ++col)
		{
			for (int row = 0; row < 6; ++row)
			{
				value[col][row] = j.at(col).at(row).get<float>();
			}
		}
	}

	nlohmann::json mat6::toJson() const
	{
		return {
			{ value[0][0], value[0][1], value[0][2], value[0][3], value[0][4], value[0][5] },
			{ value[1][0], value[1][1], value[1][2], value[1][3], value[1][4], value[1][5] },
			{ value[2][0], value[2][1], value[2][2], value[2][3], value[2][4], value[2][5] },
			{ value[3][0], value[3][1], value[3][2], value[3][3], value[3][4], value[3][5] },
			{ value[4][0], value[4][1], value[4][2], value[4][3], value[4][4], value[4][5] },
			{ value[5][0], value[5][1], value[5][2], value[5][3], value[5][4], value[5][5] },
		};
	}

	// untested
	//mat5 perspective(float fovy, float aspect, float zNear, float zFar)
	//{
	//	// https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/building-basic-perspective-projection-matrix.html
	//	mat5 res{ 0.0f };
	//
	//	float scale = 1.0f / glm::tan(glm::radians(fovy / 2.0f));
	//	res[0][0] = scale;
	//	res[1][1] = scale;
	//	res[2][2] = -zFar / (zFar - zNear);
	//	res[3][3] = scale;
	//	res[4][2] = -(zFar * zNear) / (zFar - zNear);
	//	res[2][4] = -1.0f;
	//
	//	return res;
	//}
	//
	//glm::vec4 adjustToMaxHorizSpeed(const glm::vec4& vel, const glm::vec4& deltaVel, float maxHorizSpeed)
	//{
	//	glm::vec4 totalVel = vel + deltaVel;
	//
	//	// the max horizontal speed can be thought of as the radius of a sphere centered at the origin.
	//	// a line segment between vel and totalVel will intersect the sphere's surface at the desired velocity.
	//	float radius = maxHorizSpeed;
	//	glm::vec4 intersection;
	//
	//	glm::vec3 a{ vel.x, vel.z, vel.w };
	//	glm::vec3 b{ totalVel.x, totalVel.z, totalVel.w };
	//
	//	// code adapted from https://stackoverflow.com/a/17499940/4885160
	//	double px = a.x;
	//	double py = a.y;
	//	double pz = a.z;
	//
	//	double vx = b.x - px;
	//	double vy = b.y - py;
	//	double vz = b.z - pz;
	//
	//	double A = vx * vx + vy * vy + vz * vz;
	//	double B = 2.0 * (px * vx + py * vy + pz * vz /*- vx * cx - vy * cy - vz * cz*/);
	//	double C = px * px /*- 2 * px * cx + cx * cx*/ + py * py /*- 2 * py * cy + cy * cy*/ +
	//		pz * pz /*- 2 * pz * cz + cz * cz*/ - radius * radius;
	//
	//	if (A == 0.0)
	//	{
	//		// prevent division by zero
	//		return glm::vec4{ 0, totalVel.y, 0, 0 };
	//	}
	//
	//	// discriminant
	//	double D = B * B - 4 * A * C;
	//
	//	if (D < 0)
	//	{
	//		// prevent NaN
	//		return glm::vec4{ 0, totalVel.y, 0, 0 };
	//	}
	//
	//	// there will always be two intersections for this line
	//	double t1 = (-B - glm::sqrt(D)) / (2.0 * A);
	//	glm::vec3 solution1 = {
	//		a.x * (1 - t1) + t1 * b.x,
	//		a.y * (1 - t1) + t1 * b.y,
	//		a.z * (1 - t1) + t1 * b.z
	//	};
	//
	//	if (D == 0)
	//	{
	//		return { solution1.x, totalVel.y, solution1.y, solution1.z };
	//	}
	//
	//	double t2 = (-B + glm::sqrt(D)) / (2.0 * A);
	//	glm::vec3 solution2 = {
	//		a.x * (1 - t2) + t2 * b.x,
	//		a.y * (1 - t2) + t2 * b.y,
	//		a.z * (1 - t2) + t2 * b.z
	//	};
	//
	//	// prefer a solution that's on the line segment itself
	//	if (glm::abs(t1 - 0.5) < glm::abs(t2 - 0.5))
	//	{
	//		return intersection = glm::vec4{ solution1.x, totalVel.y, solution1.y, solution1.z };
	//	}
	//
	//	return intersection = glm::vec4{ solution2.x, totalVel.y, solution2.y, solution2.z };
	//}
}
