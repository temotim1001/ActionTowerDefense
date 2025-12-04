// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class ACTIONTOWERDEFENSE_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    AProjectile();

    // Called by tower after spawn
    void InitProjectile(AActor* InTarget,
        float InDamage,
        float InSpeed,
        bool bInUseHoming,
        float InHomingAcceleration);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;  // ← add this

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    UProjectileMovementComponent* MovementComp;

    // Combat
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    float Damage = 20.f;

    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    // Base speed from tower – used to scale with global speed
    float BaseSpeed = 2000.f;

    // Remember if this projectile was homing at spawn
    bool bWasHomingProjectile = false;

    // Hit callback
    UFUNCTION()
    void OnProjectileHit(UPrimitiveComponent* HitComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        FVector NormalImpulse,
        const FHitResult& Hit);
};
