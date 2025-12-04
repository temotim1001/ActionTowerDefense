// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TowerAttackComponent.generated.h"

class AActor;

// Fired when the component decides "it's time to shoot now"
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTowerReadyToFire);

/**
 * Handles:
 *  - Enemy queue in range
 *  - Selecting current target (FIFO)
 *  - Fire cooldown (using DeltaSeconds from tower, already scaled by game speed)
 *
 * Does NOT spawn projectiles – owning tower listens to OnTowerReadyToFire
 * and calls its FireProjectile() implementation.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ACTIONTOWERDEFENSE_API UTowerAttackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTowerAttackComponent();

    /** Shots per second at game speed 1x. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    float FireRate = 1.f;

    /** Event sent when cooldown is done and tower is allowed to fire. */
    UPROPERTY(BlueprintAssignable, Category = "Attack")
    FOnTowerReadyToFire OnTowerReadyToFire;

    /** Add an enemy we can potentially shoot at. */
    void AddTarget(AActor* InTarget);

    /** Remove a target that left / died. */
    void RemoveTarget(AActor* InTarget);

    /** Clear all targets (e.g. tower disabled). */
    void ClearTargets();

    /** Current target (may be nullptr). */
    AActor* GetCurrentTarget() const;

    /**
     * Tick attack logic.
     *  - DeltaSeconds should already be scaled by game speed.
     *  - bCanFireNow combines order state + "is aimed at target".
     */
    void TickAttack(float DeltaSeconds, bool bCanFireNow);

protected:
    /** Queue of potential enemies (FIFO). */
    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> EnemyQueue;

    /** Current target we’re focusing on. */
    UPROPERTY()
    TWeakObjectPtr<AActor> CurrentTarget;

    /** Internal cooldown timer. */
    float FireCooldown = 0.f;
};
