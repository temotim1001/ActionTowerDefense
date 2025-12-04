// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackTowerBase.h"
#include "Components/SphereComponent.h"
#include "Projectile.h"
#include "DamageableTarget.h"
#include "Engine/Engine.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "STGameSpeedHelpers.h"
#include "TowerAttackComponent.h"

// ========================================================
// Constructor / BeginPlay
// ========================================================

AAttackTowerBase::AAttackTowerBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // Attack range collider
    AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangeSphere"));
    AttackRangeSphere->SetupAttachment(MeshComp);
    AttackRangeSphere->SetSphereRadius(1000.f);
    AttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AttackRangeSphere->SetGenerateOverlapEvents(true);

    AttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    AttackRangeSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);

    // Muzzle
    MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
    MuzzlePoint->SetupAttachment(MeshComp);
    MuzzlePoint->SetRelativeLocation(FVector(100.f, 0.f, 50.f));

    // Capture beam
    CaptureBeamComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CaptureBeamFX"));
    CaptureBeamComponent->SetupAttachment(MuzzlePoint);
    CaptureBeamComponent->SetAutoActivate(false);

    // Attack logic component
    AttackComponent = CreateDefaultSubobject<UTowerAttackComponent>(TEXT("AttackComponent"));
}

void AAttackTowerBase::BeginPlay()
{
    Super::BeginPlay();

    if (AttackRangeSphere)
    {
        CaptureRange = AttackRangeSphere->GetScaledSphereRadius();
    }

    AttackRangeSphere->OnComponentBeginOverlap.AddDynamic(
        this, &AAttackTowerBase::OnAttackRangeBeginOverlap);
    AttackRangeSphere->OnComponentEndOverlap.AddDynamic(
        this, &AAttackTowerBase::OnAttackRangeEndOverlap);

    if (CaptureBeamComponent && CaptureBeamSystem)
    {
        CaptureBeamComponent->SetAsset(CaptureBeamSystem);
    }

    // NEW: wire attack component and sync fire rate
    if (AttackComponent)
    {
        AttackComponent->OnTowerReadyToFire.AddDynamic(
            this,
            &AAttackTowerBase::HandleAttackReadyToFire);

        ApplyFireRateToAttackComponent();
    }
}

// ========================================================
// Tick
// ========================================================

void AAttackTowerBase::Tick(float DeltaSeconds)
{
    // Base class handles capture
    Super::Tick(DeltaSeconds);

    const float Speed = FSTGameSpeedHelpers::GetGameSpeed(this);
    if (Speed <= 0.f)
    {
        // No attacking during pause/rewind
        UpdateCaptureBeam();
        return;
    }

    const float EffectiveDelta = DeltaSeconds * Speed;
    TickAttack(EffectiveDelta);
}

void AAttackTowerBase::TickAttack(float DeltaSeconds)
{
    UpdateCaptureBeam();
    UpdateOrderState(DeltaSeconds);

    if (!AttackComponent)
    {
        return;
    }

    // Rotate towards current target
    RotateTowardsTarget(DeltaSeconds);

    const bool bIsAimed = IsAimedAtTarget();

    const bool bOrderAllowsFire =
        (CurrentOrderState != ETowerOrderState::HoldFire) &&
        (CurrentOrderState != ETowerOrderState::Disabled);

    const bool bCanFireNow = bOrderAllowsFire && bIsAimed;

    // Component does queue + cooldown + "ready to fire" event
    AttackComponent->TickAttack(DeltaSeconds, bCanFireNow);
}

// ========================================================
// Target handling: overlaps → component queue
// ========================================================

void AAttackTowerBase::OnAttackRangeBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!AttackComponent)
        return;

    if (!OtherActor || OtherActor == this)
        return;

    // Ignore towers
    if (Cast<ATowerBase>(OtherActor))
        return;

    AttackComponent->AddTarget(OtherActor);
}

void AAttackTowerBase::OnAttackRangeEndOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!AttackComponent)
        return;

    if (!OtherActor)
        return;

    AttackComponent->RemoveTarget(OtherActor);
}

// ========================================================
// Order State
// ========================================================

void AAttackTowerBase::UpdateOrderState(float DeltaSeconds)
{
    switch (CurrentOrderState)
    {
    case ETowerOrderState::AttackEnemies:
        // Normal behavior
        break;

    case ETowerOrderState::HoldFire:
        // Skip fire logic
        return;

    case ETowerOrderState::CaptureTower:
        if (IsCaptureOrderCompleted())
        {
            SetOrderState(ETowerOrderState::AttackEnemies, nullptr);
            return;
        }
        break;

    case ETowerOrderState::AttackBonus:
        // TODO: bonus logic
        break;

    case ETowerOrderState::Disabled:
        return;

    default:
        break;
    }
}

void AAttackTowerBase::SetOrderState(ETowerOrderState NewState, AActor* NewForcedTarget)
{
    CurrentOrderState = NewState;
    ForcedOrderTarget = NewForcedTarget;

    switch (CurrentOrderState)
    {
    case ETowerOrderState::AttackEnemies:
        SetCaptureTarget(nullptr);
        StopCaptureBeam();
        break;

    case ETowerOrderState::CaptureTower:
    {
        ATowerBase* TargetTower = Cast<ATowerBase>(ForcedOrderTarget);
        if (TargetTower && TargetTower->GetTeam() == ETowerTeam::Neutral)
        {
            SetCaptureTarget(TargetTower);
        }
        else
        {
            CurrentOrderState = ETowerOrderState::AttackEnemies;
            ForcedOrderTarget = nullptr;
            SetCaptureTarget(nullptr);
            StopCaptureBeam();
        }
        break;
    }

    case ETowerOrderState::AttackBonus:
        SetCaptureTarget(nullptr);
        StopCaptureBeam();
        break;

    case ETowerOrderState::HoldFire:
    case ETowerOrderState::Disabled:
        SetCaptureTarget(nullptr);
        StopCaptureBeam();
        break;

    default:
        break;
    }
}

