// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USTTowerActionPanelWidget.generated.h"

class ATowerBase;

UCLASS()
class ACTIONTOWERDEFENSE_API USTTowerActionPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** The tower this panel is currently controlling. */
    UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = "true"), Category = "Tower")
    ATowerBase* BoundTower = nullptr;

    /** Called by PlayerController when tower selection changes. */
    UFUNCTION(BlueprintCallable, Category = "Tower")
    void SetBoundTower(ATowerBase* NewTower) { BoundTower = NewTower; }

    /** Convenience to check if we have a valid tower. */
    UFUNCTION(BlueprintPure, Category = "Tower")
    bool HasValidTower() const { return BoundTower != nullptr; }
};
