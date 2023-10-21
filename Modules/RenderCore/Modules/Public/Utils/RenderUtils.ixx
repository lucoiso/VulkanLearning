// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module RenderCore.Utils.RenderUtils;

import <cstdint>;
import <vector>;
import <list>;
import <string_view>;

export namespace RenderCore
{
    [[nodiscard]] std::list<std::uint32_t> LoadObject(std::string_view ObjectPath, std::string_view ModelTexture);
    void UnloadObject(std::list<std::uint32_t> const& ObjectID);

    [[nodiscard]] std::list<std::uint32_t> GetLoadedIDs();
    [[nodiscard]] std::vector<class Object> GetLoadedObjects();
}// namespace RenderCore