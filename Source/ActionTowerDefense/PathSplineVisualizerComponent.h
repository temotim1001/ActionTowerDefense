// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PathSplineVisualizerComponent.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ACTIONTOWERDEFENSE_API UPathSplineVisualizerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPathSplineVisualizerComponent();

protected:
    virtual void OnRegister() override;
    virtual void OnUnregister() override;

    // Mesh used along the path (e.g. Engine/BasicShapes/Plane)
    UPROPERTY(EditAnywhere, Category = "Path Visual")
    UStaticMesh* PathMesh;

    // Emissive material for the glowing path
    UPROPERTY(EditAnywhere, Category = "Path Visual")
    UMaterialInterface* PathMaterial;

    // Desired path width in world units (cm)
    UPROPERTY(EditAnywhere, Category = "Path Visual")
    float PathWidth;

    // Toggle path visibility
    UPROPERTY(EditAnywhere, Category = "Path Visual")
    bool bShowPath;

private:
    // The spline we visualize
    TWeakObjectPtr<USplineComponent> CachedSpline;

    // All spline mesh components we create
    UPROPERTY(Transient)
    TArray<USplineMeshComponent*> SplineMeshComponents;

    void ClearSplineMeshes();
    void BuildSplineMeshes();
};
