#pragma once

#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <glm/glm.hpp>
#include "json.hpp"

namespace m5
{
	template<typename T = float>
	struct Tvec5
	{
		inline static constexpr size_t size = 5;

		union { T a{}; T x; T abcd; };
		union { T b{}; T y; T abce; };
		union { T c{}; T z; T abde; };
		union { T d{}; T w; T acde; };
		union { T e{}; T v; T bcde; };

		Tvec5() {}
		template<typename T2 = T>
		Tvec5(T2 e)
			: a((T)e), b((T)e), c((T)e), d((T)e), e((T)e) {}
		template<typename T2 = T>
		Tvec5(T2 a, T2 b, T2 c, T2 d, T2 e)
			: a((T)a), b((T)b), c((T)c), d((T)d), e((T)e) {}
		template<typename T2 = T>
		Tvec5(const glm::vec<4, T2>& abcd, T2 e = {})
			: a((T)abcd.x), b((T)abcd.y), c((T)abcd.z), d((T)abcd.w), e((T)e) {}

		template<typename T2 = T>
		Tvec5(const Tvec5<T2>& other)
			: a((T)other.a), b((T)other.b), c((T)other.c), d((T)other.d), e((T)other.e) {
		}
		template<typename T2 = T>
		Tvec5(Tvec5<T2>&& other) noexcept
			: a((T)other.a), b((T)other.b), c((T)other.c), d((T)other.d), e((T)other.e)
		{
			other.a = {};
			other.b = {};
			other.c = {};
			other.d = {};
			other.e = {};
		}
		template<typename T2 = T>
		Tvec5<T>& operator=(const Tvec5<T2>& other)
		{
			this->a = (T)other.a;
			this->b = (T)other.b;
			this->c = (T)other.c;
			this->d = (T)other.d;
			this->e = (T)other.e;

			return *this;
		}
		template<typename T2 = T>
		Tvec5<T>& operator=(Tvec5<T2>&& other) noexcept
		{
			if (this != &other)
			{
				this->a = (T)other.a;
				this->b = (T)other.b;
				this->c = (T)other.c;
				this->d = (T)other.d;
				this->e = (T)other.e;
				other.a = {};
				other.b = {};
				other.c = {};
				other.d = {};
				other.e = {};
			}

			return *this;
		}

		T& operator[](size_t i)
		{
			assert(i < size);
			return (&a)[i];
		}
		const T& operator[](size_t i) const
		{
			assert(i < size);
			return (&a)[i];
		}

		template<typename T2 = T>
		Tvec5<T> operator+(const Tvec5<T2>& other) const
		{
			return Tvec5<T>
			{
				this->a + (T)other.a,
				this->b + (T)other.b,
				this->c + (T)other.c,
				this->d + (T)other.d,
				this->e + (T)other.e,
			};
		}
		template<typename T2 = T>
		Tvec5<T> operator+(T2 v) const
		{
			return Tvec5<T>
			{
				this->a + (T)v,
				this->b + (T)v,
				this->c + (T)v,
				this->d + (T)v,
				this->e + (T)v,
			};
		}
		template<typename T2 = T>
		Tvec5<T> operator-(const Tvec5<T2>& other) const
		{
			return Tvec5<T>
			{
				this->a - (T)other.a,
				this->b - (T)other.b,
				this->c - (T)other.c,
				this->d - (T)other.d,
				this->e - (T)other.e,
			};
		}
		template<typename T2 = T>
		Tvec5<T> operator-(T2 v) const
		{
			return Tvec5<T>
			{
				this->a - (T)v,
				this->b - (T)v,
				this->c - (T)v,
				this->d - (T)v,
				this->e - (T)v,
			};
		}
		Tvec5<T> operator-() const
		{
			return Tvec5<T>
			{
				-this->a,
				-this->b,
				-this->c,
				-this->d,
				-this->e,
			};
		}
		template<typename T2 = T>
		Tvec5<T> operator*(const Tvec5<T2>& other) const
		{
			return Tvec5<T>
			{
				this->a * (T)other.a,
				this->b * (T)other.b,
				this->c * (T)other.c,
				this->d * (T)other.d,
				this->e * (T)other.e,
			};
		}
		template<typename T2 = T>
		Tvec5<T> operator*(T2 v) const
		{
			return Tvec5<T>
			{
				this->a * (T)v,
				this->b * (T)v,
				this->c * (T)v,
				this->d * (T)v,
				this->e * (T)v,
			};
		}
		template<typename T2 = T>
		Tvec5<T> operator/(const Tvec5<T2>& other) const
		{
			return Tvec5<T>
			{
				this->a / (T)other.a,
				this->b / (T)other.b,
				this->c / (T)other.c,
				this->d / (T)other.d,
				this->e / (T)other.e,
			};
		}
		template<typename T2 = T>
		Tvec5<T> operator/(T2 v) const
		{
			double invV = 1.0 / v;
			return Tvec5<T>
			{
				this->a * invV,
				this->b * invV,
				this->c * invV,
				this->d * invV,
				this->e * invV,
			};
		}