bool AAttackTowerBase::IsCaptureOrderCompleted() const
{
    if (!AssignedCaptureTarget || !IsValid(AssignedCaptureTarget))
    {
        return true;
    }

    if (AssignedCaptureTarget->GetTeam() != ETowerTeam::Neutral)
    {
        return true;
    }

    if (!IsValidCaptureTarget(AssignedCaptureTarget))
    {
        return true;
    }

    return false;
}

// Public order API

void AAttackTowerBase::OrderCaptureTower(ATowerBase* NeutralTower)
{
    if (!NeutralTower)
        return;

    if (NeutralTower->GetTeam() != ETowerTeam::Neutral)
        return;

    SetOrderState(ETowerOrderState::CaptureTower, NeutralTower);
}

void AAttackTowerBase::OrderAttackBonus(AActor* BonusActor)
{
    if (!BonusActor)
        return;

    SetOrderState(ETowerOrderState::AttackBonus, BonusActor);
}

void AAttackTowerBase::OrderStopCurrentAction()
{
    SetOrderState(ETowerOrderState::AttackEnemies, nullptr);
}

// ========================================================
// Rotation / Aiming
// ========================================================

void AAttackTowerBase::RotateTowardsTarget(float DeltaSeconds)
{
    if (!AttackComponent)
        return;

    AActor* Target = AttackComponent->GetCurrentTarget();
    if (!Target)
        return;

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();

    if (bUseYawOnly)
    {
        ToTarget.Z = 0.f;
    }

    if (ToTarget.IsNearlyZero())
        return;

    FRotator CurrentRot = GetActorRotation();
    FRotator DesiredRot = ToTarget.Rotation();

    if (bUseYawOnly)
    {
        DesiredRot.Pitch = 0.f;
        DesiredRot.Roll = 0.f;
    }

    FRotator NewRot = FMath::RInterpConstantTo(
        CurrentRot,
        DesiredRot,
        DeltaSeconds,
        RotationSpeedDegPerSec
    );

    SetActorRotation(NewRot);
}

bool AAttackTowerBase::IsAimedAtTarget() const
{
    if (!AttackComponent)
        return false;

    AActor* Target = AttackComponent->GetCurrentTarget();
    if (!Target)
        return false;

    FVector Forward = GetActorForwardVector();
    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();

    if (bUseYawOnly)
    {
        Forward.Z = 0.f;
        ToTarget.Z = 0.f;
    }

    if (ToTarget.IsNearlyZero())
        return true;

    Forward.Normalize();
    ToTarget.Normalize();

    const float Dot = FVector::DotProduct(Forward, ToTarget);
    const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

    return AngleDeg <= AimToleranceDeg;
}

// ========================================================
// Firing hook
// ========================================================

void AAttackTowerBase::HandleAttackReadyToFire()
{
    // Only actually shoot if we *still* have a valid target
    FireProjectile();
}

void AAttackTowerBase::FireProjectile()
{
    if (!AttackComponent || !ProjectileClass)
        return;

    AActor* Target = AttackComponent->GetCurrentTarget();
    if (!Target)
        return;

    const FVector SpawnLoc = MuzzlePoint
        ? MuzzlePoint->GetComponentLocation()
        : GetActorLocation();

    const FVector TargetLoc = Target->GetActorLocation();
    const FRotator SpawnRot = (TargetLoc - SpawnLoc).Rotation();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Params.Owner = this;
    Params.Instigator = GetInstigator();

    AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
        ProjectileClass,
        SpawnLoc,
        SpawnRot,
        Params
    );

    if (Projectile)
    {
        Projectile->InitProjectile(
            Target,
            ProjectileDamage,
            ProjectileSpeed,
            bUseHoming,
            ProjectileHomingAcceleration
        );

        BP_OnAttackFired();
    }
}

// ========================================================
// Capture Beam
// ========================================================

void AAttackTowerBase::UpdateCaptureBeam()
{
    if (!CaptureBeamComponent)
        return;

    const bool bIsCapturing =
        (CurrentOrderState == ETowerOrderState::CaptureTower) &&
        AssignedCaptureTarget != nullptr &&
        IsValid(AssignedCaptureTarget);

    if (bIsCapturing)
    {
        if (!CaptureBeamComponent->IsActive())
        {
            CaptureBeamComponent->Activate();
        }

        const FVector StartPos = MuzzlePoint
            ? MuzzlePoint->GetComponentLocation()
            : GetActorLocation();

        const FVector EndPos = AssignedCaptureTarget->GetActorLocation();

        CaptureBeamComponent->SetNiagaraVariableVec3(TEXT("User.StartPosition"), StartPos);
        CaptureBeamComponent->SetNiagaraVariableVec3(TEXT("User.EndPosition"), EndPos);
    }
    else
    {
        if (CaptureBeamComponent->IsActive())
        {
            CaptureBeamComponent->Deactivate();
        }
    }
}

// ========================================================
// TowerBase overrides
// ========================================================

float AAttackTowerBase::GetAttackRange_Implementation() const
{
    return AttackRangeSphere ? AttackRangeSphere->GetScaledSphereRadius() : 0.f;
}

// Runtime adjustments
void AAttackTowerBase::ApplyFireRateToAttackComponent()
{
    if (AttackComponent)
    {
        AttackComponent->FireRate = FireRate;
    }
}

void AAttackTowerBase::SetFireRate(float InFireRate)
{
    FireRate = InFireRate;
    ApplyFireRateToAttackComponent();
}
