// vec.hpp

#pragma once

// basic vectors used for everything including binoutput

struct Vec2f {
	float x = 0.0f;
	float z = 0.0f;
	inline Vec2f() = default;
	inline Vec2f(float x, float z) : x(x), z(z) {}
};
struct Vec3f {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	inline Vec3f() = default;
	inline Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
};
