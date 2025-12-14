#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "utils/mesh.h"
#include "utils/LiteMath.h"
#include "utils/public_camera.h"
#include "utils/public_image.h"

#include <cstdio>
#include <cstring>
#include <SDL_keycode.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <SDL.h>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cfloat>

using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::int2;
using LiteMath::int3;
using LiteMath::int4;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;

// ============ УВЕЛИЧИВАЕМ РАЗМЕР ЛАНДШАФТА ============
// Увеличиваем размер воксельной сетки (было 64x64x64, теперь 128x64x128)
const int GRID_SIZE_X = 128;  // Увеличили по X
const int GRID_SIZE_Y = 64;   // Оставляем по Y (высота)
const int GRID_SIZE_Z = 128;  // Увеличили по Z

// Типы вокселей
enum VoxelType {
    AIR = 0,
    SURFACE = 1
};

// Воксельная сетка
std::vector<std::vector<std::vector<VoxelType>>> voxelGrid;

// voxel_interface.h
#pragma once
#include "utils/LiteMath.h"
#include <cstdint>
#include <vector>
#include <memory>

// 1. Более богатая структура вокселя (для текстур и материалов)
struct Voxel {
    uint32_t type;        // тип материала: 0=air, 1=grass, 2=dirt, 3=stone, etc.
    uint32_t color;       // цвет в формате RGBA (пока используем это)
    float3 normal;        // нормаль (может быть вычислена или сохранена)
    uint8_t density;      // плотность (для полупрозрачных материалов)
    uint8_t metadata;     // дополнительные данные (влажность, температура и т.д.)
    
    Voxel() : type(0), color(0xFF000000), normal(0,0,0), density(0), metadata(0) {}
};

// 2. Интерфейс для воксельного мира (общий для сетки и октодерева)
class IVoxelWorld {
public:
    virtual ~IVoxelWorld() = default;
    
    // Основные методы доступа
    virtual Voxel getVoxel(int x, int y, int z) const = 0;
    virtual bool isSolid(int x, int y, int z) const = 0;
    virtual float3 getNormal(int x, int y, int z) const = 0;
    
    // Информация о размерах
    virtual int getSizeX() const = 0;
    virtual int getSizeY() const = 0;
    virtual int getSizeZ() const = 0;
    
    // Для отладки и оптимизации
    virtual size_t getMemoryUsage() const = 0;
    virtual std::string getDescription() const = 0;
    
    // Метод для трассировки лучей (опционально, может быть в отдельном классе)
    virtual bool rayCast(const float3& origin, const float3& direction,
                        float maxDist, float3& hitPos, float3& normal,
                        Voxel& hitVoxel) const = 0;
};

// 3. Реализация на основе регулярной сетки (ВАША текущая реализация)
class GridVoxelWorld : public IVoxelWorld {
private:
    std::vector<std::vector<std::vector<Voxel>>> grid;
    int sizeX, sizeY, sizeZ;
    
public:
    GridVoxelWorld(int sx, int sy, int sz);
    
    // Установка вокселя (для генерации ландшафта)
    void setVoxel(int x, int y, int z, const Voxel& voxel);
    
    // Реализация интерфейса
    Voxel getVoxel(int x, int y, int z) const override;
    bool isSolid(int x, int y, int z) const override;
    float3 getNormal(int x, int y, int z) const override;
    bool rayCast(const float3& origin, const float3& direction,
                float maxDist, float3& hitPos, float3& normal,
                Voxel& hitVoxel) const override;
    
    int getSizeX() const override { return sizeX; }
    int getSizeY() const override { return sizeY; }
    int getSizeZ() const override { return sizeZ; }
    
    size_t getMemoryUsage() const override;
    std::string getDescription() const override;
};

// 4. Вспомогательные функции для генерации ландшафта
namespace TerrainGenerator {
    void createHillyTerrain(GridVoxelWorld& world, float baseHeight = 20.0f);
    void createMountainRange(GridVoxelWorld& world, int mountainCount = 5);
    void createProceduralLandscape(GridVoxelWorld& world, int seed = 42);
    void createTestScene(GridVoxelWorld& world); // Простая сцена для тестирования
}

// 5. Утилиты для материалов и цветов
namespace VoxelMaterials {
    Voxel createVoxel(uint32_t type, uint32_t color = 0xFFFFFFFF);
    Voxel createAir();
    Voxel createGrass(float heightRatio = 1.0f);
    Voxel createDirt();
    Voxel createStone();
    Voxel createWater();
    
