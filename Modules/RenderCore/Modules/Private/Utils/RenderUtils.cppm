// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module RenderCore.Utils.RenderUtils;

import RenderCore.EngineCore;

using namespace RenderCore;

[[nodiscard]] std::uint32_t LoadObject(std::string_view ModelPath, std::string_view ModelTexture)
{
    return LoadScene(ModelPath, ModelTexture);
}

void UnloadObject(std::uint32_t ObjectID)
{
    UnloadScene(ObjectID);
}