// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module RenderCore.Utils.RenderUtils;

import <cstdint>;
import <string_view>;

export namespace RenderCore
{
    [[nodiscard]] std::uint32_t LoadObject(std::string_view ModelPath, std::string_view ModelTexture);
    void UnloadObject(std::uint32_t ObjectID);
}// namespace RenderCore