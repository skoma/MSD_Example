// Fill out your copyright notice in the Description page of Project Settings.

#include "Generator.h"
#include "RuntimeMeshShapeGenerator.h"


int32 calc(FVector index, FVector size)
{
    int32 result = (size.Y * size.X * index.Z) + (size.X * index.Y) + index.X;
    return result;
};


TArray<int32> get_neighbours(FVector index, FVector size)
{
    TArray<int32> result;
    
    if (index.X > 0) {
        result.Add(calc(index + FVector(-1, 0, 0), size));
    }
    if (index.X < size.X - 1) {
        result.Add(calc(index + FVector(1, 0, 0), size));
    }
    if (index.Y > 0) {
        result.Add(calc(index + FVector(0, -1, 0), size));
    }
    if (index.Y < size.Y - 1) {
        result.Add(calc(index + FVector(0, 1, 0), size));
    }
    if (index.Z > 0) {
        result.Add(calc(index + FVector(0, 0, -1), size));
    }
    if (index.Z < size.Z - 1) {
        result.Add(calc(index + FVector(0, 0, 1), size));
    }
    
    return result;
}

void generateMesh(Mesh_Section & mesh_section, FVector dim, float grid_size, float mass, float k, float damping, FRuntimeMeshAccessor& MeshBuilder)
{
    FVector steps = (dim / grid_size);
    FVector grid_steps = (dim / steps);
    FVector half = dim / 2;
    mesh_section.reset();
    
    FVector size = steps + 1;
    mesh_section.size = size;
    int32 mass_point_count = (int32)(size.X * size.Y * size.Z);
    int vert_count = size.X * size.Y * 2 + size.X * size.Z * 2 + size.Y * size.Z * 2;
    int tris_count = (int)(vert_count * 3 / 2);
    
    float dps = steps.X * steps.Y * steps.Z;
    float tris = mass_point_count * 3;
    
    mesh_section.points.SetNum(mass_point_count);
	
    FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
    {
        MeshBuilder.AddIndex(Index);
    };
    
    
    MeshBuilder.EmptyVertices(4);
	MeshBuilder.EmptyIndices(4);
    auto VerticesBuilder = [&](FVector index,
                               const FVector& p0,
                               const FVector& s1,
                               const FVector& s2,
                               const FVector& s3,
                               const FVector& size,
                               const FVector& Normal,
                               const FRuntimeMeshTangent& Tangent,
                               Mass_Point * point)
	{
        FVector i1 = index + s1;
        FVector i2 = index + s2;
        FVector i3 = index + s3;
        
        FVector p1 = p0 + s1 * grid_size;
        FVector p2 = p0 + s2 * grid_size;
        FVector p3 = p0 + s3 * grid_size;
        
        int32 idx = MeshBuilder.AddVertex(p0);
        MeshBuilder.SetNormalTangent(idx, Normal, Tangent);
		MeshBuilder.SetUV(idx, FVector2D(0.0f, 0.0f));
        
        int32 idx1 = MeshBuilder.AddVertex(p1);
        MeshBuilder.SetNormalTangent(idx1, Normal, Tangent);
		MeshBuilder.SetUV(idx1, FVector2D(0.0f, 1.0f));
        
        int32 idx2 = MeshBuilder.AddVertex(p2);
        MeshBuilder.SetNormalTangent(idx2, Normal, Tangent);
		MeshBuilder.SetUV(idx2, FVector2D(1.0f, 1.0f));
        
        int32 idx3 = MeshBuilder.AddVertex(p3);
        MeshBuilder.SetNormalTangent(idx3, Normal, Tangent);
		MeshBuilder.SetUV(idx3, FVector2D(1.0f, 0.0f));
        
		URuntimeMeshShapeGenerator::ConvertQuadToTriangles(TrianglesBuilder, idx, idx1, idx2, idx3);
        
        int32 m0 = calc(index, size);
        point->indices.Add(idx);
        
        int32 m1 = calc(i1, size);
        if (mesh_section.points.Num() > m1)
        {
            mesh_section.points[m1].indices.Add(idx1);
        }
        
        int32 m2 = calc(i2, size);
        if (mesh_section.points.Num() > m2)
        {
            mesh_section.points[m2].indices.Add(idx2);
        }
        
        int32 m3 = calc(i3, size);
        if (mesh_section.points.Num() > m3)
        {
            mesh_section.points[m3].indices.Add(idx3);
        }
    };
    
    
    UE_LOG(LogTemp, Warning,
           TEXT("---------- generate mesh ------\n"
                " dim: %s\n"
                " grid_size: %f\n"
                " grid_steps: %s\n"
                " steps: %s\n"
                " size: %s\n"
                " mass points: %d\n"
                " verts: %d\n"
                " tris: %d\n"
                "--------------\n"),
           *dim.ToString(), grid_size, *grid_steps.ToString(), *steps.ToString(), *size.ToString(),
           mass_point_count, vert_count, tris_count);
    
    FVector i(0, 0, 0);
    FVector Normal;
	FRuntimeMeshTangent Tangent;
    
    for (i.Z = 0; i.Z < size.Z; i.Z += 1)
    {
        for (i.Y = 0; i.Y < size.Y; i.Y += 1)
        {
            for (i.X = 0; i.X < size.X; i.X += 1)
            {
                int idx = calc(i, size);
                Mass_Point * point = &mesh_section.points[idx];
                point->mass = mass;
                point->k = k;
                point->damping = damping;
                
                point->index = i;
                point->fix = false;
                point->vel = FVector(0, 0, 0);
                
                point->neighbours = get_neighbours(i, size);
                
                Cube_Side side = CubeSide_None;
                point->pos = FVector(i * grid_size) - half;
                FVector vp0 = point->pos;
                
                if (i.X < (size.X - 1) && i.Y < (size.Y - 1) && i.Z == 0)
                {
                    point->side |= CubeSide_Bottom;
                    // -Z
                    Normal = FVector(0.0f, 0.0f, -1.0f);
                    Tangent.TangentX = FVector(0.0f, 1.0f, 0.0f);
                    
                    FVector vp1 = FVector(1, 0, 0);
                    FVector vp2 = FVector(1, 1, 0);
                    FVector vp3 = FVector(0, 1, 0);
                    
                    VerticesBuilder(i, vp0, vp1, vp2, vp3, size, Normal, Tangent, point);
                }
                
                if (i.X < (size.X - 1) && i.Y < (size.Y - 1) && i.Z == (size.Z - 1))
                {
                    point->side |= CubeSide_Top;
                    point->fix = true;
                    // +Z
                    Normal = FVector(0.0f, 0.0f, 1.0f);
                    Tangent.TangentX = FVector(0.0f, -1.0f, 0.0f);
                    
                    FVector vp1 = FVector(0, 1, 0);
                    FVector vp2 = FVector(1, 1, 0);
                    FVector vp3 = FVector(1, 0, 0);
                    
                    VerticesBuilder(i, vp0, vp1, vp2, vp3, size, Normal, Tangent, point);
                }
                
                if (i.X < (size.X - 1) && i.Y == 0 && i.Z < (size.Z - 1))
                {
                    point->side |= CubeSide_Left;
                    // -Y
                    Normal = FVector(0.0f, -1.0f, 0.0f);
                    Tangent.TangentX = FVector(1.0f, 0.0f, 0.0f);
                    
                    FVector vp1 = FVector(0, 0, 1);
                    FVector vp2 = FVector(1, 0, 1);
                    FVector vp3 = FVector(1, 0, 0);
                    
                    VerticesBuilder(i, vp0, vp1, vp2, vp3, size, Normal, Tangent, point);
                }
                
                if (i.X < (size.X - 1) && i.Y == (size.Y - 1) && i.Z < (size.Z - 1))
                {
                    point->side |= CubeSide_Right;
                    // +Y
                    Normal = FVector(0.0f, 1.0f, 0.0f);
                    Tangent.TangentX = FVector(-1.0f, 0.0f, 0.0f);
                    
                    FVector vp1 = FVector(1, 0, 0);
                    FVector vp2 = FVector(1, 0, 1);
                    FVector vp3 = FVector(0, 0, 1);
                    
                    VerticesBuilder(i, vp0, vp1, vp2, vp3, size, Normal, Tangent, point);
                }
                
                if (i.X == 0 && i.Y < (size.Y - 1) && i.Z < (size.Z - 1))
                {
                    point->side |= CubeSide_Front; // 1
                    // -X
                    Normal = FVector(-1.0f, 0.0f, 0.0f);
                    Tangent.TangentX = FVector(0.0f, -1.0f, 0.0f);
                    
                    FVector vp1 = FVector(0, 1, 0);
                    FVector vp2 = FVector(0, 1, 1);
                    FVector vp3 = FVector(0, 0, 1);
                    
                    VerticesBuilder(i, vp0, vp1, vp2, vp3, size, Normal, Tangent, point);
                }
                
                if (i.X == (size.X - 1) && i.Y < (size.Y - 1) && i.Z < (size.Z - 1))
                {
                    point->side |= CubeSide_Back;
                    // +X
                    Normal = FVector(1.0f, 0.0f, 0.0f);
                    Tangent.TangentX = FVector(0.0f, 1.0f, 0.0f);
                    
                    FVector vp1 = FVector(0, 0, 1);
                    FVector vp2 = FVector(0, 1, 1);
                    FVector vp3 = FVector(0, 1, 0);
                    
                    VerticesBuilder(i, vp0, vp1, vp2, vp3, size, Normal, Tangent, point);
                }
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT(">>> verts: %d, idxs: %d, mass points: %d"), MeshBuilder.NumVertices(), MeshBuilder.NumIndices(), mesh_section.points.Num());
    UE_LOG(LogTemp, Warning, TEXT(">>> masspoints: %d"), mesh_section.points.Num());
}