    uint32_t getColorByType(uint32_t type, float y = 0.0f);
    float3 getColorAsFloat3(uint32_t color);
}

// Инициализация сетки
void initVoxelGrid() {
    voxelGrid.resize(GRID_SIZE_X, 
        std::vector<std::vector<VoxelType>>(GRID_SIZE_Y,
            std::vector<VoxelType>(GRID_SIZE_Z, AIR)));
    
    // Заполнение сетки - создание более детального ландшафта
    float base_y = GRID_SIZE_Y * 0.3f;
    
    for (int x = 0; x < GRID_SIZE_X; x++) {
        for (int z = 0; z < GRID_SIZE_Z; z++) {
            // Более сложная периодическая функция для высоты
            float fx = sin(x * 0.1f) * 0.7f;  // Меньшая частота для плавных холмов
            float fz = cos(z * 0.08f) * 0.5f; // Еще одна частота
            float hills = sin(x * 0.03f + z * 0.05f) * 1.2f; // Холмы
            
            float height = base_y + (fx + fz + hills) * 8.0f; // Увеличили амплитуду
            
            int y_height = static_cast<int>(height);
            y_height = std::clamp(y_height, 0, GRID_SIZE_Y - 1);
            
            // Все воксели ниже высоты - поверхность
            for (int y = 0; y <= y_height; y++) {
                voxelGrid[x][y][z] = SURFACE;
            }
        }
    }
}

struct DDAHit {
    bool hit;
    float3 position;
    float3 normal;
    float distance;
    int voxelType;
};

