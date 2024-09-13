// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#ifndef PCH_HPP
#define PCH_HPP

#pragma once

#include "RenderCoreModule.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <execution>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <regex>
#include <semaphore>
#include <source_location>
#include <span>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <Volk/volk.h>

#include <vma/vk_mem_alloc.h>

#ifdef GLFW_INCLUDE_VULKAN
    #undef GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <imgui.h>
#include <tiny_gltf.h>

#include <stringzilla/stringzilla.hpp>
namespace strzilla = ashvardanian::stringzilla;

#endif // PCH_HPP
