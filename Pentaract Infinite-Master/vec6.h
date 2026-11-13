#pragma once

#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <glm/glm.hpp>
#include "vec5.h"

namespace m5
{
	template<typename T = float>
	struct Tvec6
	{
		inline static constexpr size_t size = 6;

		union { T x{}; T a; };
		union { T y{}; T b; };
		union { T z{}; T c; };
		union { T w{}; T d; };
		union { T v{}; T e; };
		union { T u{}; T f; };

		Tvec6() {}
		template<typename T2 = T>
		Tvec6(T2 v)
			: x((T)v), y((T)v), z((T)v), w((T)v), v((T)v), u((T)v) {}
		template<typename T2 = T>
		Tvec6(T2 x, T2 y, T2 z, T2 w, T2 v, T2 u)
			: x((T)x), y((T)y), z((T)z), w((T)w), v((T)v), u((T)u) {}
		template<typename T2 = T>
		Tvec6(const glm::vec<4, T2>& xyzw, T2 v = {}, T2 u = {})
			: x((T)xyzw.x), y((T)xyzw.y), z((T)xyzw.z), w((T)xyzw.w), v((T)v), u((T)u) {}
		template<typename T2 = T>
		Tvec6(const Tvec5<T2>& xyzwv, T2 u = {})
			: x((T)xyzwv.x), y((T)xyzwv.y), z((T)xyzwv.z), w((T)xyzwv.w), v((T)xyzwv.v), u((T)u) {}

		template<typename T2 = T>
		Tvec6(const Tvec6<T2>& other)
			: x((T)other.x), y((T)other.y), z((T)other.z), w((T)other.w), v((T)other.v), u((T)other.u) {}
		template<typename T2 = T>
		Tvec6(Tvec6<T2>&& other) noexcept
			: x((T)other.x), y((T)other.y), z((T)other.z), w((T)other.w), v((T)other.v), u((T)other.u)
		{
			other.x = {};
			other.y = {};
			other.z = {};
			other.w = {};
			other.v = {};
			other.u = {};
		}
		template<typename T2 = T>
		Tvec6<T>& operator=(const Tvec6<T2>& other)
		{
			this->x = (T)other.x;
			this->y = (T)other.y;
			this->z = (T)other.z;
			this->w = (T)other.w;
			this->v = (T)other.v;
			this->u = (T)other.u;

			return *this;
		}
		template<typename T2 = T>
		Tvec6<T>& operator=(Tvec6<T2>&& other) noexcept
		{
			if (this != &other)
			{
				this->x = (T)other.x;
				this->y = (T)other.y;
				this->z = (T)other.z;
				this->w = (T)other.w;
				this->v = (T)other.v;
				this->u = (T)other.u;
				other.x = {};
				other.y = {};
				other.z = {};
				other.w = {};
				other.v = {};
				other.u = {};
			}

			return *this;
		}

		T& operator[](size_t i)
		{
			if (i >= size) throw std::out_of_range("m5::vec6::operator[]: index out of range.");

			return (&x)[i];
		}
		const T& operator[](size_t i) const
		{
			if (i >= size) throw std::out_of_range("m5::vec6::operator[]: index out of range.");

			return (&x)[i];
		}

		template<typename T2 = T>
		Tvec6<T> operator+(const Tvec6<T2>& other) const
		{
			return Tvec6<T>
			{
				this->x + (T)other.x,
				this->y + (T)other.y,
				this->z + (T)other.z,
				this->w + (T)other.w,
				this->v + (T)other.v,
				this->v + (T)other.u
			};
		}
		template<typename T2 = T>
		Tvec6<T> operator+(T2 v) const
		{
			return Tvec6<T>
			{
				this->x + (T)v,
				this->y + (T)v,
				this->z + (T)v,
				this->w + (T)v,
				this->v + (T)v,
				this->u + (T)v
			};
		}
		template<typename T2 = T>
		Tvec6<T> operator-(const Tvec6<T2>& other) const
		{
			return Tvec6<T>
			{
				this->x - (T)other.x,
				this->y - (T)other.y,
				this->z - (T)other.z,
				this->w - (T)other.w,
				this->v - (T)other.v,
				this->u - (T)other.u
			};
		}
		template<typename T2 = T>
		Tvec6<T> operator-(T2 v) const
		{
			return Tvec6<T>
			{
				this->x - (T)v,
				this->y - (T)v,
				this->z - (T)v,
				this->w - (T)v,
				this->v - (T)v,
				this->u - (T)v
			};
		}
		Tvec6<T> operator-() const
		{
			return Tvec6<T>
			{
				-this->x,
				-this->y,
				-this->z,
				-this->w,
				-this->v,
				-this->u
			};
		}
		template<typename T2 = T>
		Tvec6<T> operator*(const Tvec6<T2>& other) const
		{
			return Tvec6<T>
			{
				this->x * (T)other.x,
				this->y * (T)other.y,
				this->z * (T)other.z,
				this->w * (T)other.w,
				this->v * (T)other.v,
				this->u * (T)other.u
			};
		}
		template<typename T2 = T>
		Tvec6<T> operator*(T2 v) const
		{
			return Tvec6<T>
			{
				this->x * (T)v,
				this->y * (T)v,
				this->z * (T)v,
				this->w * (T)v,
				this->v * (T)v,
				this->u * (T)v
			};
		}
		template<typename T2 = T>
		Tvec6<T> operator/(const Tvec6<T2>& other) const
		{
			return Tvec6<T>
			{
				this->x / (T)other.x,
				this->y / (T)other.y,
				this->z / (T)other.z,
				this->w / (T)other.w,
				this->v / (T)other.v,
				this->u / (T)other.u
			};
		}
		template<typename T2 = T>
		Tvec6<T> operator/(T2 v) const
		{
			double invV = 1.0 / v;
			return Tvec6<T>
			{
				this->x * invV,
				this->y * invV,
				this->z * invV,
				this->w * invV,
				this->v * invV,
				this->u * invV
			};
		}

