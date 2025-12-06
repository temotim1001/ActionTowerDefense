// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "STEnemyLifeComponent.h"
#include "STGameSpeedHelpers.h"
#include "STGameController.h" 

ASTEnemyBase::ASTEnemyBase()
{
    PrimaryActorTick.bCanEverTick = true;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    RootComponent = MeshComp;

    LifeComponent = CreateDefaultSubobject<USTEnemyLifeComponent>(TEXT("EnemyLifeComponent"));
}

void ASTEnemyBase::BeginPlay()
{
    Super::BeginPlay();

    // Start at full health
    CurrentHealth = MaxHealth;

    if (SplineActor)
    {
        CachedSpline = SplineActor->FindComponentByClass<USplineComponent>();
        if (!CachedSpline)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("EnemyBase: SplineActor %s has no USplineComponent"),
                *SplineActor->GetName());
        }
    }
}

void ASTEnemyBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const float Speed = FSTGameSpeedHelpers::GetGameSpeed(this);

    // Pause when speed is effectively 0 (0x)
    if (FMath::IsNearlyZero(Speed))
    {
        return;
    }

    // Scale by game speed (1x, 3x, 5x, -3x, etc.)
    const float EffectiveDelta = DeltaSeconds * Speed;
    UpdateMovement(EffectiveDelta);
}

void ASTEnemyBase::SetSplineActor(AActor* InSplineActor)
{
    SplineActor = InSplineActor;
    CachedSpline = nullptr;

    UE_LOG(LogTemp, Warning, TEXT("EnemyBase: SetSplineActor called on %s, InSplineActor=%s"),
        *GetName(),
        SplineActor ? *SplineActor->GetName() : TEXT("NULL"));

    if (SplineActor)
    {
        CachedSpline = SplineActor->FindComponentByClass<USplineComponent>();
        if (!CachedSpline)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("EnemyBase::SetSplineActor: SplineActor %s has no USplineComponent"),
                *SplineActor->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("EnemyBase: CachedSpline is VALID on %s"),
                *GetName());
        }
    }
}


void ASTEnemyBase::UpdateMovement(float DeltaSeconds)
{
    if (!CachedSpline)
    {
        return;
    }

    // Move forward/backward along the spline depending on sign of DeltaSeconds
    DistanceAlongSpline += MoveSpeed * DeltaSeconds;

    const float SplineLength = CachedSpline->GetSplineLength();

    // Clamp so we never go below 0 or beyond the spline length
    DistanceAlongSpline = FMath::Clamp(DistanceAlongSpline, 0.f, SplineLength);

    // If we’ve just reached the end going forward, treat it as a leak.
    if (DistanceAlongSpline >= SplineLength - KINDA_SMALL_NUMBER)
    {
        HandleReachedGoal();
        return;
    }

    const FVector NewLocation =
        CachedSpline->GetLocationAtDistanceAlongSpline(
            DistanceAlongSpline,
            ESplineCoordinateSpace::World
        );

    SetActorLocation(NewLocation);

    if (bOrientRotationToSpline)
    {
        const FRotator NewRotation =
            CachedSpline->GetRotationAtDistanceAlongSpline(
                DistanceAlongSpline,
                ESplineCoordinateSpace::World
            );
        SetActorRotation(NewRotation);
    }
}

void ASTEnemyBase::HandleReachedGoal()
{
    BP_OnReachedGoal();
    KillEnemy(true);
}

void ASTEnemyBase::KillEnemy(bool bReachedGoal)
{
    // Notify GameController via life component
    if (LifeComponent)
    {
        LifeComponent->MarkEnemyRemoved(bReachedGoal);
    }

    if (bReachedGoal)
    {
        // leaked through to the end – no score
        BP_OnReachedGoal();
    }
    else
    {
        // killed by towers – award score
        if (ASTGameController* GC = ASTGameController::Get(this))
        {
            GC->AwardScoreForEnemy(BaseScoreValue);
        }

        BP_OnKilled();
    }

    Destroy();
}

void ASTEnemyBase::ApplyDamage(float Amount)
{
    if (Amount <= 0.f)
    {
        return;
    }

    if (CurrentHealth <= 0.f)
    {
        // Already dead
        return;
    }

    CurrentHealth -= Amount;

    // Optional debug
    UE_LOG(LogTemp, Log,
        TEXT("Enemy %s took %.1f damage, health now %.1f"),
        *GetName(), Amount, CurrentHealth);

    if (CurrentHealth <= 0.f)
    {
        // Killed by tower damage
        KillEnemy(false);
    }
}

void ASTEnemyBase::ReceiveTowerDamage_Implementation(float Amount)
{
    ApplyDamage(Amount);
}


