#include "octree.h"
#include <cmath>
#include <queue>
#include <algorithm>
#include <functional>

using namespace LiteMath;

// Конструкторы OctreeNode
OctreeNode::OctreeNode() 
    : voxelType(0), voxelColor(0xFF000000), 
      isUniform(false), isLeaf(true), 
      minX(0), minY(0), minZ(0), size(0) {}

OctreeNode::OctreeNode(int x, int y, int z, int sz)
    : voxelType(0), voxelColor(0xFF000000),
      isUniform(false), isLeaf(true),
      minX(x), minY(y), minZ(z), size(sz) {}

void OctreeNode::subdivide() {
    if (size <= 1 || !isLeaf) return;
    
    int childSize = size / 2;
    for (int i = 0; i < 8; i++) {
        int dx = (i & 1) ? childSize : 0;
        int dy = (i & 2) ? childSize : 0;
        int dz = (i & 4) ? childSize : 0;
        
        children[i] = std::make_unique<OctreeNode>(
            minX + dx, minY + dy, minZ + dz, childSize);
        
        children[i]->voxelType = voxelType;
        children[i]->voxelColor = voxelColor;
    }
    isLeaf = false;
}

bool OctreeNode::tryCompress() {
    if (isLeaf || children[0] == nullptr) return false;
    
    uint32_t firstType = children[0]->voxelType;
    uint32_t firstColor = children[0]->voxelColor;
    bool allSame = true;
    
    for (int i = 1; i < 8; i++) {
        if (children[i] == nullptr || 
            children[i]->voxelType != firstType ||
            children[i]->voxelColor != firstColor) {
            allSame = false;
            break;
        }
    }
    
    if (allSame) {
        for (int i = 0; i < 8; i++) {
            children[i].reset();
        }
        voxelType = firstType;
        voxelColor = firstColor;
        isLeaf = true;
        isUniform = true;
        return true;
    }
    
    return false;
}

int OctreeNode::getChildIndex(int x, int y, int z) const {
    int childSize = size / 2;
    int index = 0;
    if (x >= minX + childSize) index |= 1;
    if (y >= minY + childSize) index |= 2;
    if (z >= minZ + childSize) index |= 4;
    return index;
}

// Конструктор OctreeVoxelWorld
OctreeVoxelWorld::OctreeVoxelWorld(int sizeX, int sizeY, int sizeZ)
    : worldSizeX(sizeX), worldSizeY(sizeY), worldSizeZ(sizeZ) {
    
    int maxSize = std::max({sizeX, sizeY, sizeZ});
    int rootSize = 1;
    while (rootSize < maxSize) {
        rootSize <<= 1;
    }
    
    maxDepth = 0;
    int temp = rootSize;
    while (temp > 1) {
        maxDepth++;
        temp >>= 1;
    }
    
    root = std::make_unique<OctreeNode>(0, 0, 0, rootSize);
}

// Рекурсивное получение вокселя
Voxel OctreeVoxelWorld::getVoxelRecursive(OctreeNode* node, int x, int y, int z, int depth) const {
    if (!node) return Voxel();
    
    if (node->isLeaf || depth >= maxDepth) {
        Voxel voxel;
        voxel.type = node->voxelType;
        voxel.color = node->voxelColor;
        return voxel;
    }
    
    int childIndex = node->getChildIndex(x, y, z);
    if (node->children[childIndex]) {
        return getVoxelRecursive(node->children[childIndex].get(), x, y, z, depth + 1);
    }
    
    return Voxel();
}

Voxel OctreeVoxelWorld::getVoxel(int x, int y, int z) const {
    if (x < 0 || x >= worldSizeX || 
        y < 0 || y >= worldSizeY || 
        z < 0 || z >= worldSizeZ) {
        return Voxel();
    }
    
    return getVoxelRecursive(root.get(), x, y, z, 0);
}

bool OctreeVoxelWorld::isSolid(int x, int y, int z) const {
    Voxel v = getVoxel(x, y, z);
    return v.type != 0;
}

float3 OctreeVoxelWorld::getNormal(int x, int y, int z) const {
    float3 normal(0.0f, 0.0f, 0.0f);
    
    if (x > 0 && !isSolid(x-1, y, z)) normal.x = -1.0f;
    else if (x < worldSizeX-1 && !isSolid(x+1, y, z)) normal.x = 1.0f;
    
    if (y > 0 && !isSolid(x, y-1, z)) normal.y = -1.0f;
    else if (y < worldSizeY-1 && !isSolid(x, y+1, z)) normal.y = 1.0f;
    
    if (z > 0 && !isSolid(x, y, z-1)) normal.z = -1.0f;
    else if (z < worldSizeZ-1 && !isSolid(x, y, z+1)) normal.z = 1.0f;
    
    if (length(normal) < 0.1f) {
        return float3(0.0f, 1.0f, 0.0f);
    }
    
    return normalize(normal);
}