DDAHit rayCastDDA(const float3& rayOrigin, const float3& rayDir, float maxDistance = 1000.0f) {
    // Преобразуем мировые координаты в координаты сетки
    float3 gridOrigin = rayOrigin + float3(GRID_SIZE_X/2.0f, 0, GRID_SIZE_Z/2.0f);
    
    // Если начало луча вне сетки, находим точку входа
    float3 rayStart = gridOrigin;
    
    // Направление луча
    float3 dir = LiteMath::normalize(rayDir);
    
    // Текущая позиция в координатах сетки
    float3 pos = rayStart;
    
    // Определяем текущий воксель
    int x = static_cast<int>(floor(pos.x));
    int y = static_cast<int>(floor(pos.y));
    int z = static_cast<int>(floor(pos.z));
    
    // Проверяем, находимся ли мы внутри сетки
    if (x < 0 || x >= GRID_SIZE_X || y < 0 || y >= GRID_SIZE_Y || z < 0 || z >= GRID_SIZE_Z) {
        // Находим точку входа в сетку
        float t = 0.0f;
        
        // Ищем ближайшую точку входа в сетку
        float tMin = 0.0f;
        float tMax = maxDistance;
        
        for (int i = 0; i < 3; i++) {
            if (dir[i] != 0) {
                float t1 = (0 - pos[i]) / dir[i];
                float t2 = ((i == 0 ? GRID_SIZE_X : (i == 1 ? GRID_SIZE_Y : GRID_SIZE_Z)) - 1 - pos[i]) / dir[i];
                float tNear = std::min(t1, t2);
                float tFar = std::max(t1, t2);
                
                tMin = std::max(tMin, tNear);
                tMax = std::min(tMax, tFar);
                
                if (tMin > tMax) {
                    return {false, float3(0.0f), float3(0.0f), 0.0f, AIR};
                }
            }
        }
        
        if (tMin > 0) {
            pos = pos + dir * tMin;
            x = static_cast<int>(floor(pos.x));
            y = static_cast<int>(floor(pos.y));
            z = static_cast<int>(floor(pos.z));
        } else {
            return {false, float3(0.0f), float3(0.0f), 0.0f, AIR};
        }
    }
    
    // Шаги по осям
    int stepX = (dir.x > 0) ? 1 : -1;
    int stepY = (dir.y > 0) ? 1 : -1;
    int stepZ = (dir.z > 0) ? 1 : -1;
    
    // Расстояние до следующей границы вокселя
    float tMaxX, tMaxY, tMaxZ;
    
    // Вычисляем начальные значения tMax
    float nextX = (stepX > 0) ? (x + 1) : x;
    float nextY = (stepY > 0) ? (y + 1) : y;
    float nextZ = (stepZ > 0) ? (z + 1) : z;
    
    tMaxX = (dir.x != 0) ? (nextX - pos.x) / dir.x : FLT_MAX;
    tMaxY = (dir.y != 0) ? (nextY - pos.y) / dir.y : FLT_MAX;
    tMaxZ = (dir.z != 0) ? (nextZ - pos.z) / dir.z : FLT_MAX;
    
    // Расстояние для перехода к следующему вокселю
    float tDeltaX = (dir.x != 0) ? fabs(1.0f / dir.x) : FLT_MAX;
    float tDeltaY = (dir.y != 0) ? fabs(1.0f / dir.y) : FLT_MAX;
    float tDeltaZ = (dir.z != 0) ? fabs(1.0f / dir.z) : FLT_MAX;
    
    float distance = 0;
    
    // Основной цикл DDA
    while (distance < maxDistance) {
        // Проверяем границы
        if (x < 0 || x >= GRID_SIZE_X || y < 0 || y >= GRID_SIZE_Y || z < 0 || z >= GRID_SIZE_Z) {
            break;
        }
        
        // Проверяем, попали ли в воксель поверхности
        if (voxelGrid[x][y][z] == SURFACE) {
            float3 hitPos = pos - float3(GRID_SIZE_X/2.0f, 0, GRID_SIZE_Z/2.0f);
            
            // Вычисляем нормаль (проверяем соседние воксели)
            float3 normal(0.0f, 0.0f, 0.0f);
            
            // Проверяем соседей для определения нормали
            if (x > 0 && voxelGrid[x-1][y][z] == AIR) normal.x = -1.0f;
            else if (x < GRID_SIZE_X-1 && voxelGrid[x+1][y][z] == AIR) normal.x = 1.0f;
            
            if (y > 0 && voxelGrid[x][y-1][z] == AIR) normal.y = -1.0f;
            else if (y < GRID_SIZE_Y-1 && voxelGrid[x][y+1][z] == AIR) normal.y = 1.0f;
            
            if (z > 0 && voxelGrid[x][y][z-1] == AIR) normal.z = -1.0f;
            else if (z < GRID_SIZE_Z-1 && voxelGrid[x][y][z+1] == AIR) normal.z = 1.0f;
            
            // Если нормаль нулевая (воксель полностью окружен), используем аппроксимацию
            if (LiteMath::length(normal) < 0.1f) {
                normal = float3(0.0f, 1.0f, 0.0f);
            } else {
                normal = LiteMath::normalize(normal);
            }
            
            return {true, hitPos, normal, distance, SURFACE};
        }
        
        // Переход к следующему вокселю
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            x += stepX;
            distance = tMaxX;
            tMaxX += tDeltaX;
        } else if (tMaxY < tMaxZ) {
            y += stepY;
            distance = tMaxY;
            tMaxY += tDeltaY;
        } else {
            z += stepZ;
            distance = tMaxZ;
            tMaxZ += tDeltaZ;
        }
        
        // Обновляем позицию
        pos = rayStart + dir * distance;
    }
    
    return {false, float3(0.0f), float3(0.0f), 0.0f, AIR};
}

// ============ УВЕЛИЧИВАЕМ РАЗРЕШЕНИЕ ============
//static constexpr int SCREEN_WIDTH  = 1280;  // Было 640
//static constexpr int SCREEN_HEIGHT = 960;   // Было 480 (соотношение 4:3)

static constexpr int SCREEN_WIDTH  = 640;  // Вернем к старому
static constexpr int SCREEN_HEIGHT = 480;

float rad_to_deg(float rad) { return rad * 180.0f / LiteMath::M_PI; }

uint32_t float3_to_RGBA8(float3 c)
{
  uint8_t r = (uint8_t)(std::clamp(c.x,0.0f,1.0f)*255.0f);
  uint8_t g = (uint8_t)(std::clamp(c.y,0.0f,1.0f)*255.0f);
  uint8_t b = (uint8_t)(std::clamp(c.z,0.0f,1.0f)*255.0f);
  return 0xFF000000 | (r<<16) | (g<<8) | b;
}

