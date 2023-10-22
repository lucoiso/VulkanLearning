// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module RenderCore.Utils.RenderUtils;

import RenderCore.EngineCore;

using namespace RenderCore;

std::list<std::uint32_t> RenderCore::LoadObject(std::string_view const ObjectPath, std::string_view const ModelTexture)
{
    return LoadScene(ObjectPath, ModelTexture);
}

void RenderCore::UnloadObject(std::list<std::uint32_t> const& ObjectIDs)
{
    UnloadScene(ObjectIDs);
}

std::list<std::uint32_t> RenderCore::GetLoadedIDs()
{
    std::list<std::uint32_t> IDs {};

    for (auto const& Object: GetObjects())
    {
        IDs.push_back(Object.GetID());
    }

    return IDs;
}

std::vector<class Object> const& RenderCore::GetLoadedObjects()
{
    return GetObjects();
}