// Fill out your copyright notice in the Description page of Project Settings.


#include "AMinigunTower.h"
#include "Components/SceneComponent.h"
#include "Projectile.h"

// If MeshComp is declared in TowerBase, AttackTowerBase already sets it up. :contentReference[oaicite:1]{index=1}

AMinigunTower::AMinigunTower()
{
    // Create 3 muzzle points attached to the tower mesh
    MuzzlePoint1 = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint1"));
    MuzzlePoint1->SetupAttachment(MeshComp);
    MuzzlePoint1->SetRelativeLocation(FVector(100.f, -20.f, 50.f));

    MuzzlePoint2 = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint2"));
    MuzzlePoint2->SetupAttachment(MeshComp);
    MuzzlePoint2->SetRelativeLocation(FVector(100.f, 0.f, 50.f));

    MuzzlePoint3 = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint3"));
    MuzzlePoint3->SetupAttachment(MeshComp);
    MuzzlePoint3->SetRelativeLocation(FVector(100.f, 20.f, 50.f));

    // Make it feel like a minigun: faster ROF, lower per-shot damage (tweak in BP)
    FireRate = 8.0f;        // shots per second
    ProjectileDamage = 8.0f;
}

USceneComponent* AMinigunTower::GetCurrentMuzzleComponent() const
{
    switch (CurrentMuzzleIndex)
    {
    case 0: return MuzzlePoint1;
    case 1: return MuzzlePoint2;
    case 2: return MuzzlePoint3;
    default: return MuzzlePoint1;
    }
}

void AMinigunTower::FireProjectile()
{
    if (!CurrentTarget.IsValid() || !ProjectileClass)
        return;

    USceneComponent* Muzzle = GetCurrentMuzzleComponent();
    if (!Muzzle)
    {
        // Fallback to base MuzzlePoint or actor location if something is wrong
        const FVector FallbackLocation = MuzzlePoint
            ? MuzzlePoint->GetComponentLocation()
            : GetActorLocation();

        const FVector TargetLocation = CurrentTarget->GetActorLocation();
        const FRotator FallbackRotation = (TargetLocation - FallbackLocation).Rotation();

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        Params.Owner = this;
        Params.Instigator = GetInstigator();

        AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
            ProjectileClass,
            FallbackLocation,
            FallbackRotation,
            Params
        );

        if (Projectile)
        {
            Projectile->InitProjectile(
                CurrentTarget.Get(),
                ProjectileDamage,
                ProjectileSpeed,
                bUseHoming,
                ProjectileHomingAcceleration
            );

            BP_OnAttackFired();
        }

        return;
    }

    const FVector SpawnLocation = Muzzle->GetComponentLocation();
    const FVector TargetLocation = CurrentTarget->GetActorLocation();
    const FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Params.Owner = this;
    Params.Instigator = GetInstigator();

    AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
        ProjectileClass,
        SpawnLocation,
        SpawnRotation,
        Params
    );

    if (Projectile)
    {
        Projectile->InitProjectile(
            CurrentTarget.Get(),
            ProjectileDamage,
            ProjectileSpeed,
            bUseHoming,
            ProjectileHomingAcceleration
        );

        BP_OnAttackFired(); // same BP event as CannonTower
    }

    // Cycle to next muzzle: 0 → 1 → 2 → 0 → ...
    CurrentMuzzleIndex = (CurrentMuzzleIndex + 1) % 3;
}

