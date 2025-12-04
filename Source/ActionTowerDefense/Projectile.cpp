// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DamageableTarget.h"
#include "STGameController.h" 


AProjectile::AProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    // ---- Collision ----
    CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
    CollisionComp->InitSphereRadius(10.f);
    SetRootComponent(CollisionComp);

    // This projectile itself is of type ProjectileChannel
    CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel2);

    // We want real hit events (not overlaps) against enemies
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

    // No overlaps needed for this class
    CollisionComp->SetGenerateOverlapEvents(false);

    // Required for OnComponentHit with query-only collision
    CollisionComp->SetNotifyRigidBodyCollision(true);

    // ---- Visual mesh ----
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CollisionComp);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // ---- Movement ----
    MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
    MovementComp->InitialSpeed = 2000.f;
    MovementComp->MaxSpeed = 2000.f;
    MovementComp->bRotationFollowsVelocity = true;
    MovementComp->bShouldBounce = false;
    MovementComp->ProjectileGravityScale = 0.f;

    // Safety timeout
    InitialLifeSpan = 5.f;
}

void AProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Bind hit event
    CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnProjectileHit);
}

void AProjectile::InitProjectile(AActor* InTarget,
    float InDamage,
    float InSpeed,
    bool bInUseHoming,
    float InHomingAcceleration)
{
    TargetActor = InTarget;
    Damage = InDamage;

    BaseSpeed = InSpeed;

    // Apply speed from tower
    MovementComp->InitialSpeed = InSpeed;
    MovementComp->MaxSpeed = InSpeed;

    // If you want to force an initial velocity in the forward direction:
    MovementComp->Velocity = GetActorForwardVector() * InSpeed;

    // Homing setup
    if (bInUseHoming && TargetActor.IsValid())
    {
        MovementComp->bIsHomingProjectile = true;
        MovementComp->HomingAccelerationMagnitude = InHomingAcceleration;
        MovementComp->HomingTargetComponent = TargetActor->GetRootComponent();
        bWasHomingProjectile = true;
    }
    else
    {
        MovementComp->bIsHomingProjectile = false;
        MovementComp->HomingTargetComponent = nullptr;
        bWasHomingProjectile = false;
    }
}

void AProjectile::OnProjectileHit(UPrimitiveComponent* HitComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    // Ignore self and our owner (the tower)
    if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }

    // Optional debug
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 1.5f, FColor::Yellow,
            FString::Printf(TEXT("Projectile hit: %s"), *GetNameSafe(OtherActor))
        );
    }

    // Apply damage if target implements our interface
    if (OtherActor->GetClass()->ImplementsInterface(UDamageableTarget::StaticClass()))
    {
        IDamageableTarget::Execute_ReceiveTowerDamage(OtherActor, Damage);
    }

    Destroy();
}

void AProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!MovementComp)
    {
        return;
    }

    float Speed = 1.f;
    if (ASTGameController* GC = ASTGameController::Get(this))
    {
        Speed = GC->GetGameSpeed();   // -3, 1, 3, 5
    }

    if (FMath::IsNearlyZero(Speed))
    {
        // Pause projectile
        MovementComp->Velocity = FVector::ZeroVector;
        return;
    }

    const float AbsSpeed = FMath::Abs(Speed);

    if (Speed > 0.f)
    {
        // Normal / fast-forward
        MovementComp->MaxSpeed = BaseSpeed * AbsSpeed;
        MovementComp->InitialSpeed = BaseSpeed * AbsSpeed;

        FVector Dir = MovementComp->Velocity.IsNearlyZero()
            ? GetActorForwardVector()
            : MovementComp->Velocity.GetSafeNormal();

        MovementComp->Velocity = Dir * MovementComp->InitialSpeed;

        // Restore homing if we had it originally
        if (bWasHomingProjectile && TargetActor.IsValid())
        {
            MovementComp->bIsHomingProjectile = true;
            MovementComp->HomingTargetComponent = TargetActor->GetRootComponent();
        }
    }
    else // Speed < 0 => rewind
    {
        // Turn off homing while rewinding
        MovementComp->bIsHomingProjectile = false;
        MovementComp->HomingTargetComponent = nullptr;

        FVector Dir = MovementComp->Velocity.IsNearlyZero()
            ? -GetActorForwardVector()
            : -MovementComp->Velocity.GetSafeNormal();

        MovementComp->MaxSpeed = BaseSpeed * AbsSpeed;
        MovementComp->InitialSpeed = BaseSpeed * AbsSpeed;
        MovementComp->Velocity = Dir * MovementComp->InitialSpeed;
    }
}

