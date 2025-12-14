#include "voxel_octree.h"
#include <fstream>

void save_voxel_octree(const SparseVoxelOctree &scene, const std::string &path)
{
  std::ofstream fs(path, std::ios::binary);
  fs.write((const char *)&scene.header, sizeof(SparseVoxelOctreeHeader));
  uint32_t size = scene.data.size();
  fs.write((const char *)&size, sizeof(uint32_t));
  fs.write((const char *)scene.data.data(), size * sizeof(uint32_t));
  fs.flush();
  fs.close();  
}

void load_voxel_octree(SparseVoxelOctree &scene, const std::string &path)
{
  std::ifstream fs(path, std::ios::binary);
  fs.read((char *)&scene.header, sizeof(SparseVoxelOctreeHeader));
  uint32_t size = 0;
  fs.read((char *)&size, sizeof(uint32_t));
  scene.data.resize(size);
  fs.read((char *)scene.data.data(), size * sizeof(uint32_t));
  fs.close();
}