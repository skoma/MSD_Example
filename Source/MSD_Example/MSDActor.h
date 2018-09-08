#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshActor.h"
#include "Generator.h"
#include "MSDActor.generated.h"

UCLASS(HideCategories = (Input), ShowCategories = ("Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass, Meta = (ChildCanTick))
class MSD_EXAMPLE_API AMSDActor : public AActor
{
    GENERATED_UCLASS_BODY()
    
public:
   
    virtual void OnConstruction(const FTransform& Transform) override;
    
    virtual void Tick(float DeltaTime) override;
    
    virtual void BeginPlay() override;
    
    void OnClick(UPrimitiveComponent* pComponent);
    
    
    
    UFUNCTION(BlueprintCallable, Category = "MSD")
    void applyForce(AActor * other, FVector force, FHitResult hit);
    
    UFUNCTION(BlueprintCallable, Category = "MSD")
    void update_grab(FVector location);
    
    UFUNCTION(BlueprintCallable, Category = "MSD")
    void grab_location(FVector location);
    
    UFUNCTION(BlueprintCallable, Category = "MSD")
    void release_grab();
    
    
 
    class URuntimeMeshComponent* GetRuntimeMeshComponent() const { return RuntimeMesh; }
 
    
    
    
    UFUNCTION(BlueprintNativeEvent)
    void OnOverlap(AActor* OverlappedActor, AActor* OtherActor);
    
    UFUNCTION(BlueprintNativeEvent)
    void GenerateMeshes();
    
    
    
    UPROPERTY(Category = "MSD", EditAnywhere, Meta = (AllowPrivateAccess = "true"))
    bool bRunGenerateMeshesOnConstruction;
    
	UPROPERTY(Category = "MSD", EditAnywhere, Meta = (AllowPrivateAccess = "true"))
    bool bRunGenerateMeshesOnBeginPlay;
    
    UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "MSD")
    FVector dimension;
    FVector lastDimension;
    
    UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "MSD")
    float grid_size;
    
    UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "MSD")
    float mass;
    
    UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "MSD")
    float k;
    
    UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "MSD")
    float damping;
    
    
    
    UPROPERTY(VisibleAnywhere, BluePrintReadWrite, Category = "MSD")
    URuntimeMeshComponent* RuntimeMesh;
    
    UPROPERTY(VisibleAnywhere, BluePrintReadWrite, Category = "MSD")
    USceneComponent* Root;
    
    
    TArray<int32> get_mass_points(FVector pos, int32 dist);
    
    float dt;

private:
    Mesh_Section mesh_section;
    int frame_counter;
    TArray<int32> grabbed_points;
};