		template<typename T2 = T>
		Tvec6<T>& operator+=(const Tvec6<T2>& other)
		{
			return *this = *this + other;
		}
		template<typename T2 = T>
		Tvec6<T>& operator+=(T2 v)
		{
			return *this = *this + v;
		}
		template<typename T2 = T>
		Tvec6<T>& operator-=(const Tvec6<T2>& other)
		{
			return *this = *this - other;
		}
		template<typename T2 = T>
		Tvec6<T>& operator-=(T2 v)
		{
			return *this = *this - v;
		}
		template<typename T2 = T>
		Tvec6<T>& operator*=(const Tvec6<T2>& other)
		{
			return *this = *this * other;
		}
		template<typename T2 = T>
		Tvec6<T>& operator*=(T2 v)
		{
			return *this = *this * v;
		}
		template<typename T2 = T>
		Tvec6<T>& operator/=(const Tvec6<T2>& other)
		{
			return *this = *this / other;
		}
		template<typename T2 = T>
		Tvec6<T>& operator/=(T2 v)
		{
			return *this = *this / v;
		}

		template<typename T2 = T>
		bool operator==(const Tvec6<T2>& other) const
		{
			return this->x == (T)other.x && this->y == (T)other.y && this->z == (T)other.z && this->w == (T)other.w && this->v == (T)other.v && this->u == (T)other.u;
		}

		template<typename T2 = T>
		T dot(const Tvec6<T2>& other) const
		{
			return x * (T)other.x + y * (T)other.y + z * (T)other.z + w * (T)other.w + v * (T)other.v + u * (T)other.u;
		}
		T length2() const
		{
			return dot(*this);
		}
		T length() const
		{
			return glm::sqrt(length2());
		}
		Tvec6<T>& normalize()
		{
			float len = length2();
			if (len <= glm::epsilon<T>() * glm::epsilon<T>())
			{
				return *this;
			}
			return *this /= glm::sqrt(len);
		}
		Tvec6<T> normalized() const
		{
			Tvec6<T> v = *this;
			v.normalize();
			return v;
		}

		glm::vec<4, T>& xyzw()
		{
			return *(glm::vec<4, T>*)&x;
		}
		const glm::vec<4, T>& xyzw() const
		{
			return *(const glm::vec<4, T>*)&x;
		}

		Tvec5<T>& xyzwv()
		{
			return *(Tvec5<T>*)&x;
		}
		const Tvec5<T>& xyzwv() const
		{
			return *(const Tvec5<T>*)&x;
		}

		Tvec6(const nlohmann::json& j)
			: Tvec6(j.at(0).get<T>(), j.at(1).get<T>(), j.at(2).get<T>(), j.at(3).get<T>(), j.at(4).get<T>(), j.at(5).get<T>()) {}
		nlohmann::json toJson() const
		{
			return { a, b, c, d, e, f };
		}
	};

	using vec6 = Tvec6<float>;
	using i8vec6 = Tvec6<int8_t>;
	using i16vec6 = Tvec6<int16_t>;
	using i32vec6 = Tvec6<int32_t>;
	using ivec6 = i32vec6;
	using i64vec6 = Tvec6<int64_t>;
	using u8vec6 = Tvec6<uint8_t>;
	using u16vec6 = Tvec6<uint16_t>;
	using u32vec6 = Tvec6<uint32_t>;
	using uvec6 = u32vec6;
	using u64vec6 = Tvec6<uint64_t>;

	inline float dot(const vec6& a, const vec6& b) { return a.dot(b); }
	inline vec6 normalize(const vec6& v) { return v.normalized(); }
	inline float length(const vec6& v) { return v.length(); }
	inline float length2(const vec6& v) { return v.length2(); }
	inline float distance(const vec6& a, const vec6& b) { return (b - a).length(); }
	inline float distance2(const vec6& a, const vec6& b) { return (b - a).length2(); }
	template<typename T = float, typename T2 = T>
	inline Tvec6<T> min(const Tvec6<T>& a, const Tvec6<T2>& b)
	{
		return Tvec6<T>
		{
			glm::min(a.a, (T)b.a),
			glm::min(a.b, (T)b.b),
			glm::min(a.c, (T)b.c),
			glm::min(a.d, (T)b.d),
			glm::min(a.e, (T)b.e),
			glm::min(a.f, (T)b.f),
		};
	}
	template<typename T = float, typename T2 = T>
	inline Tvec6<T> max(const Tvec6<T>& a, const Tvec6<T2>& b)
	{
		return Tvec6<T>
		{
			glm::max(a.a, (T)b.a),
			glm::max(a.b, (T)b.b),
			glm::max(a.c, (T)b.c),
			glm::max(a.d, (T)b.d),
			glm::max(a.e, (T)b.e),
			glm::max(a.f, (T)b.f),
		};
	}
	template<typename T = float, typename T2 = T, typename T3 = T>
	inline Tvec6<T> clamp(const Tvec6<T>& v, const Tvec6<T2>& min, const Tvec6<T3>& max)
	{
		return m5::min(m5::max(v, min), max);
	}
}