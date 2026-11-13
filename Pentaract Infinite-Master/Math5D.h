#pragma once
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vec5.h"
#include "vec6.h"
#include "json.hpp"

namespace m5
{
	struct bivec5
	{
		inline static constexpr size_t size = 10;

		union { float ab = 0.0f; float xy; };
		union { float ac = 0.0f; float xz; };
		union { float ad = 0.0f; float xw; };
		union { float ae = 0.0f; float xv; };
		union { float bc = 0.0f; float yz; };
		union { float bd = 0.0f; float yw; };
		union { float be = 0.0f; float yv; };
		union { float cd = 0.0f; float zw; };
		union { float ce = 0.0f; float zv; };
		union { float de = 0.0f; float wv; };

		bivec5() {}
		bivec5(float v)
			: ab{ v }, ac{ v }, ad{ v }, ae{ v }, bc{ v }, bd{ v }, be{ v }, cd{ v }, ce{ v }, de{ v } {}
		bivec5(float ab, float ac, float ad, float ae, float bc, float bd, float be, float cd, float ce, float de)
			: ab{ ab }, ac{ ac }, ad{ ad }, ae{ ae }, bc{ bc }, bd{ bd }, be{ be }, cd{ cd }, ce{ ce }, de{ de } {}

		bivec5(const bivec5& other);
		bivec5(bivec5&& other) noexcept;
		bivec5& operator=(const bivec5& other);
		bivec5& operator=(bivec5&& other) noexcept;

		bivec5(const nlohmann::json& j);
		nlohmann::json toJson() const;

		float& operator[](size_t i)
		{
			assert(i < size);
			return (&ab)[i];
		}

		const float& operator[](size_t i) const
		{
			assert(i < size);
			return (&ab)[i];
		}

		bivec5 operator+(const bivec5& other) const;
		bivec5 operator+(float v) const;
		bivec5 operator-(const bivec5& other) const;
		bivec5 operator-(float v) const;
		bivec5 operator-() const;
		bivec5 operator*(const bivec5& other) const;
		bivec5 operator*(float v) const;
		bivec5 operator/(const bivec5& other) const;
		bivec5 operator/(float v) const;

		bivec5& operator+=(const bivec5& other);
		bivec5& operator+=(float v);
		bivec5& operator-=(const bivec5& other);
		bivec5& operator-=(float v);
		bivec5& operator*=(const bivec5& other);
		bivec5& operator*=(float v);
		bivec5& operator/=(const bivec5& other);
		bivec5& operator/=(float v);

		bool operator==(const bivec5& other) const;

		bivec5 normalized() const;
		bivec5& normalize();

		// ig we're just going to do two component names for everything lol
		inline static bivec5 AB() { return bivec5{ 1,0,0,0,0,0,0,0,0,0 }; }
		inline static bivec5 AC() { return bivec5{ 0,1,0,0,0,0,0,0,0,0 }; }
		inline static bivec5 AD() { return bivec5{ 0,0,1,0,0,0,0,0,0,0 }; }
		inline static bivec5 AE() { return bivec5{ 0,0,0,1,0,0,0,0,0,0 }; }
		inline static bivec5 BC() { return bivec5{ 0,0,0,0,1,0,0,0,0,0 }; }
		inline static bivec5 BD() { return bivec5{ 0,0,0,0,0,1,0,0,0,0 }; }
		inline static bivec5 BE() { return bivec5{ 0,0,0,0,0,0,1,0,0,0 }; }
		inline static bivec5 CD() { return bivec5{ 0,0,0,0,0,0,0,1,0,0 }; }
		inline static bivec5 CE() { return bivec5{ 0,0,0,0,0,0,0,0,1,0 }; }
		inline static bivec5 DE() { return bivec5{ 0,0,0,0,0,0,0,0,0,1 }; }

