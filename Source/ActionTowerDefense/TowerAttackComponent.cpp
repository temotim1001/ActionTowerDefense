// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerAttackComponent.h"
#include "GameFramework/Actor.h"

UTowerAttackComponent::UTowerAttackComponent()
{
    // We let the owning tower drive TickAttack() manually
    PrimaryComponentTick.bCanEverTick = false;
}

void UTowerAttackComponent::AddTarget(AActor* InTarget)
{
    if (!IsValid(InTarget))
    {
        return;
    }

    // Avoid duplicates
    for (const TWeakObjectPtr<AActor>& Existing : EnemyQueue)
    {
        if (Existing.Get() == InTarget)
        {
            return;
        }
    }

    EnemyQueue.Add(InTarget);

    // If we had no target, lock onto this one immediately
    if (!CurrentTarget.IsValid())
    {
        CurrentTarget = InTarget;
    }
}

void UTowerAttackComponent::RemoveTarget(AActor* InTarget)
{
    if (!InTarget)
    {
        return;
    }

    EnemyQueue.RemoveAll(
        [InTarget](const TWeakObjectPtr<AActor>& Item)
        {
            return Item.Get() == InTarget;
        });

    if (CurrentTarget.Get() == InTarget)
    {
        CurrentTarget = nullptr;
    }
}

void UTowerAttackComponent::ClearTargets()
{
    EnemyQueue.Reset();
    CurrentTarget = nullptr;
}

AActor* UTowerAttackComponent::GetCurrentTarget() const
{
    return CurrentTarget.Get();
}

void UTowerAttackComponent::TickAttack(float DeltaSeconds, bool bCanFireNow)
{
    // Clean invalid current target
    if (CurrentTarget.IsValid() && !IsValid(CurrentTarget.Get()))
    {
        CurrentTarget = nullptr;
    }

    // Clean invalid items in queue
    EnemyQueue.RemoveAll(
        [](const TWeakObjectPtr<AActor>& Item)
        {
            return !IsValid(Item.Get());
        });

    // Select next target if needed
    if (!CurrentTarget.IsValid() && EnemyQueue.Num() > 0)
    {
        CurrentTarget = EnemyQueue[0];
        EnemyQueue.RemoveAt(0);
    }

    if (!bCanFireNow)
    {
        return;
    }

    if (!CurrentTarget.IsValid())
    {
        return;
    }

    FireCooldown -= DeltaSeconds;
    if (FireCooldown > 0.f)
    {
        return;
    }

    // Tell the tower to actually shoot
    OnTowerReadyToFire.Broadcast();

    FireCooldown = (FireRate > 0.f) ? (1.f / FireRate) : 0.f;
}
