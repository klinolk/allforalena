#ifndef VOXEL_H
#define VOXEL_H

#include "LiteMath.h"
#include <cstdint>
#include <string>

// Структура вокселя с материалами
struct Voxel {
    uint32_t type;        // тип материала: 0=air, 1=grass, 2=dirt, 3=stone, 4=water
    uint32_t color;       // цвет в формате RGBA
    LiteMath::float3 normal;        // нормаль
    uint8_t density;      // плотность
    uint8_t metadata;     // дополнительные данные
    
    Voxel() : type(0), color(0xFF000000), normal(0,0,0), density(0), metadata(0) {}
    
    // Простой конструктор
    Voxel(uint32_t t, uint32_t c) : type(t), color(c), normal(0,0,0), density(0), metadata(0) {}
};

// Абстрактный интерфейс для воксельного мира
class IVoxelWorld {
public:
    virtual ~IVoxelWorld() = default;
    
    // Основные методы доступа
    virtual Voxel getVoxel(int x, int y, int z) const = 0;
    virtual bool isSolid(int x, int y, int z) const = 0;
    virtual LiteMath::float3 getNormal(int x, int y, int z) const = 0;
    
    // Информация о размерах
    virtual int getSizeX() const = 0;
    virtual int getSizeY() const = 0;
    virtual int getSizeZ() const = 0;
    
    // Для отладки и оптимизации
    virtual size_t getMemoryUsage() const = 0;
    virtual std::string getDescription() const = 0;
    
    // Метод для трассировки лучей
    virtual bool rayCast(const LiteMath::float3& origin, const LiteMath::float3& direction,
                        float maxDist, LiteMath::float3& hitPos, LiteMath::float3& normal,
                        Voxel& hitVoxel) const = 0;
};

#endif // VOXEL_H