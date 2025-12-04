// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackTowerBase.h"
#include "Components/SphereComponent.h"
#include "Projectile.h"
#include "DamageableTarget.h"
#include "Engine/Engine.h"
#include "STGameController.h"
#include "NiagaraComponent.h"        
#include "NiagaraSystem.h" 

AAttackTowerBase::AAttackTowerBase()
{
    PrimaryActorTick.bCanEverTick = true;

    // Attachments to existing MeshComp from ATowerBase
    
    AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangeSphere"));
    AttackRangeSphere->SetupAttachment(MeshComp);
    AttackRangeSphere->SetSphereRadius(1000.f);

    AttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AttackRangeSphere->SetGenerateOverlapEvents(true);

    // Start from "ignore everything"
    AttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

    // Overlap only enemies
    AttackRangeSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);

    MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
    MuzzlePoint->SetupAttachment(MeshComp);
    MuzzlePoint->SetRelativeLocation(FVector(100.f, 0.f, 50.f)); // tweak in BP

    CaptureBeamComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CaptureBeamFX"));
    CaptureBeamComponent->SetupAttachment(MuzzlePoint);   // beam starts at muzzle
    CaptureBeamComponent->SetAutoActivate(false);
}

void AAttackTowerBase::BeginPlay()
{
    Super::BeginPlay();

    // NEW: now that editor / BP values are applied, sync capture range to attack range
    if (AttackRangeSphere)
    {
        CaptureRange = AttackRangeSphere->GetScaledSphereRadius();
    }

    AttackRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AAttackTowerBase::OnAttackRangeBeginOverlap);
    AttackRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AAttackTowerBase::OnAttackRangeEndOverlap);

    // Assign Niagara system if set
    if (CaptureBeamComponent && CaptureBeamSystem)
    {
        CaptureBeamComponent->SetAsset(CaptureBeamSystem);
    }
}

void AAttackTowerBase::Tick(float DeltaSeconds)
{
    // 1) Base Tick: capture logic (TowerBase handles capture here)
    Super::Tick(DeltaSeconds);

    // 2) Apply global speed
    float EffectiveDelta = DeltaSeconds;

    if (ASTGameController* GC = ASTGameController::Get(this))
    {
        const float Speed = GC->GetGameSpeed();
        if (Speed <= 0.f)
        {
            // No firing while rewinding or paused
            UpdateCaptureBeam(); // also ensure beam is off if needed
            return;
        }

        EffectiveDelta *= Speed;
    }

    // 3) Behavior based on current order
    switch (CurrentOrderState)
    {
    case ETowerOrderState::AttackEnemies:
        AttackTick(EffectiveDelta);
        break;

    case ETowerOrderState::CaptureTower:
        // Capture is handled by ATowerBase::Tick / TickCapture
        break;

    case ETowerOrderState::AttackBonus:
        // TODO: bonus logic later
        break;
    }

    // 4) If capture is done, revert to attacking enemies
    if (CurrentOrderState == ETowerOrderState::CaptureTower &&
        IsCaptureOrderCompleted())
    {
        SetOrderState(ETowerOrderState::AttackEnemies, nullptr);
    }

    // 5) Update capture beam VFX
    UpdateCaptureBeam();
}

// Only run capture logic when we are actually in CaptureTower state
/* To be deleted - handled by super (Towerbase)
void AAttackTowerBase::TickCapture(float DeltaSeconds)
{
    if (CurrentOrderState == ETowerOrderState::CaptureTower)
    {
        // Run the normal capture logic from ATowerBase
        Super::TickCapture(DeltaSeconds);
    }
    else
    {
        // Make sure we’re not showing a capture beam in other states
        StopCaptureBeam();
        // Optional: don’t clear AssignedCaptureTarget here so we can resume later if you want
    }
}
*/

// =======================
// Overlap / Target queue
// =======================

void AAttackTowerBase::OnAttackRangeBeginOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
        return;

    // Just in case, ignore towers
    if (Cast<ATowerBase>(OtherActor))
        return;

    EnemyQueue.AddUnique(OtherActor);

    if (!CurrentTarget.IsValid())
    {
        CurrentTarget = OtherActor;
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 2.f, FColor::Green,
            FString::Printf(TEXT("Added to queue: %s | QueueSize=%d"),
                *GetNameSafe(OtherActor),
                EnemyQueue.Num())
        );
    }
}

void AAttackTowerBase::OnAttackRangeEndOverlap(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!OtherActor)
        return;

    EnemyQueue.RemoveAll([OtherActor](const TWeakObjectPtr<AActor>& Item)
        {
            return Item.Get() == OtherActor;
        });

    if (CurrentTarget.Get() == OtherActor)
    {
        CurrentTarget = nullptr;
    }
}

bool AAttackTowerBase::IsEnemyValid(AActor* EnemyActor) const
{
    // Minimal version: only check that it still exists
    return IsValid(EnemyActor);

    /*
    if (!IsValid(EnemyActor))
        return false;

    const float DistSq = FVector::DistSquared(
        EnemyActor->GetActorLocation(),
        GetActorLocation()
    );
    const float RangeSq = FMath::Square(AttackRangeSphere->GetScaledSphereRadius());

    return DistSq <= RangeSq;
    */
}

void AAttackTowerBase::CleanupTargetQueue()
{
    EnemyQueue.RemoveAll([](const TWeakObjectPtr<AActor>& Item)
        {
            return !IsValid(Item.Get());
        });

    if (CurrentTarget.IsValid() && !IsValid(CurrentTarget.Get()))
    {
        CurrentTarget = nullptr;
    }
}

