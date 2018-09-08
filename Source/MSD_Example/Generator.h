// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshComponent.h"

#define DEBUG_TIME 20.0f

#define pi32   3.14159265359f
#define tau32  6.28318530717958647692f

enum Cube_Side
{
    CubeSide_None   = 0,
    CubeSide_Front  = 1,
    CubeSide_Back   = 2,
    CubeSide_Left   = 4,
    CubeSide_Right  = 8,
    CubeSide_Top    = 16,
    CubeSide_Bottom = 32
};


struct Mass_Point
{
    float mass;
    float k;
    float damping;
    
    FVector index;
    FVector pos;
    FVector vel;
    
    uint32 side;
    bool fix;
    
    TArray<int32> indices;
    TArray<int32> neighbours;
};

struct Mesh_Section
{
    void reset()
    {
        size = FVector(0, 0, 0);
        vertices.Reset();
        triangles.Reset();
        points.Reset();
    };
    
     FVector size;
    TArray<FVector> vertices;
    TArray<int32> triangles;
    TArray<Mass_Point> points;
};


static int32 calc(FVector index, FVector size);
static TArray<int32> get_neighbours(FVector index, FVector size);
static void generateMesh(Mesh_Section & meshSection, FVector dimen, float grid_size,float mass, float k, float damping, FRuntimeMeshAccessor& MeshBuilder);

    