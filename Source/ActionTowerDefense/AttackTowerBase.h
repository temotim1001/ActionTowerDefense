// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerBase.h"
#include "TowerAttackComponent.h"  
#include "AttackTowerBase.generated.h"

class USphereComponent;
class AProjectile;
class UNiagaraSystem;
class UNiagaraComponent;
class UTowerAttackComponent;

UENUM(BlueprintType)
enum class ETowerOrderState : uint8
{
    AttackEnemies    UMETA(DisplayName = "Attack Enemies"),
    CaptureTower     UMETA(DisplayName = "Capture Tower"),
    AttackBonus      UMETA(DisplayName = "Attack Bonus"),
    HoldFire         UMETA(DisplayName = "Hold Fire"),
    Disabled         UMETA(DisplayName = "Disabled")
};

UCLASS()
class ACTIONTOWERDEFENSE_API AAttackTowerBase : public ATowerBase
{
    GENERATED_BODY()

public:
    AAttackTowerBase();
    virtual void Tick(float DeltaSeconds) override;

    // --- Orders exposed to PlayerController / BP ---
    UFUNCTION(BlueprintCallable, Category = "Tower|Orders")
    void OrderCaptureTower(ATowerBase* NeutralTower);

    UFUNCTION(BlueprintCallable, Category = "Tower|Orders")
    void OrderAttackBonus(AActor* BonusActor);

    UFUNCTION(BlueprintCallable, Category = "Tower|Orders")
    void OrderStopCurrentAction();

    UFUNCTION(BlueprintCallable, Category = "Attack")
    void SetFireRate(float InFireRate);

protected:
    virtual void BeginPlay() override;

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Components")
    USphereComponent* AttackRangeSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Components")
    USceneComponent* MuzzlePoint;

    // --- Combat settings ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float FireRate = 1.0f;   // shots per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float ProjectileDamage = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    TSubclassOf<AProjectile> ProjectileClass;

    void ApplyFireRateToAttackComponent();

    // --- Projectile movement ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Projectile")
    float ProjectileSpeed = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Projectile")
    bool bUseHoming = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Projectile", meta = (EditCondition = "bUseHoming"))
    float ProjectileHomingAcceleration = 8000.f;

    // --- Rotation ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aiming")
    float RotationSpeedDegPerSec = 90.f;   // how fast the tower turns

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aiming")
    float AimToleranceDeg = 5.f;           // how close (in degrees) before it may fire

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aiming")
    bool bUseYawOnly = true;              // ignore pitch for a flat 2D tower

    // --- Orders / state ---
    /** Optional forced target for special orders (capture / bonus). */
    UPROPERTY()
    AActor* ForcedOrderTarget = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tower|Orders")
    ETowerOrderState CurrentOrderState = ETowerOrderState::AttackEnemies;

    // Overlap callbacks
    UFUNCTION()
    void OnAttackRangeBeginOverlap(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnAttackRangeEndOverlap(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    // ================================
    // Internal Tick helpers
    // ================================
    void TickAttack(float DeltaSeconds);
    void UpdateOrderState(float DeltaSeconds);
    void RotateTowardsTarget(float DeltaSeconds);   
    bool IsAimedAtTarget() const;

    // Order/state helpers
    void SetOrderState(ETowerOrderState NewState, AActor* NewForcedTarget);
    bool IsCaptureOrderCompleted() const;

    // Tell TowerBase what our attack range is
    virtual float GetAttackRange_Implementation() const override;

    // --- Capture beam / VFX ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Capture|VFX")
    UNiagaraSystem* CaptureBeamSystem = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capture|VFX")
    UNiagaraComponent* CaptureBeamComponent = nullptr;

    void UpdateCaptureBeam(); // helper called from Tick

public:
    // For muzzle flash, sound etc.
    UFUNCTION(BlueprintImplementableEvent, Category = "Attack")
    void BP_OnAttackFired();

protected:
    // Allow child towers (e.g. Minigun) to customize the actual shot
    virtual void FireProjectile();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
    UTowerAttackComponent* AttackComponent = nullptr;

    UFUNCTION()
    void HandleAttackReadyToFire();
};