		inline static bivec5 XY() { return bivec5{ 1,0,0,0,0,0,0,0,0,0 }; }
		inline static bivec5 XZ() { return bivec5{ 0,1,0,0,0,0,0,0,0,0 }; }
		inline static bivec5 XW() { return bivec5{ 0,0,1,0,0,0,0,0,0,0 }; }
		inline static bivec5 XV() { return bivec5{ 0,0,0,1,0,0,0,0,0,0 }; }
		inline static bivec5 YZ() { return bivec5{ 0,0,0,0,1,0,0,0,0,0 }; }
		inline static bivec5 YW() { return bivec5{ 0,0,0,0,0,1,0,0,0,0 }; }
		inline static bivec5 YV() { return bivec5{ 0,0,0,0,0,0,1,0,0,0 }; }
		inline static bivec5 ZW() { return bivec5{ 0,0,0,0,0,0,0,1,0,0 }; }
		inline static bivec5 ZV() { return bivec5{ 0,0,0,0,0,0,0,0,1,0 }; }
		inline static bivec5 WV() { return bivec5{ 0,0,0,0,0,0,0,0,0,1 }; }
	};

	float dot(const bivec5& a, const bivec5& b);
	bivec5 wedge(const vec5& a, const vec5& b);
	bivec5 normalize(const bivec5& v);
	float length(const bivec5& v);
	float length2(const bivec5& v);
	glm::vec4 cross(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c);
	vec5 cross(const vec5& a, const vec5& b, const vec5& c, const vec5& d);

