#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <memory>
#include <algorithm>

namespace utils
{
	inline std::vector<std::string> splitStr(std::string_view str, char delim)
	{
		std::vector<std::string> tokens;
		size_t pos = 0;
		size_t len = str.length();
		tokens.reserve(len / 2);  // allocate memory for expected number of tokens

		while (pos < len)
		{
			size_t end = str.find_first_of(delim, pos);
			if (end == std::string::npos)
			{
				tokens.emplace_back(str.substr(pos));
				break;
			}
			tokens.emplace_back(str.substr(pos, end - pos));
			pos = end + 1;
		}

		return tokens;
	}
	inline void trimStart(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](uint8_t c) {
			return !std::isspace(c);
			}));
	}
	inline void trimEnd(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](uint8_t c) {
			return !std::isspace(c);
			}).base(), s.end());
	}
	inline void trim(std::string& s)
	{
		trimStart(s);
		trimEnd(s);
	}
	inline void toLower(std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(),
			[](uint8_t c) { return std::tolower(c); });
	}
	inline void toUpper(std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(),
			[](uint8_t c) { return std::toupper(c); });
	}

	// window and framebuffer sizes may be different on mac and wayland because of hidpi.
	// this maps the cursor pos from window coordinates to framebuffer coordinates.
	inline void getCursorPos(GLFWwindow* window, double* x, double* y)
	{
		double xp, yp;
		glfwGetCursorPos(window, &xp, &yp);
		int wW, wH;
		glfwGetWindowSize(window, &wW, &wH);
		int fW, fH;
		glfwGetFramebufferSize(window, &fW, &fH);

		if (x) *x = xp * fW / wW;
		if (y) *y = yp * fH / wH;
	}
	inline void setCursorPos(GLFWwindow* window, double x, double y)
	{
		int wW, wH;
		glfwGetWindowSize(window, &wW, &wH);
		int fW, fH;
		glfwGetFramebufferSize(window, &fW, &fH);

		glfwSetCursorPos(window, x * wW / fW, y * wH / fH);
	}

	// wayland doesn't hide the cursor when you disable it.
	inline void hideCursor(GLFWwindow* window)
	{
		static uint8_t pixels[4] = { 0, 0, 0, 0 };
		static GLFWimage image{ 1, 1, pixels };
		static GLFWcursor* cursor = glfwCreateCursor(&image, 0, 0);

		glfwSetCursor(window, cursor);
	}
	inline void showCursor(GLFWwindow* window)
	{
		glfwSetCursor(window, nullptr);
	}
}
