#include "Mesh.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/postprocess.h>     // Post processing flags
#include <assimp/scene.h>           // Output data structure

namespace Engine
{
    MeshLoader::result_type MeshLoader::operator()(std::string_view filePath) const
    {
        Assimp::Importer importer;

        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;


        const aiScene* scene = importer.ReadFile(filePath.data(), aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast);
        if (!scene)
        {
            //spdlog::debug("{}", importer.GetErrorString());
            return nullptr;
        }

        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        {
            for (unsigned int j = 0; j < scene->mMeshes[i]->mNumVertices; ++j)
            {
                const aiVector3D vertex = scene->mMeshes[i]->mVertices[j];
                const aiVector3D normal = scene->mMeshes[i]->mNormals[j];

                aiVector3D texCoord(0.0f, 0.0f, 0.0f);
                if (scene->mMeshes[i]->HasTextureCoords(0))
                {
                    texCoord = scene->mMeshes[i]->mTextureCoords[0][j];
                }

                Vertices.push_back({
                    SimpleMath::Vector3(vertex.x, vertex.y, vertex.z), SimpleMath::Vector3(normal.x, normal.y, normal.z),
                    SimpleMath::Vector2(texCoord.x, texCoord.y)
                });
            }

            for (unsigned int j = 0; j < scene->mMeshes[i]->mNumFaces; ++j)
            {
                const aiFace face = scene->mMeshes[i]->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; ++k)
                {
                    Indices.push_back(face.mIndices[k]);
                }
            }
        }


        return std::make_shared<Mesh>(Vertices, Indices);
    }
}
