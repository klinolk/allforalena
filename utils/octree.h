#ifndef OCTREE_VOXEL_WORLD_H
#define OCTREE_VOXEL_WORLD_H

#include "voxel.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

// Структура узла октодерева
struct OctreeNode {
    std::unique_ptr<OctreeNode> children[8];
    uint32_t voxelType;
    uint32_t voxelColor;
    bool isUniform;
    bool isLeaf;
    int minX, minY, minZ;
    int size;
    
    OctreeNode();
    OctreeNode(int x, int y, int z, int sz);
    void subdivide();
    bool tryCompress();
    int getChildIndex(int x, int y, int z) const;
};

// Класс октодерева для воксельного мира
class OctreeVoxelWorld : public IVoxelWorld {
private:
    std::unique_ptr<OctreeNode> root;
    int worldSizeX, worldSizeY, worldSizeZ;
    int maxDepth;
    
    // Рекурсивные методы
    Voxel getVoxelRecursive(OctreeNode* node, int x, int y, int z, int depth) const;
    void insertVoxelRecursive(OctreeNode* node, int x, int y, int z, const Voxel& voxel, int depth);
    bool rayCastRecursive(OctreeNode* node, const LiteMath::float3& rayStart,
                         const LiteMath::float3& rayDir, float tMin, float tMax,
                         LiteMath::float3& hitPos, LiteMath::float3& normal, Voxel& hitVoxel) const;
    
public:
    OctreeVoxelWorld(int sizeX, int sizeY, int sizeZ);
    
    // Реализация интерфейса IVoxelWorld
    Voxel getVoxel(int x, int y, int z) const override;
    bool isSolid(int x, int y, int z) const override;
    LiteMath::float3 getNormal(int x, int y, int z) const override;
    bool rayCast(const LiteMath::float3& origin, const LiteMath::float3& direction,
                float maxDist, LiteMath::float3& hitPos, LiteMath::float3& normal,
                Voxel& hitVoxel) const override;
    
    // Дополнительные методы
    void setVoxel(int x, int y, int z, const Voxel& voxel);
    void compressTree();
    
    // Методы IVoxelWorld
    int getSizeX() const override { return worldSizeX; }
    int getSizeY() const override { return worldSizeY; }
    int getSizeZ() const override { return worldSizeZ; }
    
    size_t getMemoryUsage() const override;
    std::string getDescription() const override;
    
    // Информация
    int getNodeCount() const;
    int getLeafCount() const;
};

#endif // OCTREE_VOXEL_WORLD_H