// Рекурсивная вставка
void OctreeVoxelWorld::insertVoxelRecursive(OctreeNode* node, int x, int y, int z, 
                                           const Voxel& voxel, int depth) {
    if (node->size == 1 || depth >= maxDepth) {
        node->voxelType = voxel.type;
        node->voxelColor = voxel.color;
        node->isLeaf = true;
        return;
    }
    
    if (node->isLeaf) {
        if (node->voxelType == voxel.type && node->voxelColor == voxel.color) {
            return;
        }
        
        node->subdivide();
        
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                node->children[i]->voxelType = node->voxelType;
                node->children[i]->voxelColor = node->voxelColor;
            }
        }
    }
    
    int childIndex = node->getChildIndex(x, y, z);
    if (!node->children[childIndex]) {
        int childSize = node->size / 2;
        int dx = (childIndex & 1) ? childSize : 0;
        int dy = (childIndex & 2) ? childSize : 0;
        int dz = (childIndex & 4) ? childSize : 0;
        
        node->children[childIndex] = std::make_unique<OctreeNode>(
            node->minX + dx, node->minY + dy, node->minZ + dz, childSize);
    }
    
    insertVoxelRecursive(node->children[childIndex].get(), x, y, z, voxel, depth + 1);
}

void OctreeVoxelWorld::setVoxel(int x, int y, int z, const Voxel& voxel) {
    if (x < 0 || x >= worldSizeX || 
        y < 0 || y >= worldSizeY || 
        z < 0 || z >= worldSizeZ) {
        return;
    }
    
    if (voxel.type != 0) {
        insertVoxelRecursive(root.get(), x, y, z, voxel, 0);
    }
}

// Сжатие дерева
void OctreeVoxelWorld::compressTree() {
    std::function<bool(OctreeNode*)> compressRecursive = [&](OctreeNode* node) -> bool {
        if (!node || node->isLeaf) return false;
        
        bool compressedAny = false;
        
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                if (compressRecursive(node->children[i].get())) {
                    compressedAny = true;
                }
            }
        }
        
        if (node->tryCompress()) {
            compressedAny = true;
        }
        
        return compressedAny;
    };
    
    bool compressed;
    do {
        compressed = compressRecursive(root.get());
    } while (compressed);
}