		template<typename T2 = T>
		Tvec5<T>& operator+=(const Tvec5<T2>& other)
		{
			return *this = *this + other;
		}
		template<typename T2 = T>
		Tvec5<T>& operator+=(T2 v)
		{
			return *this = *this + v;
		}
		template<typename T2 = T>
		Tvec5<T>& operator-=(const Tvec5<T2>& other)
		{
			return *this = *this - other;
		}
		template<typename T2 = T>
		Tvec5<T>& operator-=(T2 v)
		{
			return *this = *this - v;
		}
		template<typename T2 = T>
		Tvec5<T>& operator*=(const Tvec5<T>& other)
		{
			return *this = *this * other;
		}
		template<typename T2 = T>
		Tvec5<T>& operator*=(T2 v)
		{
			return *this = *this * v;
		}
		template<typename T2 = T>
		Tvec5<T>& operator/=(const Tvec5<T2>& other)
		{
			return *this = *this / other;
		}
		template<typename T2 = T>
		Tvec5<T>& operator/=(T2 v)
		{
			return *this = *this / v;
		}

		template<typename T2 = T>
		bool operator==(const Tvec5<T2>& other) const
		{
			return this->a == (T)other.a && this->b == (T)other.b && this->c == (T)other.c && this->d == (T)other.d && this->e == (T)other.e;
		}

		template<typename T2 = T>
		T dot(const Tvec5<T2>& other) const
		{
			return a * (T)other.a + b * (T)other.b + c * (T)other.c + d * (T)other.d + e * (T)other.e;
		}
		T length2() const
		{
			return dot(*this);
		}
		T length() const
		{
			return glm::sqrt(length2());
		}
		Tvec5<T>& normalize()
		{
			float len = length2();
			if (len <= glm::epsilon<T>() * glm::epsilon<T>())
			{
				return *this;
			}
			return *this /= glm::sqrt(len);
		}
		Tvec5<T> normalized() const
		{
			Tvec5<T> v = *this;
			v.normalize();
			return v;
		}

		glm::vec<4, T>& xyzw()
		{
			return *(glm::vec<4, T>*)&a;
		}
		const glm::vec<4, T>& xyzw() const
		{
			return *(const glm::vec<4, T>*)&a;
		}

		Tvec5(const nlohmann::json& j)
			: Tvec5(j.at(0).get<T>(), j.at(1).get<T>(), j.at(2).get<T>(), j.at(3).get<T>(), j.at(4).get<T>()) {}
		nlohmann::json toJson() const
		{
			return { a, b, c, d, e };
		}

		inline static Tvec5<T> up(){ return Tvec5<T>{ 1, 0, 0, 0, 0 }; }
		inline static Tvec5<T> forward(){ return Tvec5<T>{ 0, 1, 0, 0, 0 }; }
		inline static Tvec5<T> left(){ return Tvec5<T>{ 0, 0, 1, 0, 0 }; }
		inline static Tvec5<T> over(){ return Tvec5<T>{ 0, 0, 0, 1, 0 }; }
		inline static Tvec5<T> yonder() { return Tvec5<T>{ 0, 0, 0, 0, 1 }; }
	};

	using vec5 = Tvec5<float>;
	using i8vec5 = Tvec5<int8_t>;
	using i16vec5 = Tvec5<int16_t>;
	using i32vec5 = Tvec5<int32_t>;
	using ivec5 = i32vec5;
	using i64vec5 = Tvec5<int64_t>;
	using u8vec5 = Tvec5<uint8_t>;
	using u16vec5 = Tvec5<uint16_t>;
	using u32vec5 = Tvec5<uint32_t>;
	using uvec5 = u32vec5;
	using u64vec5 = Tvec5<uint64_t>;
	using bvec5 = Tvec5<bool>;
	using quadvec5 = vec5;

