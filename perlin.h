#ifndef PERLIN_H
#define PERLIN_H

#include "mybib.h"

// Perlin Noise functions
typedef struct {
    int width, height;
    float scale;
    int octaves;
    float persistence;
    float lacunarity;
} TerrainParams;

// Core Perlin Noise functions
float perlinNoise(float x, float y);
float fade(float t);
float lerp(float t, float a, float b);
float grad(int hash, float x, float y);
void initPerlinNoise(unsigned int seed);

// Terrain generation functions
void generatePerlinTerrain(ObjModel *model, TerrainParams params);
void generateMountainousTerrain(ObjModel *model, TerrainParams params);

#endif // PERLIN_H