void AAttackTowerBase::SelectNextTarget()
{
    for (int32 i = 0; i < EnemyQueue.Num(); ++i)
    {
        AActor* EnemyActor = EnemyQueue[i].Get();
        if (IsEnemyValid(EnemyActor))
        {
            CurrentTarget = EnemyActor; // first valid in list = first seen
            return;
        }
    }

    CurrentTarget = nullptr;
}

// =======================
// Firing
// =======================

void AAttackTowerBase::AttackTick(float DeltaSeconds)
{
    CleanupTargetQueue();

    if (!CurrentTarget.IsValid() && EnemyQueue.Num() > 0)
    {
        CurrentTarget = EnemyQueue[0];
    }

    if (CurrentTarget.IsValid())
    {
        RotateTowardsTarget(DeltaSeconds);
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 0.f, FColor::Cyan,
            FString::Printf(TEXT("CurrentTarget: %s | Queue: %d"),
                *GetNameSafe(CurrentTarget.Get()),
                EnemyQueue.Num())
        );
    }

    TryFire(DeltaSeconds);
}

void AAttackTowerBase::TryFire(float DeltaSeconds)
{
    if (!CurrentTarget.IsValid())
        return;

    if (!IsAimedAtTarget())
        return; // not lined up yet

    if (!ProjectileClass)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 1.f, FColor::Red,
                TEXT("No ProjectileClass set on tower!")
            );
        }
        return;
    }

    FireCooldown -= DeltaSeconds;
    if (FireCooldown > 0.f)
        return;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1, 1.f, FColor::Yellow,
            TEXT("TryFire → calling FireProjectile"));
    }

    FireProjectile();

    FireCooldown = (FireRate > 0.f) ? (1.f / FireRate) : 0.f;
}

void AAttackTowerBase::FireProjectile()
{
    if (!CurrentTarget.IsValid() || !ProjectileClass)
        return;

    const FVector SpawnLocation = MuzzlePoint->GetComponentLocation();
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

        BP_OnAttackFired(); // let BP handle muzzle flash / sound / animation
    }
}

// =======================
// Rotation helpers
// =======================

void AAttackTowerBase::RotateTowardsTarget(float DeltaSeconds)
{
    if (!CurrentTarget.IsValid())
        return;

    FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();

    if (bUseYawOnly)
    {
        ToTarget.Z = 0.f; // flat rotation
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
    if (!CurrentTarget.IsValid())
        return false;

    FVector Forward = GetActorForwardVector();
    FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();

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

// =======================
// Order/state helpers
// =======================

void AAttackTowerBase::SetOrderState(ETowerOrderState NewState, AActor* NewForcedTarget)
{
    CurrentOrderState = NewState;
    ForcedOrderTarget = NewForcedTarget;

    switch (CurrentOrderState)
    {
    case ETowerOrderState::AttackEnemies:
        // Cancel any capture and stop the beam
        SetCaptureTarget(nullptr);   // ATowerBase API
        StopCaptureBeam();           // ATowerBase helper
        break;

    case ETowerOrderState::CaptureTower:
    {
        ATowerBase* TargetTower = Cast<ATowerBase>(ForcedOrderTarget);
        if (TargetTower && TargetTower->Team == ETowerTeam::Neutral)
        {
            // This is what actually starts capture:
            // ATowerBase::Tick sees AssignedCaptureTarget and drains CaptureHP.
            SetCaptureTarget(TargetTower);
        }
        else
        {
            // Invalid target -> revert safely
            CurrentOrderState = ETowerOrderState::AttackEnemies;
            ForcedOrderTarget = nullptr;
            SetCaptureTarget(nullptr);
            StopCaptureBeam();
        }
    }
    break;

    case ETowerOrderState::AttackBonus:
        // No capture for bonus items
        SetCaptureTarget(nullptr);
        StopCaptureBeam();
        break;
    }
}

bool AAttackTowerBase::IsCaptureOrderCompleted() const
{
    // If pointer is null or actor already destroyed, we're done
    if (!AssignedCaptureTarget || !IsValid(AssignedCaptureTarget))
    {
        return true;
    }

    // If the target is no longer neutral, someone captured/upgraded it -> done
    if (AssignedCaptureTarget->Team != ETowerTeam::Neutral)
    {
        return true;
    }

    // If it's not a valid capture target (too far etc.), also done
    if (!IsValidCaptureTarget(AssignedCaptureTarget))
    {
        return true;
    }

    return false;
}

// =======================
// Public order API
// =======================

void AAttackTowerBase::OrderCaptureTower(ATowerBase* NeutralTower)
{
    if (!NeutralTower)
        return;

    if (NeutralTower->Team != ETowerTeam::Neutral)
        return;

    SetOrderState(ETowerOrderState::CaptureTower, NeutralTower);
}

void AAttackTowerBase::OrderAttackBonus(AActor* BonusActor)
{
    if (!BonusActor)
        return;

    // TODO: later implement proper bonus-targeting logic
    SetOrderState(ETowerOrderState::AttackBonus, BonusActor);
}


void AAttackTowerBase::OrderStopCurrentAction()
{
    SetOrderState(ETowerOrderState::AttackEnemies, nullptr);
}

float AAttackTowerBase::GetAttackRange_Implementation() const
{
    // Use the actual collision sphere radius as attack range
    return AttackRangeSphere ? AttackRangeSphere->GetScaledSphereRadius() : 0.f;
}

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