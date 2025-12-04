// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "STGameController.generated.h"

class ASTGameState;
class ASTSpawner; // your wave spawner
class USTEndGameWidget;

UENUM(BlueprintType)
enum class EGameSpeedMode : uint8
{
    Reverse     UMETA(DisplayName = "-3x"),
    Normal      UMETA(DisplayName = "1x"),
    Fast        UMETA(DisplayName = "3x"),
    VeryFast    UMETA(DisplayName = "5x"),
};

UCLASS()
class ACTIONTOWERDEFENSE_API ASTGameController : public AActor
{
    GENERATED_BODY()

public:
    ASTGameController();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;   // ← ADD THIS

    // NEW: handlers for spawner events
    UFUNCTION()
    void HandleWaveStarted(int32 WaveIndex, int32 TotalWaves);

    UFUNCTION()
    void HandleNextWaveScheduled(float TimeUntilNextWave);

public:
    UPROPERTY(BlueprintReadOnly, Category = "Refs")
    ASTGameState* STGameStateRef = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Refs")
    ASTSpawner* SpawnerRef = nullptr;

    // --- Global time/speed ---

    UPROPERTY(BlueprintReadOnly, Category = "Game Speed")
    EGameSpeedMode SpeedMode = EGameSpeedMode::Normal;

    // The actual scalar used everywhere: -3, 1, 3, 5
    UPROPERTY(BlueprintReadOnly, Category = "Game Speed")
    float CurrentSpeed = 1.f;

    // ---- Reverse system ----
    UPROPERTY(BlueprintReadOnly, Category = "Reverse")
    bool bIsReversing = false;

    UPROPERTY(BlueprintReadOnly, Category = "Reverse")
    float PreviousNonReverseSpeed = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverse")
    float ReverseSpeed = -3.f;

    // ---- Reverse Meter ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverse")
    float MaxReverseMeter = 100.f;

    UPROPERTY(BlueprintReadOnly, Category = "Reverse")
    float CurrentReverseMeter = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverse")
    float ReverseDrainPerSecond = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverse")
    float ReverseGainPerKill = 5.f;

    UFUNCTION(BlueprintCallable, Category = "Reverse")
    void StartReverse();

    UFUNCTION(BlueprintCallable, Category = "Reverse")
    void StopReverse();

    bool CanUseReverse() const { return CurrentReverseMeter > 0.1f; }

    UFUNCTION(BlueprintCallable, Category = "Game Speed")
    void SetSpeedMode(EGameSpeedMode NewMode);

    UFUNCTION(BlueprintPure, Category = "Game Speed")
    FORCEINLINE float GetGameSpeed() const { return CurrentSpeed; }

    // Convenience helper so other classes can find the controller
    static ASTGameController* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "Game", meta = (WorldContext = "WorldContextObject", DisplayName = "Get STGameController"))
    static ASTGameController* GetSTGameController(const UObject* WorldContextObject)
    {
        // Reuse your existing C++ helper
        return Get(WorldContextObject);
    }

    // HUD will call these – this is your "API"
    UFUNCTION(BlueprintCallable, Category = "Commands")
    void Command_SetGameSpeed(float NewSpeed);

    UFUNCTION(BlueprintCallable, Category = "Commands")
    void Command_RequestNextWave();

    // ======================
    //      SCORE SYSTEM
    // ======================

    /** How many score points we lose per second of rewind at 1x reverse speed.
     *  Actual drain = ReverseScoreCostPerSecond * |CurrentSpeed| * DeltaSeconds when speed < 0.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score")
    float ReverseScoreCostPerSecond = 500.f;

    /** Internal float-based score; GameState->Score mirrors this as an int. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Score")
    float ScoreInternal = 0.f;

    /** Award score for killing an enemy with the given base value.
     *  Applies speed multiplier, but gives 0 if CurrentSpeed <= 0 (reverse / paused).
     */
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AwardScoreForEnemy(float BaseEnemyScore);

    /** Generic score change (can be positive or negative). */
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddRawScore(float ScoreDelta);

    // ======================
    //      LIVES / STATE
    // ======================

    // How many lives to start with (can tweak per level)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
    int32 StartLives = 20;

    // Current enemies alive in the world
    UPROPERTY(BlueprintReadOnly, Category = "Game|Enemies")
    int32 NumEnemiesAlive = 0;

    // Simple state flags
    UPROPERTY(BlueprintReadOnly, Category = "Game")
    bool bIsGameOver = false;

    UPROPERTY(BlueprintReadOnly, Category = "Game")
    bool bPlayerWon = false;

    // Called whenever an enemy is spawned (from BP)
    UFUNCTION(BlueprintCallable, Category = "Game|Enemies")
    void NotifyEnemySpawned();

    // Called whenever an enemy is removed from play
    // bReachedGoal = true if it leaked through to the end
    UFUNCTION(BlueprintCallable, Category = "Game|Enemies")
    void NotifyEnemyRemoved(bool bReachedGoal);

    // Explicit life loss; in practice only used from NotifyEnemyRemoved
    UFUNCTION(BlueprintCallable, Category = "Game")
    void LoseLife(int32 Amount = 1);

    // Implement this in BP to show win/lose UI etc.
    UFUNCTION(BlueprintImplementableEvent, Category = "Game")
    void BP_OnGameOver(bool bInPlayerWon);

    // --------------- END GAME UI ---------------

/** Widget class to use for the end-game screen (set in BP or defaults). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<USTEndGameWidget> EndGameWidgetClass;

    /** Runtime instance of the end-game widget. */
    UPROPERTY()
    USTEndGameWidget* EndGameWidgetInstance = nullptr;

protected:
    /** Internal helper to apply rewind score cost every tick. */
    void ApplyReverseScoreCost(float DeltaSeconds);

    /** Internal helper to push ScoreInternal -> GameState->Score. */
    void SyncScoreToGameState();

    /** Internal helper to show end-game popup. */
    void ShowEndGameScreen(bool bInPlayerWon);

    void HandleVictory();
    void HandleDefeat();
};
