#include "AssetManager.h"
#include <glad/glad.h>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace assets {

    void AssetManager::TickMainThread() {
        for (;;) {
            std::function<void()> job;
            {
                std::lock_guard<std::mutex> lk(main_mtx_);
                if (main_jobs_.empty()) break;
                job = std::move(main_jobs_.front());
                main_jobs_.pop();
            }
            if (job) job();
        }
    }

    //// 把 aiMesh 转为我们的 Vertex/indices
    //static void ConvertMesh(const aiMesh* mesh, Model& out) {
    //    assets::SubmeshCPU sm;
    //    sm.vertices.reserve(mesh->mNumVertices);

    //    for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
    //        assets::Vertex vert;
    //        vert.pos = {
    //            mesh->mVertices[v].x,
    //            mesh->mVertices[v].y,
    //            mesh->mVertices[v].z
    //        };

    //        if (mesh->HasNormals()) {
    //            vert.normal = {
    //                mesh->mNormals[v].x,
    //                mesh->mNormals[v].y,
    //                mesh->mNormals[v].z
    //            };
    //        }

    //        if (mesh->HasTextureCoords(0)) {
    //            vert.uv = {
    //                mesh->mTextureCoords[0][v].x,
    //                mesh->mTextureCoords[0][v].y
    //            };
    //        }

    //        sm.vertices.push_back(vert);
    //    }

    //    // 索引（三角形）
    //    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
    //        const aiFace& face = mesh->mFaces[f];
    //        for (unsigned j = 0; j < face.mNumIndices; ++j) {
    //            sm.indices.push_back(face.mIndices[j]);
    //        }
    //    }

    //    out.meshesCPU.push_back(std::move(sm));
    //}

    //// 递归处理节点
    //static void TraverseNode(const aiNode* node, const aiScene* scene, Model& out) {
    //    for (unsigned i = 0; i < node->mNumMeshes; ++i) {
    //        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    //        ConvertMesh(mesh, out);
    //    }
    //    for (unsigned c = 0; c < node->mNumChildren; ++c) {
    //        TraverseNode(node->mChildren[c], scene, out);
    //    }
    //}

    static void ConvertMesh(const aiMesh* mesh, assets::Model& out) {
        assets::SubmeshCPU sm;
        sm.vertices.reserve(mesh->mNumVertices);

        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            assets::Vertex vert;
            vert.pos = {
                mesh->mVertices[v].x,
                mesh->mVertices[v].y,
                mesh->mVertices[v].z
            };
            if (mesh->HasNormals()) {
                vert.normal = {
                    mesh->mNormals[v].x,
                    mesh->mNormals[v].y,
                    mesh->mNormals[v].z
                };
            }
            if (mesh->HasTextureCoords(0)) {
                vert.uv = {
                    mesh->mTextureCoords[0][v].x,
                    mesh->mTextureCoords[0][v].y
                };
            }

            // NEW: 更新 AABB
            out.aabbMin = glm::min(out.aabbMin, vert.pos);
            out.aabbMax = glm::max(out.aabbMax, vert.pos);

            sm.vertices.push_back(vert);
        }

        // 索引（三角形）
        sm.indices.reserve(mesh->mNumFaces * 3);
        for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned j = 0; j < face.mNumIndices; ++j) {
                sm.indices.push_back(face.mIndices[j]);
            }
        }

        out.meshesCPU.push_back(std::move(sm));
    }

    static void TraverseNode(const aiNode* node, const aiScene* scene, assets::Model& out) {
        for (unsigned i = 0; i < node->mNumMeshes; ++i) {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ConvertMesh(mesh, out);
        }
        for (unsigned c = 0; c < node->mNumChildren; ++c) {
            TraverseNode(node->mChildren[c], scene, out);
        }
    }

    bool AssetManager::LoadModelCPU_Assimp(const std::string& path,
        const std::shared_ptr<Model>& outModel,
        std::string& errMsg) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_CalcTangentSpace |
            aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph |
            aiProcess_FlipUVs
        );
        if (!scene || !scene->mRootNode) {
            errMsg = importer.GetErrorString();
            return false;
        }

        // 清空旧数据 & 重置 AABB
        outModel->meshesCPU.clear();
        outModel->aabbMin = { FLT_MAX,  FLT_MAX,  FLT_MAX };
        outModel->aabbMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        TraverseNode(scene->mRootNode, scene, *outModel);

        if (outModel->meshesCPU.empty()) {
            errMsg = "no meshes parsed";
            return false;
        }
        return true;
    }


} // namespace assets