void voxel_landscape_demo(const Camera &camera, uint32_t *out_image, int W, int H) {
    LiteMath::float4x4 view = LiteMath::lookAt(camera.pos, camera.target, camera.up);
    LiteMath::float4x4 proj = LiteMath::perspectiveMatrix(rad_to_deg(camera.fov_rad), 
                                                          (float)W/(float)H, 
                                                          camera.z_near, 
                                                          camera.z_far);
    LiteMath::float4x4 viewProjInv = LiteMath::inverse4x4(proj*view);
    
    // Направление источника света
    const float3 light_dir = LiteMath::normalize(float3(-1.0f, -1.0f, -1.0f));
    // Светло-серый базовый цвет
    const float3 base_color = float3(0.8f, 0.8f, 0.8f);
    
    // Добавляем простой антиалиасинг - суперсэмплинг 2x2
    const int samples = 2; // 2x2 = 4 сэмпла на пиксель
    const float inv_samples = 1.0f / (samples * samples);
    
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float3 color_accum = float3(0.0f, 0.0f, 0.0f);
            
            // Суперсэмплинг для антиалиасинга
            for (int sy = 0; sy < samples; sy++) {
                for (int sx = 0; sx < samples; sx++) {
                    // Смещение внутри пикселя
                    float u = (x + (sx + 0.5f) / samples) / W;
                    float v = (y + (sy + 0.5f) / samples) / H;
                    float ndc_x = 2.0f * u - 1.0f;
                    float ndc_y = 1.0f - 2.0f * v; // Инвертируем ось Y
                    
                    float4 point_NDC = float4(ndc_x, ndc_y, 0.0f, 1.0f);
                    float4 point_W = viewProjInv * point_NDC;
                    float3 point = LiteMath::to_float3(point_W) / point_W.w;
                    float3 ray_pos = camera.pos;
                    float3 ray_dir = LiteMath::normalize(point - ray_pos);
                    
                    // Трассируем луч через воксельную сетку
                    DDAHit hit = rayCastDDA(ray_pos, ray_dir);
                    
                    if (hit.hit) {
                        // Модель освещения Ламберта
                        float lambert = std::max(0.0f, LiteMath::dot(LiteMath::normalize(hit.normal), -light_dir));
                        color_accum += base_color * (0.25f + 0.75f * lambert);
                    }
                }
            }
            
            // Усредняем сэмплы
            float3 final_color = color_accum * inv_samples;
            out_image[y*W + x] = float3_to_RGBA8(final_color);
        }
    }
}

void draw_frame_example(const Camera &camera, std::vector<uint32_t> &pixels) {
    voxel_landscape_demo(camera, pixels.data(), SCREEN_WIDTH, SCREEN_HEIGHT);
}

// ============ УПРАВЛЕНИЕ КАМЕРОЙ ============
struct FreeCameraModel
{
    enum class CameraMoveType : uint8_t
    {
        NONE,
        TUMBLE,
        TRACK,
        DOLLY
    };
    FreeCameraModel() = default;
    CameraMoveType move_type{CameraMoveType::NONE};
    int2 mouse_pos;
    float theta{0};
    float phi{0};
    float3 look_at{0, 0, 0};
    float dist_to_target{10};
};

FreeCameraModel freecam_model;
constexpr float kRotateAmpl = 0.005f;
constexpr float kPanAmpl = 0.01f;
constexpr float kScrollAmpl = 0.1f;

enum EventFlags
{
    EF_NONE = 0,
    EF_SHIFT_DOWN = 1 << 0,
    EF_CONTROL_DOWN = 1 << 1,
    EF_ALT_DOWN = 1 << 2,
    EF_LEFT_DOWN = 1 << 3,
    EF_MIDDLE_DOWN = 1 << 4,
    EF_RIGHT_DOWN = 1 << 5
};

void OnMousePressed(int flags, int2 location)
{
    freecam_model.mouse_pos = location;
    if (flags & EF_ALT_DOWN) {
        freecam_model.move_type = (flags & EF_LEFT_DOWN)     ? FreeCameraModel::CameraMoveType::TUMBLE
                                  : (flags & EF_MIDDLE_DOWN) ? FreeCameraModel::CameraMoveType::TRACK
                                  : (flags & EF_RIGHT_DOWN)  ? FreeCameraModel::CameraMoveType::DOLLY
                                                             : FreeCameraModel::CameraMoveType::NONE;
    }
}

void OnMouseReleased()
{
    freecam_model.move_type = FreeCameraModel::CameraMoveType::NONE;
}

