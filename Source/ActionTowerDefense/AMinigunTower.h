// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttackTowerBase.h"
#include "AMinigunTower.generated.h"

UCLASS()
class ACTIONTOWERDEFENSE_API AMinigunTower : public AAttackTowerBase
{
    GENERATED_BODY()

public:
    AMinigunTower();

protected:
    // Three separate muzzle points
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Components")
    USceneComponent* MuzzlePoint1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Components")
    USceneComponent* MuzzlePoint2;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack|Components")
    USceneComponent* MuzzlePoint3;

    // Which muzzle we use next (0,1,2)
    int32 CurrentMuzzleIndex = 0;

    virtual void FireProjectile() override;

    // Helper to get the current muzzle component
    USceneComponent* GetCurrentMuzzleComponent() const;
};