#include "perlin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Perlin Noise Implementation
#include <time.h>

static int permutation[512];
static int p[256] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

// Function to generate a random seed
unsigned int generateRandomSeed() {
    return (unsigned int)time(NULL);
}

// Function to shuffle the permutation array based on a seed
void initPerlinNoise(unsigned int seed) {
    // Use the seed to initialize random number generator
    srand(seed);
    
    // Create a copy of the original p array
    int tempP[256];
    for (int i = 0; i < 256; i++) {
        tempP[i] = p[i];
    }
    
    // Fisher-Yates shuffle
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = tempP[i];
        tempP[i] = tempP[j];
        tempP[j] = temp;
    }
    
    // Populate permutation array
    for (int i = 0; i < 256; i++) {
        permutation[i] = permutation[256 + i] = tempP[i];
    }
}

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float grad(int hash, float x, float y) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlinNoise(float x, float y) {
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    
    x -= floor(x);
    y -= floor(y);
    
    float u = fade(x);
    float v = fade(y);
    
    int A = permutation[X] + Y;
    int AA = permutation[A];
    int AB = permutation[A + 1];
    int B = permutation[X + 1] + Y;
    int BA = permutation[B];
    int BB = permutation[B + 1];
    
    return lerp(v, lerp(u, grad(permutation[AA], x, y),
                           grad(permutation[BA], x - 1, y)),
                   lerp(u, grad(permutation[AB], x, y - 1),
                           grad(permutation[BB], x - 1, y - 1)));
}