	struct mat5
	{
		std::array<vec5, 5> value
		{
			vec5{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec5{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec5{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec5{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec5{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
		};

		mat5(float x = 0.0f);
		mat5(const glm::mat4& m);
		mat5(const std::array<vec5, 5>& value);
		mat5(const vec5& m0, const vec5& m1, const vec5& m2, const vec5& m3, const vec5& m4);

		mat5(const mat5& other);
		mat5(mat5&& other) noexcept;
		mat5& operator=(const mat5& other);
		mat5& operator=(mat5&& other) noexcept;

		mat5(const nlohmann::json& j);
		nlohmann::json toJson() const;

		mat5 operator*(const mat5& other) const;
		mat5& operator*=(const mat5& other);

		vec5 multiply(const vec5& v) const;
		glm::vec4 multiply(const glm::vec4& v, float finalComp = 1.0f) const;
		vec5 operator*(const vec5& v) const;
		glm::vec4 operator*(const glm::vec4& v) const;

		vec5& operator[](size_t i)
		{
			return value[i];
		}
		const vec5& operator[](size_t i) const
		{
			return value[i];
		}

		void translate(const glm::vec4& v);
		void translate(float x, float y, float z, float w);
		void translate(float a);
		void scale(const glm::vec4& s);
		void scale(float x, float y, float z, float w);
		void scale(float s);

		static mat5 transpose(const mat5& m);
		static mat5 inverse(const mat5& m);

		operator glm::mat4() const;
	};

	mat5 createCamera4D(const glm::vec4& eye, const glm::vec4& left, const glm::vec4& up, const glm::vec4& forward, const glm::vec4& over);

	struct rotor5
	{
		// scalar
		float s = 1;
		// bivector
		bivec5 b = 0;
		// quadrivector
		quadvec5 q = 0;

		// default constructor
		rotor5() {}
		rotor5(float s, const bivec5& b, const quadvec5& q)
			: s{ s }, b{ b }, q{ q } {}
		// rotate one vector to another
		rotor5(const vec5& from, const vec5& to);
		// rotate by angle on a given plane ("plane" must be normailzed)
		rotor5(const bivec5& plane, float radians);
		// interpolate between two rotors
		rotor5(const rotor5& from, const rotor5& to, float a);

		rotor5(const rotor5& other);
		rotor5(rotor5&& other) noexcept;
		rotor5& operator=(const rotor5& other);
		rotor5& operator=(rotor5&& other) noexcept;

		rotor5(const nlohmann::json& j);
		nlohmann::json toJson() const;

		// multiply
		rotor5 operator*(const rotor5& r) const;
		rotor5 operator*=(const rotor5& r);
		rotor5 rotate(const rotor5& r) const;

		// reverse
		rotor5 reverse() const;
		rotor5 operator~() const { return reverse(); }
		rotor5 operator-() const { return reverse(); }

		bool operator==(const rotor5& other) const;

		// rotate a vector by the rotor
		vec5 rotate(const vec5& v) const;

		rotor5 normalized() const;
		rotor5& normalize();

		operator mat5() const;
	};

	rotor5 normalize(const rotor5& v);
	float length(const rotor5& v);
	float length2(const rotor5& v);

	struct mat6
	{
		std::array<vec6, 6> value
		{
			vec6{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec6{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec6{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec6{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec6{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
			vec6{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
		};

		mat6(float x = 0.0f);
		mat6(const mat5& m);
		mat6(const std::array<vec6, 6>& value);
		mat6(const vec6& m0, const vec6& m1, const vec6& m2, const vec6& m3, const vec6& m4, const vec6& m5);

		mat6(const mat6& other);
		mat6(mat6&& other) noexcept;
		mat6& operator=(const mat6& other);
		mat6& operator=(mat6&& other) noexcept;

		mat6(const nlohmann::json& j);
		nlohmann::json toJson() const;

		mat6 operator*(const mat6& other) const;
		mat6& operator*=(const mat6& other);

		vec6 multiply(const vec6& v) const;
		vec6 operator*(const vec6& v) const;

		vec6& operator[](size_t i)
		{
			return value[i];
		}
		const vec6& operator[](size_t i) const
		{
			return value[i];
		}

		void translate(const vec5& v);
		void translate(float x, float y, float z, float w, float v);
		void translate(float a);
		void scale(const vec5& s);
		void scale(float x, float y, float z, float w, float v);
		void scale(float s);
		void rotate(const bivec5& planes, float radians);
		void rotateXY(float radians);
		void rotateXZ(float radians);
		void rotateXW(float radians);
		void rotateXV(float radians);
		void rotateYZ(float radians);
		void rotateYW(float radians);
		void rotateYV(float radians);
		void rotateZW(float radians);
		void rotateZV(float radians);
		void rotateWV(float radians);

		static mat6 transpose(const mat6& m);
		static mat6 inverse(const mat6& m);

		operator mat5() const;
	};

	// yonder - at some distance in the direction indicated; over there
	m5::mat6 createCamera5D(const vec5& eye, const vec5& left, const vec5& up, const vec5& forward, const vec5& over, const vec5& yonder);

	// json conversion functions
	template <typename T = float>
	glm::vec<2, T> vec2FromJson(const nlohmann::json& j)
	{
		return { j.at(0).get<T>(), j.at(1).get<T>() };
	}
	template <typename T = float>
	nlohmann::json vec2ToJson(const glm::vec<2, T>& vec)
	{
		return { vec.x, vec.y };
	}

	template <typename T = float>
	glm::vec<3, T> vec3FromJson(const nlohmann::json& j)
	{
		return { j.at(0).get<T>(), j.at(1).get<T>(), j.at(2).get<T>() };
	}
	template <typename T = float>
	nlohmann::json vec3ToJson(const glm::vec<3, T>& vec)
	{
		return { vec.x, vec.y, vec.z };
	}

	template <typename T = float>
	glm::vec<4, T> vec4FromJson(const nlohmann::json& j)
	{
		return { j.at(0).get<T>(), j.at(1).get<T>(), j.at(2).get<T>(), j.at(3).get<T>() };
	}
	template <typename T = float>
	nlohmann::json vec4ToJson(const glm::vec<4, T>& vec)
	{
		return { vec.x, vec.y, vec.z, vec.w };
	}

	template <typename T = float>
	glm::qua<T> quatFromJson(const nlohmann::json& j)
	{
		return { j.at(0).get<T>(), j.at(1).get<T>(), j.at(2).get<T>(), j.at(3).get<T>() };
	}
	template <typename T = float>
	nlohmann::json quatToJson(const glm::qua<T>& quat)
	{
		return { quat.w, quat.x, quat.y, quat.z };
	}

	template<glm::length_t L, typename T>
	inline glm::vec<L, T> safeNormalize(const glm::vec<L, T>& v)
	{
		T len = glm::dot(v, v);
		if (len <= glm::epsilon<T>() * glm::epsilon<T>())
		{
			return glm::vec<L, T>{};
		}
		return v / glm::sqrt(len);
	}
}