void OnMouseMoved(int flags, int2 location, Camera &camera)
{
    if (freecam_model.move_type == FreeCameraModel::CameraMoveType::NONE) {
        return;
    }

    int2 delta = location - freecam_model.mouse_pos;
    freecam_model.mouse_pos = location;

    switch (freecam_model.move_type) {
    case FreeCameraModel::CameraMoveType::TUMBLE: {
        freecam_model.theta -= delta.x * kRotateAmpl;
        freecam_model.phi -= delta.y * kRotateAmpl;
        freecam_model.phi = std::clamp(freecam_model.phi, -LiteMath::M_PI / 2 + 0.1f, LiteMath::M_PI / 2 - 0.1f);
        float x = freecam_model.dist_to_target * cos(freecam_model.phi) * sin(freecam_model.theta);
        float y = freecam_model.dist_to_target * sin(freecam_model.phi);
        float z = freecam_model.dist_to_target * cos(freecam_model.phi) * cos(freecam_model.theta);
        camera.pos = freecam_model.look_at + float3(x, y, z);
        camera.target = freecam_model.look_at;
        break;
    }
    case FreeCameraModel::CameraMoveType::TRACK: {
        float3 forward = LiteMath::normalize(camera.target - camera.pos);
        float3 right = LiteMath::normalize(LiteMath::cross(float3(0, 1, 0), forward));
        float3 up = LiteMath::normalize(LiteMath::cross(forward, right));

        float3 move = right * (-delta.x * kPanAmpl) + up * (-delta.y * kPanAmpl);
        camera.pos += move;
        camera.target += move;
        freecam_model.look_at += move;
        break;
    }
    case FreeCameraModel::CameraMoveType::DOLLY: {
        float3 forward = LiteMath::normalize(camera.target - camera.pos);
        float3 move = forward * ((delta.x + delta.y) * kScrollAmpl);
        camera.pos += move;
        camera.target += move;
        freecam_model.dist_to_target = LiteMath::length(camera.pos - freecam_model.look_at);
        break;
    }
    default:
        break;
    }
}

void OnMouseWheel(int delta, Camera &camera)
{
    float3 forward = LiteMath::normalize(camera.target - camera.pos);
    float3 move = forward * (delta * kScrollAmpl);
    camera.pos += move;
    camera.target += move;
    freecam_model.dist_to_target = LiteMath::length(camera.pos - freecam_model.look_at);
}

void WASD(Camera &camera, float dt)
{
    float moveSpeed = 10.0f * dt;  // Увеличили скорость движения для большего ландшафта
    float3 forward = LiteMath::normalize(camera.target - camera.pos);
    float3 right = LiteMath::normalize(LiteMath::cross(float3(0, 1, 0), forward));
    float3 up = LiteMath::normalize(LiteMath::cross(forward, right));

    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    float3 move(0, 0, 0);
    if (keystate[SDL_SCANCODE_W]) {
        move += forward * moveSpeed;
    }
    if (keystate[SDL_SCANCODE_S]) {
        move -= forward * moveSpeed;
    }
    if (keystate[SDL_SCANCODE_A]) {
        move += right * moveSpeed;
    }
    if (keystate[SDL_SCANCODE_D]) {
        move -= right * moveSpeed;
    }
    if (keystate[SDL_SCANCODE_Q]) {
        move += up * moveSpeed;
    }
    if (keystate[SDL_SCANCODE_E]) {
        move -= up * moveSpeed;
    }

    camera.pos += move;
    camera.target += move;
    freecam_model.look_at += move;

    freecam_model.dist_to_target = LiteMath::length(camera.pos - freecam_model.look_at);
    float3 dir = LiteMath::normalize(camera.pos - camera.target);
    freecam_model.theta = atan2(dir.x, dir.z);
    freecam_model.phi = asin(dir.y);
}

void InitializeFreeCameraFromCamera(const Camera &camera)
{
    freecam_model.look_at = camera.target;
    freecam_model.dist_to_target = LiteMath::length(camera.pos - camera.target);

    float3 dir = LiteMath::normalize(camera.pos - camera.target);
    freecam_model.theta = atan2(dir.x, dir.z);
    freecam_model.phi = asin(dir.y);
}