	inline float dot(const vec5& a, const vec5& b) { return a.dot(b); }
	inline vec5 normalize(const vec5& v) { return v.normalized(); }
	inline float length(const vec5& v) { return v.length(); }
	inline float length2(const vec5& v) { return v.length2(); }
	inline float distance(const vec5& a, const vec5& b) { return (b - a).length(); }
	inline float distance2(const vec5& a, const vec5& b) { return (b - a).length2(); }
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> min(const Tvec5<T>& a, const Tvec5<T2>& b)
	{
		return Tvec5<T>
		{
			glm::min(a.a, (T)b.a),
			glm::min(a.b, (T)b.b),
			glm::min(a.c, (T)b.c),
			glm::min(a.d, (T)b.d),
			glm::min(a.e, (T)b.e),
		};
	}
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> max(const Tvec5<T>& a, const Tvec5<T2>& b)
	{
		return Tvec5<T>
		{
			glm::max(a.a, (T)b.a),
			glm::max(a.b, (T)b.b),
			glm::max(a.c, (T)b.c),
			glm::max(a.d, (T)b.d),
			glm::max(a.e, (T)b.e),
		};
	}
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> mix(const Tvec5<T>& a, const Tvec5<T2>& b, float t)
	{
		return Tvec5<T>
		{
			glm::mix(a.a, (T)b.a, t),
			glm::mix(a.b, (T)b.b, t),
			glm::mix(a.c, (T)b.c, t),
			glm::mix(a.d, (T)b.d, t),
			glm::mix(a.e, (T)b.e, t),
		};
	}
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> mix(const Tvec5<T>& a, const Tvec5<T2>& b, const bvec5& t)
	{
		return Tvec5<T>
		{
			glm::mix(a.a, (T)b.a, t.a),
			glm::mix(a.b, (T)b.b, t.b),
			glm::mix(a.c, (T)b.c, t.c),
			glm::mix(a.d, (T)b.d, t.d),
			glm::mix(a.e, (T)b.e, t.e),
		};
	}
	template<typename T = float, typename T2 = T>
	inline bvec5 lessThan(const Tvec5<T>& a, const Tvec5<T2>& b)
	{
		return bvec5
		{
			a.a < (T)b.a,
			a.b < (T)b.b,
			a.c < (T)b.c,
			a.d < (T)b.d,
			a.e < (T)b.e,
		};
	}
	template<typename T = float, typename T2 = T>
	inline bvec5 lessThanEqual(const Tvec5<T>& a, const Tvec5<T2>& b)
	{
		return bvec5
		{
			a.a <= (T)b.a,
			a.b <= (T)b.b,
			a.c <= (T)b.c,
			a.d <= (T)b.d,
			a.e <= (T)b.e,
		};
	}
	template<typename T = float, typename T2 = T>
	inline bvec5 greaterThan(const Tvec5<T>& a, const Tvec5<T2>& b)
	{
		return bvec5
		{
			a.a > (T)b.a,
			a.b > (T)b.b,
			a.c > (T)b.c,
			a.d > (T)b.d,
			a.e > (T)b.e,
		};
	}
	template<typename T = float, typename T2 = T>
	inline bvec5 greaterThanEqual(const Tvec5<T>& a, const Tvec5<T2>& b)
	{
		return bvec5
		{
			a.a >= (T)b.a,
			a.b >= (T)b.b,
			a.c >= (T)b.c,
			a.d >= (T)b.d,
			a.e >= (T)b.e,
		};
	}
	template<typename T = float, typename T2 = T, typename T3 = T>
	inline Tvec5<T> clamp(const Tvec5<T>& v, const Tvec5<T2>& min, const Tvec5<T3>& max)
	{
		return m5::min(m5::max(v, min), max);
	}
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> abs(const Tvec5<T>& v)
	{
		return Tvec5<T>
		{
			glm::abs(v.a),
			glm::abs(v.b),
			glm::abs(v.c),
			glm::abs(v.d),
			glm::abs(v.e),
		};
	}
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> floor(const Tvec5<T>& v)
	{
		return Tvec5<T>
		{
			glm::floor(v.a),
			glm::floor(v.b),
			glm::floor(v.c),
			glm::floor(v.d),
			glm::floor(v.e),
		};
	}
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> fract(const Tvec5<T>& v)
	{
		return Tvec5<T>
		{
			glm::fract(v.a),
			glm::fract(v.b),
			glm::fract(v.c),
			glm::fract(v.d),
			glm::fract(v.e),
		};
	}
	template<typename T = float, typename T2 = T>
	inline Tvec5<T> sign(const Tvec5<T>& v)
	{
		return Tvec5<T>
		{
			glm::sign(v.a),
			glm::sign(v.b),
			glm::sign(v.c),
			glm::sign(v.d),
			glm::sign(v.e),
		};
	}
	inline bool any(const bvec5& v)
	{
		return v.a || v.b || v.c || v.d || v.e;
	}
}