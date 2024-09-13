// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.Model;

using namespace RenderCore;

void RenderCore::InsertIndiceInContainer(std::vector<std::uint32_t> &Indices, tinygltf::Accessor const &IndexAccessor, auto const *Data)
{
    for (std::uint32_t Iterator = 0U; Iterator < IndexAccessor.count; ++Iterator)
    {
        Indices.push_back(static_cast<std::uint32_t>(Data[Iterator]));
    }
}

float const *RenderCore::GetPrimitiveData(strzilla::string_view const &ID,
                                          tinygltf::Model const &      Model,
                                          tinygltf::Primitive const &  Primitive,
                                          std::uint32_t *              NumComponents = nullptr)
{
    if (Primitive.attributes.contains(std::data(ID)))
    {
        tinygltf::Accessor const &  Accessor   = Model.accessors.at(Primitive.attributes.at(std::data(ID)));
        tinygltf::BufferView const &BufferView = Model.bufferViews.at(Accessor.bufferView);
        tinygltf::Buffer const &    Buffer     = Model.buffers.at(BufferView.buffer);

        if (NumComponents)
        {
            switch (Accessor.type)
            {
                case TINYGLTF_TYPE_SCALAR:
                    *NumComponents = 1U;
                    break;
                case TINYGLTF_TYPE_VEC2:
                    *NumComponents = 2U;
                    break;
                case TINYGLTF_TYPE_VEC3:
                    *NumComponents = 3U;
                    break;
                case TINYGLTF_TYPE_VEC4:
                    *NumComponents = 4U;
                    break;
                default:
                    break;
            }
        }

        return reinterpret_cast<float const *>(std::data(Buffer.data) + BufferView.byteOffset + Accessor.byteOffset);
    }

    return nullptr;
}

void RenderCore::SetVertexAttributes(std::shared_ptr<Mesh> const &Mesh, tinygltf::Model const &Model, tinygltf::Primitive const &Primitive)
{
    std::vector<Vertex> Vertices;
    {
        tinygltf::Accessor const &PositionAccessor = Model.accessors.at(Primitive.attributes.at("POSITION"));
        Vertices.resize(PositionAccessor.count);
    }

    float const  * PositionData = GetPrimitiveData("POSITION", Model, Primitive);
    float const  * NormalData   = GetPrimitiveData("NORMAL", Model, Primitive);
    float const  * TexCoordData = GetPrimitiveData("TEXCOORD_0", Model, Primitive);
    std::uint32_t NumColorComponents {};
    float const  * ColorData   = GetPrimitiveData("COLOR_0", Model, Primitive, &NumColorComponents);
    float const  * JointData   = GetPrimitiveData("JOINTS_0", Model, Primitive);
    float const  * WeightData  = GetPrimitiveData("WEIGHTS_0", Model, Primitive);
    float const  * TangentData = GetPrimitiveData("TANGENT", Model, Primitive);

    bool const HasSkin = JointData && WeightData;

    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(Vertices)); ++Iterator)
    {
        if (PositionData)
        {
            Vertices.at(Iterator).Position = glm::make_vec3(&PositionData[Iterator * 3]);
        }

        if (NormalData)
        {
            Vertices.at(Iterator).Normal = glm::make_vec3(&NormalData[Iterator * 3]);
        }

        if (TexCoordData)
        {
            Vertices.at(Iterator).TextureCoordinate = glm::make_vec2(&TexCoordData[Iterator * 2]);
        }

        if (ColorData)
        {
            switch (NumColorComponents)
            {
                case 3U:
                    Vertices.at(Iterator).Color = glm::vec4(glm::make_vec3(&ColorData[Iterator * 3]), 1.F);
                    break;
                case 4U:
                    Vertices.at(Iterator).Color = glm::make_vec4(&ColorData[Iterator * 4]);
                    break;
                default:
                    break;
            }
        }
        else
        {
            Vertices.at(Iterator).Color = glm::vec4(1.F);
        }

        if (HasSkin)
        {
            Vertices.at(Iterator).Joint  = glm::make_vec4(&JointData[Iterator * 4]);
            Vertices.at(Iterator).Weight = glm::make_vec4(&WeightData[Iterator * 4]);
        }

        if (TangentData)
        {
            Vertices.at(Iterator).Tangent = glm::make_vec4(&TangentData[Iterator * 4]);
        }
    }

    Mesh->SetVertices(Vertices);
}

void RenderCore::AllocatePrimitiveIndices(std::shared_ptr<Mesh> const &Mesh, tinygltf::Model const &Model, tinygltf::Primitive const &Primitive)
{
    std::vector<std::uint32_t> Indices;

    if (Primitive.indices >= 0)
    {
        tinygltf::Accessor const &  IndexAccessor   = Model.accessors.at(Primitive.indices);
        tinygltf::BufferView const &IndexBufferView = Model.bufferViews.at(IndexAccessor.bufferView);
        tinygltf::Buffer const &    IndexBuffer     = Model.buffers.at(IndexBufferView.buffer);
        unsigned char const *const  IndicesData     = std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset;

        Indices.reserve(IndexAccessor.count);

        switch (IndexAccessor.componentType)
        {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
                InsertIndiceInContainer(Indices, IndexAccessor, reinterpret_cast<std::uint32_t const *>(IndicesData));
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
                InsertIndiceInContainer(Indices, IndexAccessor, reinterpret_cast<uint16_t const *>(IndicesData));
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
                InsertIndiceInContainer(Indices, IndexAccessor, IndicesData);
                break;
            }
            default:
                break;
        }
    }

    Mesh->SetIndices(Indices);
}

void RenderCore::SetPrimitiveTransform(std::shared_ptr<Mesh> const &Mesh, tinygltf::Node const &Node)
{
    Transform Transform {};

    if (!std::empty(Node.translation))
    {
        Transform.SetPosition(glm::make_vec3(std::data(Node.translation)));
    }

    if (!std::empty(Node.scale))
    {
        Transform.SetScale(glm::make_vec3(std::data(Node.scale)));
    }

    if (!std::empty(Node.rotation))
    {
        Transform.SetRotation(degrees(eulerAngles(glm::make_quat(std::data(Node.rotation)))));
    }

    if (!Node.matrix.empty())
    {
        Transform.SetMatrix(glm::make_mat4(std::data(Node.matrix)));
    }

    Mesh->SetTransform(Transform);
    Mesh->SetupBounds();
}
