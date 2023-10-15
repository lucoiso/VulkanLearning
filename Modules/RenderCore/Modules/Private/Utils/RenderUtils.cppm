// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module RenderCore.Utils.RenderUtils;

import RenderCore.EngineCore;

using namespace RenderCore;

std::uint32_t RenderCore::LoadObject(std::string_view const ModelPath, std::string_view const ModelTexture)
{
    return LoadModel(ModelPath, ModelTexture);
}

void RenderCore::UnloadObject(std::uint32_t ObjectID)
{
    UnLoadModel(ObjectID);
}