// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "STEnemyLifeComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnEnemyRemoved,
    AActor*, EnemyActor,
    bool, bReachedGoal
);

/**
 * Simple lifecycle component for enemies (works with BP-only enemies).
 * - BP calls MarkEnemyRemoved(true/false) when it dies or reaches goal
 * - GameController (and others) can bind to OnEnemyRemoved
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ACTIONTOWERDEFENSE_API USTEnemyLifeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USTEnemyLifeComponent();

    /** Fired once when the enemy is removed from play (dead or leaked). */
    UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
    FOnEnemyRemoved OnEnemyRemoved;

    /** Called from BP when enemy dies or reaches goal. */
    UFUNCTION(BlueprintCallable, Category = "Enemy|Life")
    void MarkEnemyRemoved(bool bReachedGoal);

protected:
    /** Ensure we only broadcast once. */
    bool bHasBeenRemoved = false;
};
