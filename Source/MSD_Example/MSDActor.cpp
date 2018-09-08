#include "MSDActor.h"

#include "DrawDebugHelpers.h"

#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodySetup.h"
#include "AssetRegistryModule.h"
#include "RawMesh.h"

#include <math.h>

#include "RuntimeMeshComponent.h"
#include "RuntimeMeshShapeGenerator.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshLibrary.h"
#include "RuntimeMeshData.h"
#include "RuntimeMesh.h"

#include "Generator.cpp"


AMSDActor::AMSDActor(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
 ,bRunGenerateMeshesOnConstruction(true)
, bRunGenerateMeshesOnBeginPlay(true)
{
    PrimaryActorTick.bCanEverTick = true;
    dimension = FVector(20, 20, 20);
    grid_size = 5;
    mass = 20.0f;
    k = 50.0f;
    damping = 10.0f;
    frame_counter = 0;
    dt = 0;
    
    
    Root = CreateDefaultSubobject<USceneComponent>("RootComponent");
    //Root->RegisterComponent();
    //RootComponent = Root;
    SetRootComponent(Root);
    
    
    RuntimeMesh = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("MSD Mesh"));
	RuntimeMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
	RuntimeMesh->SetGenerateOverlapEvents(false);
#else
	RuntimeMesh->bGenerateOverlapEvents = false;
#endif
    RuntimeMesh->SetCollisionUseComplexAsSimple(false);
    RuntimeMesh->SetMeshSectionCollisionEnabled(0, false);
    
    //RuntimeMesh->SetSimulatePhysics(true);
 }

void AMSDActor::BeginPlay()
{
    Super::BeginPlay();
    
    RuntimeMesh->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
    bool bIsGameWorld = GetWorld() && GetWorld()->IsGameWorld() && !GetWorld()->IsPreviewWorld() && !GetWorld()->IsEditorWorld();
    
	bool bHadSerializedMeshData = false;
	if (RuntimeMesh)
	{
		URuntimeMesh* Mesh = RuntimeMesh->GetRuntimeMesh();
		if (Mesh)
		{
			bHadSerializedMeshData = Mesh->ShouldSerializeMeshData();
		}
	}
    
	if ((bIsGameWorld && !bHadSerializedMeshData) || bRunGenerateMeshesOnBeginPlay)
	{
    	GenerateMeshes();
	}
}

void AMSDActor::OnConstruction(const FTransform& Transform)
{
    if (bRunGenerateMeshesOnConstruction)
    {
        GenerateMeshes();
    }
}

#define debugTime 10.0f
void AMSDActor::GenerateMeshes_Implementation()
{
    if (dimension.X <= 0 || dimension.Y <= 0 || dimension.Z <= 0 || grid_size <= 0)
    {
        if (GEngine) {
            GEngine->AddOnScreenDebugMessage(-1, DEBUG_TIME,  FColor(255, 0, 0, 255), FString::Printf(TEXT("invalid dimension or grid_size")));
        }
        dimension = lastDimension;
        return;
    }
    
    FVector steps = (dimension / grid_size);
    if ( (steps.X - (int)(steps.X)) != 0.0f || (steps.Y - (int)(steps.Y)) != 0.0f || (steps.Z - (int)(steps.Z)) != 0.0f )
    {
        if (GEngine) {
            GEngine->AddOnScreenDebugMessage(-1, DEBUG_TIME,  FColor(255, 0, 0, 255), FString::Printf(TEXT("dimension must be a multiple of the grid size")));
        }
        dimension = lastDimension;
        return;
    }

    
    FRuntimeMeshDataPtr Data = RuntimeMesh->GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
    Data->CreateMeshSection(0, false, false, 1, false, true, EUpdateFrequency::Average);
    
    auto Section = Data->BeginSectionUpdate(0);
    generateMesh(mesh_section, dimension, grid_size, mass, k, damping, *Section.Get());
    UE_LOG(LogTemp, Warning, TEXT("genereted verts: %d, tris: %d, mass points: %d"), Section->NumVertices(), Section->NumIndices(), mesh_section.points.Num());
    Section->Commit();
    
    lastDimension = dimension;
}

void AMSDActor::OnOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
}

#define DEBUG_DRAW_FORCE_NET 0

void AMSDActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    ++frame_counter;
    if (frame_counter < 3 || !mesh_section.points.Num())
    {
        return;
    }
    
    DeltaTime = (frame_counter + 1) * DeltaTime;
    frame_counter = 0;
    FVector g = FVector(0, 0, -9.8);
    
    FRuntimeMeshDataPtr Data = RuntimeMesh->GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
    auto Section = Data->BeginSectionUpdate(0);
    
    //int anchorIdx = 0;
    FVector center = (mesh_section.size - 1)  / 2;
    int centerIdx = calc(center, mesh_section.size);
    int anchorIdx = (int)(mesh_section.points.Num() / 2);
    
    for (int idx = 0; idx < mesh_section.points.Num(); ++idx)
    {
        Mass_Point *point = &mesh_section.points[idx];
        {
            FVector force = FVector(0, 0, 0); //g;
            
            for (int32 i = 0; i < point->neighbours.Num(); ++i)
            {
                int32 ni = point->neighbours[i];
                if (mesh_section.points.Num() > ni)
                {
                    Mass_Point *mp = &mesh_section.points[ni];
                    
                    FVector offset = (point->index - mp->index) * grid_size;
                    FVector anchor = mp->pos + offset;
                    FVector dist = point->pos - anchor;
                    FVector dist2 = anchor - point->pos;
                    
                    FVector spring_force = -k * dist;
                    FVector damping_force = damping * point->vel;
        
                    force += spring_force - damping_force;
                }
            }
            
            point->pos = point->pos + (point->vel * DeltaTime);
            point->vel = point->vel + ((force / mass) * DeltaTime);
                
#if DEBUG_DRAW_FORCE_NET
            FVector new_pos = GetTransform().Rotator().RotateVector(point->pos);
            DrawDebugSphere(GetWorld(), GetActorLocation() + new_pos, 0.4, 6, FColor(100, 100, 255, 100), false, DeltaTime);
            DrawDebugString(GetWorld(), GetActorLocation() + new_pos + FVector(0.0f, -1.0f, -0.0f),
                            *FString::Printf(TEXT("%d"), idx), NULL, FColor(255, 0, 0, 255), DeltaTime, true);
            #endif
            
            for (int32 i = 0; i < point->indices.Num(); ++i)
                {
                    int32 ii = point->indices[i];
                    //FVector pos = Section->GetPosition(ii);
                    //pos.Z -= DeltaTime;
                    Section->SetPosition(ii, point->pos);
                }
            #if 0
        } else {
#if DEBUG_DRAW_FORCE_NET
            FVector new_pos = GetTransform().Rotator().RotateVector(point->pos);
            DrawDebugSphere(GetWorld(), GetActorLocation() + new_pos, 0.4, 6, FColor(255, 100, 100, 100), false, DeltaTime);
#endif
            
#endif
        }
    }
    
    Section->Commit();
}

void AMSDActor::update_grab(FVector location)
{
    FRotator revRot = GetTransform().Rotator().GetInverse();
    FVector relative_pos = revRot.RotateVector(location - GetActorLocation());
    
    for (int32 idx : grabbed_points)
    {
        Mass_Point * point = &mesh_section.points[idx];
        FVector dist = point->pos - relative_pos;
        //if (dist.Size() > 1)
        {
            //FVector grab_force = -10 * dist;
            point->vel = point->vel - dist;
        }
    }
}

void AMSDActor::grab_location(FVector location)
{
    grabbed_points = get_mass_points(location, (grid_size / 2) + 0.01);
}

void AMSDActor::release_grab()
{
    grabbed_points.Reset();
}

TArray<int32> AMSDActor::get_mass_points(FVector pos, int32 dist)
{
    TArray<int32> result;
    FRotator revRot = GetTransform().Rotator().GetInverse();
    FVector relative_pos = revRot.RotateVector(pos - GetActorLocation());
    
    for (int idx = 0; idx < mesh_section.points.Num(); ++idx)
    {
        Mass_Point point = mesh_section.points[idx];
        FVector diff = relative_pos - point.pos;
        if (diff.Size() < dist)
        {
            result.Add(idx);
        }
    }
    
    return result;
}


#define DEBUG_DRAW_IMPACAT_POINT 0
#define DEBUG_DRAW_IMPACAT_POINT_ROTATED 0
#define DEBUG_DRAW_HIT_VERTICIES 0

void AMSDActor::applyForce(AActor * other, FVector force, FHitResult hit)
{
    FTransform trans = GetTransform();
    FRotator rot = trans.Rotator();
    FRotator revRot = rot.GetInverse();
    
    FVector newForce = revRot.RotateVector(-force);
    
    
#if DEBUG_DRAW_IMPACAT_POINT
    DrawDebugSphere(GetWorld(), hit.ImpactPoint, 1, 6, FColor(255, 255, 0, 100), false, debugTime);
    DrawDebugLine(GetWorld(), hit.Location, hit.Location - (0.02f * force), FColor(255, 0, 0), false, debugTime, 0, 0);
    UE_LOG(LogTemp, Log, TEXT("hit at  %s,  force: %s"), *hit.Location.ToCompactString(), *force.ToCompactString()));
#endif
    
    
#if DEBUG_DRAW_IMPACAT_POINT_ROTATED
    FVector newHit = revRot.RotateVector(hit.ImpactPoint - GetActorLocation());
    DrawDebugSphere(GetWorld(), newHit, 0.5, 6, FColor(255, 0, 255, 100), false, debugTime);
    DrawDebugLine(GetWorld(), newHit, newHit + (.002f * newForce), FColor(200, 0, 255), false, debugTime, 0, 0);
    UE_LOG(LogTemp, Log, TEXT("rotate at %s, force %s"), *newHit.ToCompactString(), *newForce.ToCompactString()));
#endif

    TArray<int32> hits = get_mass_points(hit.ImpactPoint, (grid_size * 2) + 0.01);
    if (hits.Num())
    {
        for (int32 hit_index : hits)
        {
            mesh_section.points[hit_index].vel += newForce;
        }
    }
}