void generatePerlinTerrain(ObjModel *model, TerrainParams params) {
    // Initialize Perlin noise with a random seed
    unsigned int seed = generateRandomSeed();
    initPerlinNoise(seed);
    
    // Clear existing model data
    freeObjModel(model);
    
    // Initialize model
    model->vertices = NULL;
    model->texCoords = NULL;
    model->faces = NULL;
    model->normals = NULL;
    model->materials = NULL;
    model->textures = NULL;
    
    model->vertexCount = 0;
    model->texCoordCount = 0;
    model->faceCount = 0;
    model->normalCount = 0;
    model->materialCount = 0;
    model->textureCount = 0;
    
    // Generate vertices
    int totalVertices = (params.width + 1) * (params.height + 1);
    model->vertices = (Vertex*)malloc(totalVertices * sizeof(Vertex));
    model->texCoords = (TexCoord*)malloc(totalVertices * sizeof(TexCoord));
    
    if (!model->vertices || !model->texCoords) {
        printf("Erro ao alocar memória para terreno Perlin.\n");
        return;
    }
    
    model->vertexCount = totalVertices;
    model->texCoordCount = totalVertices;
    
    // Generate height map using Perlin noise
    for (int z = 0; z <= params.height; z++) {
        for (int x = 0; x <= params.width; x++) {
            int index = z * (params.width + 1) + x;
            
            float height = 0.0f;
            float amplitude = 1.0f;
            float frequency = params.scale;
            
            // Apply multiple octaves for more realistic terrain
            for (int octave = 0; octave < params.octaves; octave++) {
                height += perlinNoise(x * frequency, z * frequency) * amplitude;
                amplitude *= params.persistence;
                frequency *= params.lacunarity;
            }
            
            // Amplify the height for more dramatic peaks and valleys
            height = height * 3.0f;  // Multiply by 3 for more dramatic terrain
            
            // Add some additional variation for sharper peaks
            float sharpness = perlinNoise(x * 0.02f, z * 0.02f) * 2.0f;
            height += sharpness;
            
            // Set vertex position - scale to fit well with your (100,100,100) scale
            model->vertices[index].x = ((float)x - params.width / 2.0f) * 0.02f;  // Scale down X
            model->vertices[index].y = height * 0.01f;                            // Scale down Y but keep variation
            model->vertices[index].z = ((float)z - params.height / 2.0f) * 0.02f; // Scale down Z
            
            // Set texture coordinates
            model->texCoords[index].u = (float)x / params.width;
            model->texCoords[index].v = (float)z / params.height;
        }
    }
    
    // Generate faces (triangles)
    int totalFaces = params.width * params.height * 2;
    model->faces = (Face*)malloc(totalFaces * sizeof(Face));
    
    if (!model->faces) {
        printf("Erro ao alocar memória para faces do terreno.\n");
        return;
    }
    
    model->faceCount = totalFaces;
    
    int faceIndex = 0;
    for (int z = 0; z < params.height; z++) {
        for (int x = 0; x < params.width; x++) {
            int topLeft = z * (params.width + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (params.width + 1) + x;
            int bottomRight = bottomLeft + 1;
            
            // First triangle (top-left, bottom-left, top-right)
            model->faces[faceIndex].v1 = topLeft + 1;     // OBJ indices start at 1
            model->faces[faceIndex].v2 = bottomLeft + 1;
            model->faces[faceIndex].v3 = topRight + 1;
            model->faces[faceIndex].t1 = topLeft + 1;
            model->faces[faceIndex].t2 = bottomLeft + 1;
            model->faces[faceIndex].t3 = topRight + 1;
            model->faces[faceIndex].n1 = 0; // No normals for now
            model->faces[faceIndex].n2 = 0;
            model->faces[faceIndex].n3 = 0;
            strcpy(model->faces[faceIndex].material, "terrain");
            faceIndex++;
            
            // Second triangle (top-right, bottom-left, bottom-right)
            model->faces[faceIndex].v1 = topRight + 1;
            model->faces[faceIndex].v2 = bottomLeft + 1;
            model->faces[faceIndex].v3 = bottomRight + 1;
            model->faces[faceIndex].t1 = topRight + 1;
            model->faces[faceIndex].t2 = bottomLeft + 1;
            model->faces[faceIndex].t3 = bottomRight + 1;
            model->faces[faceIndex].n1 = 0;
            model->faces[faceIndex].n2 = 0;
            model->faces[faceIndex].n3 = 0;
            strcpy(model->faces[faceIndex].material, "terrain");
            faceIndex++;
        }
    }
    
    // Create a simple material for the terrain
    model->materialCount = 1;
    model->materials = (Material*)malloc(sizeof(Material));
    
    if (model->materials) {
        strcpy(model->materials[0].name, "terrain");
        model->materials[0].textureID = 0;
        
        // Set material properties (green-brown terrain)
        model->materials[0].Ka[0] = 0.2f; model->materials[0].Ka[1] = 0.3f; model->materials[0].Ka[2] = 0.1f;
        model->materials[0].Kd[0] = 0.4f; model->materials[0].Kd[1] = 0.6f; model->materials[0].Kd[2] = 0.2f;
        model->materials[0].Ks[0] = 0.1f; model->materials[0].Ks[1] = 0.1f; model->materials[0].Ks[2] = 0.1f;
        model->materials[0].Ke[0] = 0.0f; model->materials[0].Ke[1] = 0.0f; model->materials[0].Ke[2] = 0.0f;
        model->materials[0].Ns = 10.0f;
        model->materials[0].d = 1.0f;
        model->materials[0].illum = 2;
    }

    
    for (int i = 0; i < totalVertices; i++) {
        model->vertices[i].r = 1.0f;
        model->vertices[i].g = 1.0f;
        model->vertices[i].b = 1.0f;
    } 

    calculateTerrainColors(model);

    // Generate bounding box
    geraBox(model);
    buildAdjacency(model);
    
    printf("Terreno Perlin gerado: %d vértices, %d faces\n", model->vertexCount, model->faceCount);
}

void generateMountainousTerrain(ObjModel *model, TerrainParams params) {
    // Initialize Perlin noise with a random seed
    unsigned int seed = generateRandomSeed();
    initPerlinNoise(seed);
    
    // Clear existing model data
    freeObjModel(model);
    
    // Initialize model
    model->vertices = NULL;
    model->texCoords = NULL;
    model->faces = NULL;
    model->normals = NULL;
    model->materials = NULL;
    model->textures = NULL;
    
    model->vertexCount = 0;
    model->texCoordCount = 0;
    model->faceCount = 0;
    model->normalCount = 0;
    model->materialCount = 0;
    model->textureCount = 0;
    
    // Generate vertices
    int totalVertices = (params.width + 1) * (params.height + 1);
    model->vertices = (Vertex*)malloc(totalVertices * sizeof(Vertex));
    model->texCoords = (TexCoord*)malloc(totalVertices * sizeof(TexCoord));
    
    if (!model->vertices || !model->texCoords) {
        printf("Erro ao alocar memória para terreno montanhoso.\n");
        return;
    }
    
    model->vertexCount = totalVertices;
    model->texCoordCount = totalVertices;
    
    // Generate height map using multiple Perlin noise layers for Minecraft-like terrain
    for (int z = 0; z <= params.height; z++) {
        for (int x = 0; x <= params.width; x++) {
            int index = z * (params.width + 1) + x;
            
            float height = 0.0f;
            
            // Large-scale terrain base (continental/regional features)
            float continentalNoise = perlinNoise(x * 0.01f, z * 0.01f) * 10.0f;
            
            // Mountain ranges and ridges
            float mountainRidges = perlinNoise(x * 0.03f, z * 0.03f) * 5.0f;
            
            // Medium-scale terrain features
            float mediumFeatures = perlinNoise(x * 0.05f, z * 0.05f) * 3.0f;
            
            // Small-scale details and roughness
            float smallDetails = perlinNoise(x * 0.1f, z * 0.1f) * 1.5f;
            
            // Ultra-fine details for rocky surfaces
            float ultraDetails = perlinNoise(x * 0.3f, z * 0.3f) * 0.5f;
            
            // Combine noise layers with exponential falloff
            height = continentalNoise;
            
            // Add mountain ranges with exponential influence
            if (mountainRidges > 0) {
                height += pow(mountainRidges, 2) * 10.0f;
            }
            
            // Add medium and small features
            height += mediumFeatures * 0.5f + smallDetails * 0.25f + ultraDetails * 0.125f;
            
            // Dramatic elevation changes
            float elevationVariation = perlinNoise(x * 0.02f, z * 0.02f);
            
            // Create extreme peaks and deep valleys
            if (elevationVariation > 0.7f) {
                height += pow(elevationVariation - 0.7f, 3) * 100.0f; // Extreme high peaks
            }
            if (elevationVariation < 0.3f) {
                height -= pow(0.3f - elevationVariation, 3) * 50.0f; // Deep valleys
            }
            
            // Add randomness to break uniform patterns
            float randomness = perlinNoise(x * 0.5f, z * 0.5f) * 0.5f;
            height += randomness;
            
            // Set vertex position - scale to work well with your (100,100,100) scale
            model->vertices[index].x = ((float)x - params.width / 2.0f) * 0.02f;
            model->vertices[index].y = height * 0.01f; // Adjusted scaling for more dramatic terrain
            model->vertices[index].z = ((float)z - params.height / 2.0f) * 0.02f;
            
            // Set texture coordinates
            model->texCoords[index].u = (float)x / params.width;
            model->texCoords[index].v = (float)z / params.height;
        }
    }
    
    // Generate faces (triangles) - same as before
    int totalFaces = params.width * params.height * 2;
    model->faces = (Face*)malloc(totalFaces * sizeof(Face));
    
    if (!model->faces) {
        printf("Erro ao alocar memória para faces do terreno montanhoso.\n");
        return;
    }
    
    model->faceCount = totalFaces;
    
    int faceIndex = 0;
    for (int z = 0; z < params.height; z++) {
        for (int x = 0; x < params.width; x++) {
            int topLeft = z * (params.width + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (params.width + 1) + x;
            int bottomRight = bottomLeft + 1;
            
            // First triangle
            model->faces[faceIndex].v1 = topLeft + 1;
            model->faces[faceIndex].v2 = bottomLeft + 1;
            model->faces[faceIndex].v3 = topRight + 1;
            model->faces[faceIndex].t1 = topLeft + 1;
            model->faces[faceIndex].t2 = bottomLeft + 1;
            model->faces[faceIndex].t3 = topRight + 1;
            model->faces[faceIndex].n1 = 0;
            model->faces[faceIndex].n2 = 0;
            model->faces[faceIndex].n3 = 0;
            strcpy(model->faces[faceIndex].material, "mountain");
            faceIndex++;
            
            // Second triangle
            model->faces[faceIndex].v1 = topRight + 1;
            model->faces[faceIndex].v2 = bottomLeft + 1;
            model->faces[faceIndex].v3 = bottomRight + 1;
            model->faces[faceIndex].t1 = topRight + 1;
            model->faces[faceIndex].t2 = bottomLeft + 1;
            model->faces[faceIndex].t3 = bottomRight + 1;
            model->faces[faceIndex].n1 = 0;
            model->faces[faceIndex].n2 = 0;
            model->faces[faceIndex].n3 = 0;
            strcpy(model->faces[faceIndex].material, "mountain");
            faceIndex++;
        }
    }
    
    // Create a mountain material (rocky brown-gray)
    model->materialCount = 1;
    model->materials = (Material*)malloc(sizeof(Material));
    
    if (model->materials) {
        strcpy(model->materials[0].name, "mountain");
        model->materials[0].textureID = 0;
        
        // Set material properties (rocky mountain colors)
        model->materials[0].Ka[0] = 0.3f; model->materials[0].Ka[1] = 0.25f; model->materials[0].Ka[2] = 0.2f;
        model->materials[0].Kd[0] = 0.6f; model->materials[0].Kd[1] = 0.5f;  model->materials[0].Kd[2] = 0.4f;
        model->materials[0].Ks[0] = 0.2f; model->materials[0].Ks[1] = 0.2f;  model->materials[0].Ks[2] = 0.2f;
        model->materials[0].Ke[0] = 0.0f; model->materials[0].Ke[1] = 0.0f;  model->materials[0].Ke[2] = 0.0f;
        model->materials[0].Ns = 20.0f;
        model->materials[0].d = 1.0f;
        model->materials[0].illum = 2;
    }

    for (int i = 0; i < totalVertices; i++) {
        model->vertices[i].r = 1.0f;
        model->vertices[i].g = 1.0f;
        model->vertices[i].b = 1.0f;
    }

    calculateTerrainColors(model); 

    // Generate bounding box
    geraBox(model);
    buildAdjacency(model); 
    printf("Terreno montanhoso gerado: %d vértices, %d faces\n", model->vertexCount, model->faceCount);
}