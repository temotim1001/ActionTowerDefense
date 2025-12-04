// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USTHUDWidget.generated.h"

class UTextBlock;
class UButton;
class ASTGameState;
class ASTGameController;

UCLASS()
class ACTIONTOWERDEFENSE_API USTHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // --- UI widgets (must match names in UMG) ---
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* LivesText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* WaveText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* NextWaveTimeText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TotalTimeText;

    // New speed buttons: -3x, 1x, 3x, 5x
    UPROPERTY(meta = (BindWidget))
    UButton* SpeedReverseButton;

    UPROPERTY(meta = (BindWidget))
    UButton* Speed1xButton;

    UPROPERTY(meta = (BindWidget))
    UButton* Speed3xButton;

    UPROPERTY(meta = (BindWidget))
    UButton* Speed5xButton;

    UPROPERTY(meta = (BindWidget))
    UButton* CallNextWaveButton;

    // --- references to game objects (read-only / commands) ---
    UPROPERTY()
    ASTGameState* GameStateRef = nullptr;

    UPROPERTY()
    ASTGameController* GameControllerRef = nullptr;

    // --- button handlers ---
    // --- button handlers ---
    UFUNCTION()
    void HandleSpeedReverseClicked();

    UFUNCTION()
    void HandleSpeed1xClicked();

    UFUNCTION()
    void HandleSpeed3xClicked();

    UFUNCTION()
    void HandleSpeed5xClicked();

    UFUNCTION()
    void HandleCallNextWaveClicked();

    UFUNCTION()
    void HandleReversePressed();

    UFUNCTION()
    void HandleReverseReleased();

    // --- helpers ---
    FString FormatSeconds(float Seconds) const;
    void RefreshFromGameState();
    void RefreshSpeedButtons();   // new: highlights active speed 
};
