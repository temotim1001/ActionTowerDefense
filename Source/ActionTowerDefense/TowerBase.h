// Fill out your copyright notice in the Description page of Project Settings.

#pragma once   // ← MUST be the very first line. Nothing above this.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TowerBase.generated.h"   // ← MUST be the last include. Only once.

class UStaticMeshComponent;
class UMaterialInterface;

// ------------------ Team enum ------------------

UENUM(BlueprintType)
enum class ETowerTeam : uint8
{
    Neutral UMETA(DisplayName = "Neutral"),
    Player  UMETA(DisplayName = "Player"),
    Enemy   UMETA(DisplayName = "Enemy")
};

// ------------------ Tower class ------------------

UCLASS()
class ACTIONTOWERDEFENSE_API ATowerBase : public AActor
{
    GENERATED_BODY()

public:
    ATowerBase();

    virtual void Tick(float DeltaSeconds) override;

    // --- Selection API ---
    void SetSelected(bool bNewSelected);

    UFUNCTION(BlueprintPure, Category = "Selection")
    FORCEINLINE bool IsSelected() const { return bIsSelected; }

    // --- Team / ownership API ---

    UFUNCTION(BlueprintPure, Category = "Tower")
    FORCEINLINE ETowerTeam GetTeam() const { return Team; }

    UFUNCTION(BlueprintCallable, Category = "Tower")
    void SetTeam(ETowerTeam NewTeam);

    // --- Capture API helpers for UI / logic ---

    /** 0..1 capture progress (for UI bars etc.). */
    UFUNCTION(BlueprintPure, Category = "Tower|Capture")
    float GetCaptureProgress01() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickCapture(float DeltaSeconds);
    
    // For drawing range, towers can override this in C++ or BP
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
    float GetAttackRange() const;
    virtual float GetAttackRange_Implementation() const;
    
    void StartCaptureBeam();
    void StopCaptureBeam();

    // --- Helpers ---
    bool IsValidCaptureTarget(ATowerBase* Target) const;
    void OnCapturedBy(ATowerBase* SourceTower);

    // NEW: update rings when selection/team changes
    void UpdateSelectionVisuals();

public:

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComp;

    // NEW: visual components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
    UStaticMeshComponent* SelectionRingMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
    UStaticMeshComponent* RangeRingMesh;

    // Team / Ownership
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tower")
    ETowerTeam Team = ETowerTeam::Neutral;

    // Capture System
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
    float CaptureHPMax = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capture")
    float CaptureHP;

    /** The tower this tower is currently attempting to capture.
    *  Only valid when Team == Player and TickCapture is running.
    */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capture")
    ATowerBase* AssignedCaptureTarget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
    float CaptureRange = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
    float CaptureDPS = 10.f;

    /** True while the capture beam is visually active. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capture")
    bool bIsCurrentlyCapturing = false;

    // Capture API
    UFUNCTION(BlueprintCallable, Category = "Capture")
    void ApplyCaptureDamage(float Amount, ATowerBase* SourceTower);

    UFUNCTION(BlueprintCallable, Category = "Capture")
    void SetCaptureTarget(ATowerBase* NewTarget);

    // Blueprint events (you implement these in BP)
    UFUNCTION(BlueprintImplementableEvent, Category = "Capture")
    void BP_OnCaptureProgressChanged(float NewHP, float MaxHP);

    UFUNCTION(BlueprintImplementableEvent, Category = "Capture")
    void BP_OnCaptured(ETowerTeam NewTeam);

    UFUNCTION(BlueprintImplementableEvent, Category = "Capture")
    void BP_OnCaptureBeamStarted(ATowerBase* TargetTower);

    UFUNCTION(BlueprintImplementableEvent, Category = "Capture")
    void BP_OnCaptureBeamStopped();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capture")
    TSubclassOf<ATowerBase> UpgradeTowerClass;

    UFUNCTION(BlueprintCallable, Category = "Tower|Progression")
    virtual void LevelUp();

    UFUNCTION(BlueprintCallable, Category = "Tower|Progression")
    virtual void UpgradeTower();



protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
    bool bIsSelected = false;

    // NEW: materials you can assign per-Blueprint
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
    UMaterialInterface* PlayerSelectionMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
    UMaterialInterface* NeutralSelectionMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
    UMaterialInterface* EnemySelectionMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
    UMaterialInterface* RangeRingMaterial;

    UFUNCTION(BlueprintImplementableEvent, Category = "Selection")
    void BP_OnSelectionChanged(bool bNewSelected);

};
