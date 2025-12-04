// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerBase.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "STGameState.h"

ATowerBase::ATowerBase()
{
    PrimaryActorTick.bCanEverTick = true;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    RootComponent = MeshComp;

    // --- Selection ring ---
    SelectionRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
    SelectionRingMesh->SetupAttachment(RootComponent);
    SelectionRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionRingMesh->SetGenerateOverlapEvents(false);
    SelectionRingMesh->SetCastShadow(false);
    SelectionRingMesh->bReceivesDecals = false;
    SelectionRingMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    SelectionRingMesh->SetHiddenInGame(true);

    // --- Range ring ---
    RangeRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangeRing"));
    RangeRingMesh->SetupAttachment(RootComponent);
    RangeRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RangeRingMesh->SetGenerateOverlapEvents(false);
    RangeRingMesh->SetCastShadow(false);
    RangeRingMesh->bReceivesDecals = false;
    RangeRingMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    RangeRingMesh->SetHiddenInGame(true);

    // Reasonable defaults
    CaptureHPMax = 100.f;
    CaptureHP = CaptureHPMax;
    CaptureRange = 1000.f;
    CaptureDPS = 1.f;
    bIsCurrentlyCapturing = false;
}

void ATowerBase::BeginPlay()
{
    Super::BeginPlay();

    // Ensure capture HP starts at max
    CaptureHP = CaptureHPMax;

    UpdateSelectionVisuals(); // ensure visuals match team + selection
}

void ATowerBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Apply global game speed to capture logic
    float EffectiveDelta = DeltaSeconds;

    float Speed = 1.f;
    if (ASTGameState* GS = ASTGameState::Get(this))
    {
        Speed = GS->GetCurrentSpeed();  // -3, 1, 3, 5
    }

    // Rule: no capture progress while rewinding or paused
    if (Speed <= 0.f)
    {
        StopCaptureBeam();   // make sure beam visuals are off
        return;
    }

    EffectiveDelta *= Speed; // 1x / 3x / 5x

    TickCapture(EffectiveDelta);
}

// Capture neutral tower function
void ATowerBase::TickCapture(float DeltaSeconds)
{
    if (Team != ETowerTeam::Player)
    {
        StopCaptureBeam();
        return;
    }

    if (!AssignedCaptureTarget)
    {
        StopCaptureBeam();
        return;
    }

    if (!IsValidCaptureTarget(AssignedCaptureTarget))
    {
        StopCaptureBeam();
        AssignedCaptureTarget = nullptr;
        return;
    }

    const float CaptureAmount = CaptureDPS * DeltaSeconds;

    StartCaptureBeam();
    AssignedCaptureTarget->ApplyCaptureDamage(CaptureAmount, this);
}

bool ATowerBase::IsValidCaptureTarget(ATowerBase* Target) const
{
    if (!Target)
    {
        return false;
    }

    // Only neutral towers can be captured in this version
    if (Target->Team != ETowerTeam::Neutral)
    {
        return false;
    }

    // Range check
    const float DistanceSq = FVector::DistSquared(GetActorLocation(), Target->GetActorLocation());
    const float RangeSq = CaptureRange * CaptureRange;

    return DistanceSq <= RangeSq;
}

void ATowerBase::SetCaptureTarget(ATowerBase* NewTarget)
{
    // Optional: only allow targets that are not this tower
    if (NewTarget == this)
    {
        AssignedCaptureTarget = nullptr;
        return;
    }

    AssignedCaptureTarget = NewTarget;
}

void ATowerBase::ApplyCaptureDamage(float Amount, ATowerBase* SourceTower)
{
    if (!SourceTower)
    {
        return;
    }

    // Only neutral towers can be captured in this simple rule set
    if (Team != ETowerTeam::Neutral)
    {
        return;
    }

    // Decrease capture HP
    CaptureHP = FMath::Clamp(CaptureHP - Amount, 0.f, CaptureHPMax);

    // Notify Blueprint about progress (for UI, etc.)
    BP_OnCaptureProgressChanged(CaptureHP, CaptureHPMax);

    if (CaptureHP <= 0.f)
    {
        OnCapturedBy(SourceTower);
    }
}