// Упрощенная трассировка лучей (базовая версия)
bool OctreeVoxelWorld::rayCastRecursive(OctreeNode* node, const float3& rayStart,
                                       const float3& rayDir, float tMin, float tMax,
                                       float3& hitPos, float3& normal, Voxel& hitVoxel) const {
    if (!node) return false;
    
    if (node->isLeaf) {
        if (node->voxelType == 0) return false;
        
        // Упрощенная проверка пересечения
        float3 voxelCenter(node->minX + node->size/2.0f, 
                          node->minY + node->size/2.0f,
                          node->minZ + node->size/2.0f);
        float3 voxelHalfSize(node->size/2.0f);
        
        float t0 = tMin, t1 = tMax;
        for (int i = 0; i < 3; i++) {
            float invD = 1.0f / rayDir[i];
            float tNear = (voxelCenter[i] - voxelHalfSize[i] - rayStart[i]) * invD;
            float tFar = (voxelCenter[i] + voxelHalfSize[i] - rayStart[i]) * invD;
            if (invD < 0.0f) std::swap(tNear, tFar);
            
            t0 = std::max(t0, tNear);
            t1 = std::min(t1, tFar);
            if (t0 > t1) return false;
        }
        
        if (t0 < tMax) {
            hitPos = rayStart + rayDir * t0;
            hitVoxel.type = node->voxelType;
            hitVoxel.color = node->voxelColor;
            
            // Простая нормаль
            float3 localHit = hitPos - voxelCenter;
            float3 absHit = abs(localHit);
            
            if (absHit.x > absHit.y && absHit.x > absHit.z) {
                normal = float3(localHit.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
            } else if (absHit.y > absHit.z) {
                normal = float3(0.0f, localHit.y > 0 ? 1.0f : -1.0f, 0.0f);
            } else {
                normal = float3(0.0f, 0.0f, localHit.z > 0 ? 1.0f : -1.0f);
            }
            
            return true;
        }
        return false;
    }
    
    // Простой обход детей
    for (int i = 0; i < 8; i++) {
        if (!node->children[i]) continue;
        
        float3 childCenter(node->children[i]->minX + node->children[i]->size/2.0f,
                          node->children[i]->minY + node->children[i]->size/2.0f,
                          node->children[i]->minZ + node->children[i]->size/2.0f);
        float3 childHalfSize(node->children[i]->size/2.0f);
        
        float t0 = tMin, t1 = tMax;
        bool intersect = true;
        
        for (int j = 0; j < 3; j++) {
            float invD = 1.0f / rayDir[j];
            float tNear = (childCenter[j] - childHalfSize[j] - rayStart[j]) * invD;
            float tFar = (childCenter[j] + childHalfSize[j] - rayStart[j]) * invD;
            if (invD < 0.0f) std::swap(tNear, tFar);
            
            t0 = std::max(t0, tNear);
            t1 = std::min(t1, tFar);
            if (t0 > t1) {
                intersect = false;
                break;
            }
        }
        
        if (intersect && t0 < tMax) {
            float3 tempHitPos, tempNormal;
            Voxel tempVoxel;
            
            if (rayCastRecursive(node->children[i].get(), rayStart, rayDir,
                                t0, t1, tempHitPos, tempNormal, tempVoxel)) {
                hitPos = tempHitPos;
                normal = tempNormal;
                hitVoxel = tempVoxel;
                return true;
            }
        }
    }
    
    return false;
}

bool OctreeVoxelWorld::rayCast(const float3& origin, const float3& direction,
                              float maxDist, float3& hitPos, float3& normal,
                              Voxel& hitVoxel) const {
    float3 rayStart = origin + float3(worldSizeX/2.0f, 0, worldSizeZ/2.0f);
    float3 rayDir = normalize(direction);
    
    // Проверка границ
    float3 worldMin(0, 0, 0);
    float3 worldMax(worldSizeX, worldSizeY, worldSizeZ);
    
    float tMin = 0.0f, tMax = maxDist;
    for (int i = 0; i < 3; i++) {
        float invD = 1.0f / rayDir[i];
        float tNear = (worldMin[i] - rayStart[i]) * invD;
        float tFar = (worldMax[i] - rayStart[i]) * invD;
        if (invD < 0.0f) std::swap(tNear, tFar);
        
        tMin = std::max(tMin, tNear);
        tMax = std::min(tMax, tFar);
        if (tMin > tMax) return false;
    }
    
    if (rayCastRecursive(root.get(), rayStart, rayDir, tMin, tMax,
                        hitPos, normal, hitVoxel)) {
        hitPos = hitPos - float3(worldSizeX/2.0f, 0, worldSizeZ/2.0f);
        return true;
    }
    
    return false;
}

// Подсчет узлов
int OctreeVoxelWorld::getNodeCount() const {
    std::function<int(const OctreeNode*)> countRecursive = [&](const OctreeNode* node) -> int {
        if (!node) return 0;
        int count = 1;
        if (!node->isLeaf) {
            for (int i = 0; i < 8; i++) {
                count += countRecursive(node->children[i].get());
            }
        }
        return count;
    };
    
    return countRecursive(root.get());
}

int OctreeVoxelWorld::getLeafCount() const {
    std::function<int(const OctreeNode*)> countRecursive = [&](const OctreeNode* node) -> int {
        if (!node) return 0;
        if (node->isLeaf) return 1;
        
        int count = 0;
        for (int i = 0; i < 8; i++) {
            count += countRecursive(node->children[i].get());
        }
        return count;
    };
    
    return countRecursive(root.get());
}

size_t OctreeVoxelWorld::getMemoryUsage() const {
    int nodeCount = getNodeCount();
    return nodeCount * sizeof(OctreeNode) + sizeof(*this);
}

std::string OctreeVoxelWorld::getDescription() const {
    int nodes = getNodeCount();
    int leaves = getLeafCount();
    float compression = 100.0f * (1.0f - (float)nodes / (worldSizeX * worldSizeY * worldSizeZ));
    
    return "Octree (" + 
           std::to_string(worldSizeX) + "x" + 
           std::to_string(worldSizeY) + "x" + 
           std::to_string(worldSizeZ) + "), " +
           "Nodes: " + std::to_string(nodes) + ", " +
           "Leaves: " + std::to_string(leaves);
}