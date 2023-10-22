// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module RenderCore.Utils.RenderUtils;

export import <list>;
export import <cstdint>;
export import <vector>;
export import <string_view>;
export import RenderCore.Types.Object;

export namespace RenderCore
{
    [[nodiscard]] std::list<std::uint32_t> LoadObject(std::string_view ObjectPath, std::string_view ModelTexture);
    void UnloadObject(std::list<std::uint32_t> const& ObjectID);

    [[nodiscard]] std::list<std::uint32_t> GetLoadedIDs();
    [[nodiscard]] std::vector<Object> const& GetLoadedObjects();
}// namespace RenderCore