int main(int argc, char **args)
{
    // Инициализация воксельной сетки
    initVoxelGrid();
    
    // Pixel buffer (RGBA format)
    std::vector<uint32_t> pixels(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFFFFFFFF);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create our window
    SDL_Window *window = SDL_CreateWindow("Voxel Landscape - High Resolution", 
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create a texture
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);

    if (!texture)
    {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Event ev;
    bool running = true;

    Camera camera;
    // Настройка камеры для обзора большего ландшафта
    camera.pos = float3(0.0f, 50.0f, 100.0f);   // Отодвинули камеру еще дальше
    camera.target = float3(0.0f, 20.0f, 0.0f);   // Смотрим выше
    camera.up = float3(0.0f, 1.0f, 0.0f);
    camera.fov_rad = LiteMath::M_PI / 4.0f; // 45 градусов
    camera.z_near = 1.0f;
    camera.z_far = 300.0f;  // Увеличили дальность видимости

    InitializeFreeCameraFromCamera(camera);
    
    bool alt_pressed = false;
    bool mouse_left = false;
    bool mouse_middle = false;
    bool mouse_right = false;

    auto time = std::chrono::high_resolution_clock::now();
    auto prev_time = time;
    float time_from_start = 0;
    uint32_t frameNum = 0;

    printf("Voxel Landscape Parameters:\n");
    printf("  Grid size: %d x %d x %d\n", GRID_SIZE_X, GRID_SIZE_Y, GRID_SIZE_Z);
    printf("  Screen resolution: %d x %d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("  Controls:\n");
    printf("    - Alt + Left Mouse: Rotate camera\n");
    printf("    - Alt + Middle Mouse: Pan\n");
    printf("    - Alt + Right Mouse: Dolly (move forward/backward)\n");
    printf("    - Mouse Wheel: Zoom\n");
    printf("    - WASD: Move camera\n");
    printf("    - Q/E: Move up/down\n");
    printf("    - ESC: Exit\n");

    // Main loop
    while (running)
    {
        // Process input
        while (SDL_PollEvent(&ev) != 0)
        {
            switch (ev.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            
            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                // Update modifier keys
                if (ev.key.keysym.sym == SDLK_LALT || ev.key.keysym.sym == SDLK_RALT) {
                    alt_pressed = true;
                }
                break;
            
            case SDL_KEYUP:
                if (ev.key.keysym.sym == SDLK_LALT || ev.key.keysym.sym == SDLK_RALT) {
                    alt_pressed = false;
                }
                break;
            
            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    mouse_left = true;
                }
                if (ev.button.button == SDL_BUTTON_MIDDLE) {
                    mouse_middle = true;
                }
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    mouse_right = true;
                }

                if (alt_pressed) {
                    int flags = EF_ALT_DOWN;
                    if (mouse_left) {
                        flags |= EF_LEFT_DOWN;
                    }
                    if (mouse_middle) {
                        flags |= EF_MIDDLE_DOWN;
                    }
                    if (mouse_right) {
                        flags |= EF_RIGHT_DOWN;
                    }

                    OnMousePressed(flags, LiteMath::int2(ev.button.x, ev.button.y));
                }
                break;
            
            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    mouse_left = false;
                }
                if (ev.button.button == SDL_BUTTON_MIDDLE) {
                    mouse_middle = false;
                }
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    mouse_right = false;
                }
                OnMouseReleased();
                break;
            
            case SDL_MOUSEMOTION:
                if (alt_pressed && (mouse_left || mouse_middle || mouse_right)) {
                    int flags = EF_ALT_DOWN;
                    if (mouse_left) {
                        flags |= EF_LEFT_DOWN;
                    }
                    if (mouse_middle) {
                        flags |= EF_MIDDLE_DOWN;
                    }
                    if (mouse_right) {
                        flags |= EF_RIGHT_DOWN;
                    }

                    OnMouseMoved(flags, LiteMath::int2(ev.motion.x, ev.motion.y), camera);
                }
                break;
            
            case SDL_MOUSEWHEEL:
                OnMouseWheel(ev.wheel.y, camera);
                break;
            }
        }

        // Update timing
        prev_time = time;
        time = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::milli>(time - prev_time).count() / 1000.0f;
        time_from_start += dt;
        frameNum++;

        if (frameNum % 60 == 0) {
            printf("FPS: %.1f\n", 1.0f / dt);
        }

        // Handle WASD movement
        WASD(camera, dt);

        // Render the scene
        draw_frame_example(camera, pixels);

        // Update the texture with the pixel buffer
        SDL_UpdateTexture(texture, nullptr, pixels.data(), SCREEN_WIDTH * sizeof(uint32_t));

        // Clear the renderer
        SDL_RenderClear(renderer);

        // Copy the texture to the renderer
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);

        // Update the screen
        SDL_RenderPresent(renderer);
        
        // Cap frame rate
        SDL_Delay(1);
    }

    // Cleanup
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}