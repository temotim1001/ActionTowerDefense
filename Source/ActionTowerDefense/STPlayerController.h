// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "STGameController.h"
#include "STPlayerController.generated.h"

class ATowerBase;
class USTHUDWidget;
class USTTowerActionPanelWidget;   // NEW

UCLASS()
class ACTIONTOWERDEFENSE_API ASTPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASTPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    // NEW: we want to move the panel each frame to follow the tower
    virtual void Tick(float DeltaSeconds) override;

    void SetSelectedTower(ATowerBase* NewTower);
    void ClearSelection();

    /** Called when the Select button (e.g. Left Mouse) is pressed */
    void OnSelectPressed();

    /** Currently selected tower (if any) */
    UPROPERTY()
    ATowerBase* SelectedTower = nullptr;   // init for safety

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USTHUDWidget> HUDWidgetClass;

    UPROPERTY()
    USTHUDWidget* HUDWidget = nullptr;

    // --- NEW: Tower action panel UI ---

    /** Widget class for the tower action panel (set in BP). */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<USTTowerActionPanelWidget> TowerActionPanelClass;

    /** Live instance of the tower action panel. */
    UPROPERTY()
    USTTowerActionPanelWidget* TowerActionPanel = nullptr;

    /** Screen offset so the panel appears just under the tower. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    FVector2D TowerPanelScreenOffset = FVector2D(0.f, 50.f);

public:

    // --- Game speed API for HUD ---

    UFUNCTION(BlueprintCallable, Category = "Game Speed")
    void RequestSpeedMode(EGameSpeedMode NewMode);

    UFUNCTION(BlueprintPure, Category = "Game Speed")
    EGameSpeedMode GetCurrentSpeedMode() const;
};
