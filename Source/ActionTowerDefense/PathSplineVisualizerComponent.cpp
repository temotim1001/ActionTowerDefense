// Fill out your copyright notice in the Description page of Project Settings.


#include "PathSplineVisualizerComponent.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

UPathSplineVisualizerComponent::UPathSplineVisualizerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    PathMesh = nullptr;
    PathMaterial = nullptr;
    PathWidth = 100.f;   // default width
    bShowPath = true;
}

void UPathSplineVisualizerComponent::OnRegister()
{
    Super::OnRegister();

    ClearSplineMeshes();

    if (!bShowPath || !PathMesh || !PathMaterial)
    {
        return;
    }

    if (AActor* Owner = GetOwner())
    {
        CachedSpline = Owner->FindComponentByClass<USplineComponent>();
    }

    if (CachedSpline.IsValid())
    {
        BuildSplineMeshes();
    }
}

void UPathSplineVisualizerComponent::OnUnregister()
{
    ClearSplineMeshes();
    Super::OnUnregister();
}

void UPathSplineVisualizerComponent::ClearSplineMeshes()
{
    for (USplineMeshComponent* Comp : SplineMeshComponents)
    {
        if (Comp)
        {
            Comp->DestroyComponent();
        }
    }
    SplineMeshComponents.Empty();
}

void UPathSplineVisualizerComponent::BuildSplineMeshes()
{
    USplineComponent* SplineComp = CachedSpline.Get();
    if (!SplineComp || !PathMesh || !PathMaterial) return;

    const float SplineLength = SplineComp->GetSplineLength();
    const float SegmentLength = 100.f; // adjust for smoothness

    // Use the actual mesh width so PathWidth is in UU
    const float MeshNativeWidth =
        PathMesh->GetBounds().BoxExtent.Y * 2.f; // full width (Y axis)

    const float WidthScale =
        (MeshNativeWidth > KINDA_SMALL_NUMBER)
        ? (PathWidth / MeshNativeWidth)
        : 1.f;

    float Distance = 0.f;

    while (Distance < SplineLength)
    {
        const float NextDistance = FMath::Min(Distance + SegmentLength, SplineLength);

        USplineMeshComponent* SplineMesh =
            NewObject<USplineMeshComponent>(GetOwner(), USplineMeshComponent::StaticClass());

        SplineMesh->SetMobility(EComponentMobility::Movable);
        SplineMesh->SetStaticMesh(PathMesh);
        SplineMesh->SetMaterial(0, PathMaterial);
        SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        SplineMesh->AttachToComponent(
            SplineComp,
            FAttachmentTransformRules::KeepRelativeTransform
        );
        SplineMesh->RegisterComponent();

        FVector StartPos = SplineComp->GetLocationAtDistanceAlongSpline(
            Distance, ESplineCoordinateSpace::Local);
        FVector StartTan = SplineComp->GetTangentAtDistanceAlongSpline(
            Distance, ESplineCoordinateSpace::Local);

        FVector EndPos = SplineComp->GetLocationAtDistanceAlongSpline(
            NextDistance, ESplineCoordinateSpace::Local);
        FVector EndTan = SplineComp->GetTangentAtDistanceAlongSpline(
            NextDistance, ESplineCoordinateSpace::Local);

        // Optional Z offset, if you want it above the floor:
        StartPos.Z += -10.f;
        EndPos.Z   += -10.f;

        SplineMesh->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan);

        // Path runs along X axis of the mesh
        SplineMesh->SetForwardAxis(ESplineMeshAxis::X, true);

        // X=length, Y=half-width → we use WidthScale so world width = PathWidth
        SplineMesh->SetStartScale(FVector2D(1.f, WidthScale));
        SplineMesh->SetEndScale(FVector2D(1.f, WidthScale));

        SplineMesh->UpdateMesh();

        SplineMeshComponents.Add(SplineMesh);

        Distance = NextDistance;
    }
}



