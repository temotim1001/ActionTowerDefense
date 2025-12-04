// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "STGameState.generated.h"

UCLASS()
class ACTIONTOWERDEFENSE_API ASTGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 Score = 0;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 Lives = 20;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 CurrentWaveIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    int32 TotalWaves = 10;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    float TimeToNextWave = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    float TotalTimeElapsed = 0.f;

    // 🔹 HUD can bind to this to show current speed
    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    float CurrentSpeed = 1.f;

    // NEW: basic game result flags
    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    bool bIsGameOver = false;

    UPROPERTY(BlueprintReadOnly, Category = "HUD")
    bool bPlayerWon = false;

    ASTGameState();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // --- Convenience accessors for game speed ---

    UFUNCTION(BlueprintPure, Category = "Game Speed")
    FORCEINLINE float GetCurrentSpeed() const { return CurrentSpeed; }

    // Convenience helper so other classes can find the game state
    UFUNCTION(BlueprintPure, Category = "Game", meta = (WorldContext = "WorldContextObject", DisplayName = "Get STGameState"))
    static ASTGameState* Get(const UObject* WorldContextObject);
};
