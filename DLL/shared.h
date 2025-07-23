#pragma once

#include "framework.h"
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <kiero.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdint>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <inicpp.h>
#include <ordered_map.h>
#include <OIS.h>
#include <utility>
#include <detours.h>
#include <Psapi.h>
#include <ranges>

struct FastIO {
	FastIO() {
		std::ios_base::sync_with_stdio(false);
		std::cin.tie(nullptr);
	}
} fast_io_dummy;

inline std::string currentTime() {
	auto time = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(time);
	std::tm tm;
	localtime_s(&tm, &t);
	std::ostringstream oss;
	oss << '[' << std::setw(2) << std::setfill('0') << tm.tm_hour
		<< ':' << std::setw(2) << std::setfill('0') << tm.tm_min
		<< ':' << std::setw(2) << std::setfill('0') << tm.tm_sec
		<< '.' << std::setw(3) << std::setfill('0') << (duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000).count() << ']';
	return oss.str();
}

template<typename T>
inline void LogMessage(const T& msg) {
	std::cout << currentTime() << ' ' << msg << '\n';
}

template<typename T, typename... Args>
inline void LogMessage(const T& first, const Args&... rest) {
	std::cout << currentTime() << ' ' << first;
	// Using a fold-like trick to print rest with spaces
	using expander = int[];
	(void)expander {
		0, (std::cout << ' ' << rest, 0)...
	};
	std::cout << '\n';
}