void ATowerBase::OnCapturedBy(ATowerBase* SourceTower)
{
    if (!SourceTower)
    {
        return;
    }

    const ETowerTeam NewTeam = SourceTower->Team;

    Team = NewTeam;
    CaptureHP = CaptureHPMax;

    BP_OnCaptured(Team);

    // Stop any capture beam this tower might have been doing
    StopCaptureBeam();
    AssignedCaptureTarget = nullptr;

    // If we *don’t* upgrade into a new tower instance,
    // we still want the selection visuals to reflect the new team.
    UpdateSelectionVisuals();

    // --- Upgrade logic: replace this actor with UpgradeTowerClass, if set ---
    if (UpgradeTowerClass && NewTeam == ETowerTeam::Player)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            const FTransform SpawnTransform = GetActorTransform();

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            Params.Owner = SourceTower->GetOwner();

            ATowerBase* NewTower = World->SpawnActor<ATowerBase>(
                UpgradeTowerClass,
                SpawnTransform,
                Params
            );

            if (NewTower)
            {
                NewTower->Team = NewTeam;
                NewTower->CaptureHP = NewTower->CaptureHPMax;

                // Optional: you can add some initialisation call here if needed
                // e.g. NewTower->OnUpgradedFrom(this);
            }
        }

        // Destroy the old neutral tower
        Destroy();
    }
}

void ATowerBase::SetSelected(bool bNewSelected)
{
    if (bIsSelected == bNewSelected)
        return;

    bIsSelected = bNewSelected;

    UpdateSelectionVisuals();
    BP_OnSelectionChanged(bIsSelected);
}

// Helpers
void ATowerBase::StartCaptureBeam()
{
    if (!bIsCurrentlyCapturing)
    {
        bIsCurrentlyCapturing = true;
        BP_OnCaptureBeamStarted(AssignedCaptureTarget);
    }
}

void ATowerBase::StopCaptureBeam()
{
    if (bIsCurrentlyCapturing)
    {
        bIsCurrentlyCapturing = false;
        BP_OnCaptureBeamStopped();
    }
}

// Selection visuals below
float ATowerBase::GetAttackRange_Implementation() const
{
    return 0.f; // Neutral towers do not attack by default
}

void ATowerBase::UpdateSelectionVisuals()
{
    // Always hide everything if not selected
    SelectionRingMesh->SetHiddenInGame(!bIsSelected);

    const float AttackRange = GetAttackRange();
    const bool bHasAttackRange = AttackRange > 0.f;

    // Only show range ring if:
    // - tower is selected
    // - AND it actually has an attack range
    RangeRingMesh->SetHiddenInGame(!(bIsSelected && bHasAttackRange));

    if (!bIsSelected)
    {
        return;
    }

    // --- Apply selection material based on Team ---
    UMaterialInterface* SelectedMat = nullptr;

    switch (Team)
    {
    case ETowerTeam::Player:
        SelectedMat = PlayerSelectionMaterial;   // green in your material
        break;
    case ETowerTeam::Neutral:
        SelectedMat = NeutralSelectionMaterial;  // grey
        break;
    case ETowerTeam::Enemy:
        SelectedMat = EnemySelectionMaterial;    // red if you want
        break;
    default:
        break;
    }

    if (SelectedMat)
    {
        SelectionRingMesh->SetMaterial(0, SelectedMat);
    }

    // --- Attack range ring only if tower has a range ---
    if (bHasAttackRange && RangeRingMaterial)
    {
        RangeRingMesh->SetMaterial(0, RangeRingMaterial);

        // Assumes the ring mesh has radius 50uu → adjust to your mesh
        constexpr float MeshRadiusUU = 50.f;
        const float Scale = AttackRange / MeshRadiusUU;

        RangeRingMesh->SetWorldScale3D(FVector(Scale, Scale, 1.f));
    }

}

void ATowerBase::LevelUp()
{
    // Default: do nothing.
    // Later: override in C++ or Blueprint for specific tower types.
}

void ATowerBase::UpgradeTower()
{
    // Only allow upgrading for player-owned towers (generic rule).
    if (Team != ETowerTeam::Player)
    {
        return;
    }

    // No upgrade path configured for this tower.
    if (!UpgradeTowerClass)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FTransform SpawnTransform = GetActorTransform();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Params.Owner = GetOwner(); // same owner as current tower

    ATowerBase* NewTower = World->SpawnActor<ATowerBase>(
        UpgradeTowerClass,
        SpawnTransform,
        Params
    );

    if (NewTower)
    {
        // Inherit team & reset capture HP
        NewTower->Team = Team;
        NewTower->CaptureHP = NewTower->CaptureHPMax;

        // If you later add levels, you can copy them here too
        // NewTower->CurrentLevel = CurrentLevel;

        // Optional: Blueprint hook, if you want
        // NewTower->BP_OnUpgradedFrom(this);
    }

    // Destroy the old tower (this).
    Destroy();
}

