// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TowerBase.h"
#include "AttackTowerBase.generated.h"

class USphereComponent;
class AProjectile;
class UNiagaraSystem;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class ETowerOrderState : uint8
{
    AttackEnemies   UMETA(DisplayName = "Attack Enemies"),
    CaptureTower    UMETA(DisplayName = "Capture Tower"),
    AttackBonus     UMETA(DisplayName = "Attack Bonus")
};

UCLASS()
class ACTIONTOWERDEFENSE_API AAttackTowerBase : public ATowerBase
{
    GENERATED_BODY()

public:
    AAttackTowerBase();

    virtual void Tick(float DeltaSeconds) override;

    // --- Orders exposed to PlayerController / BP ---

    /** Order this tower to capture the given neutral tower. */
    UFUNCTION(BlueprintCallable, Category = "Tower|Orders")
    void OrderCaptureTower(ATowerBase* NeutralTower);

    /** Order this tower to attack a bonus item (not implemented yet). */
    UFUNCTION(BlueprintCallable, Category = "Tower|Orders")
    void OrderAttackBonus(AActor* BonusActor);

    /** Cancel current special order and return to attacking enemies. */
    UFUNCTION(BlueprintCallable, Category = "Tower|Orders")
    void OrderStopCurrentAction();

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

    float FireCooldown = 0.f;

    // First-seen-first-out target queue
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> EnemyQueue;

    UPROPERTY()
    TWeakObjectPtr<AActor> CurrentTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Projectile")
    float ProjectileSpeed = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Projectile")
    bool bUseHoming = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Projectile", meta = (EditCondition = "bUseHoming"))
    float ProjectileHomingAcceleration = 8000.f;

    // Rotation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aiming")
    float RotationSpeedDegPerSec = 90.f;   // how fast the tower turns

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aiming")
    float AimToleranceDeg = 5.f;           // how close (in degrees) before it may fire

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack|Aiming")
    bool bUseYawOnly = true;              // ignore pitch for a flat 2D tower

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

    // Helpers
    void CleanupTargetQueue();
    void SelectNextTarget();
    void AttackTick(float DeltaSeconds);
    void TryFire(float DeltaSeconds);
    virtual void FireProjectile();

    bool IsEnemyValid(AActor* EnemyActor) const;

    // Rotation helpers
    void RotateTowardsTarget(float DeltaSeconds);
    bool IsAimedAtTarget() const;

    /** Internal helper to update state + capture/beam targets. */
    void SetOrderState(ETowerOrderState NewState, AActor* NewForcedTarget);

    /** Returns true when current capture order has finished. */
    bool IsCaptureOrderCompleted() const;

    // Tell TowerBase what our attack range is
    virtual float GetAttackRange_Implementation() const override;

    /** Which Niagara system to use for the capture beam (set in BP). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Capture|VFX")
    UNiagaraSystem* CaptureBeamSystem = nullptr;

    /** Component that actually plays the capture beam. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Capture|VFX")
    UNiagaraComponent* CaptureBeamComponent = nullptr;

    void UpdateCaptureBeam(); // helper called from Tick

public:
    // For muzzle flash, sound etc.
    UFUNCTION(BlueprintImplementableEvent, Category = "Attack")
    void BP_OnAttackFired();
};
