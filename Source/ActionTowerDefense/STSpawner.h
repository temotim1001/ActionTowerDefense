// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h" 
#include "STWaveSet.h"         // Include the WaveSet definitions
#include "STSpawner.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveStarted, int32, WaveIndex, int32, TotalWaves);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNextWaveScheduled, float, TimeUntilNextWave);

// Internal runtime state per lane (not exposed to Blueprint)
USTRUCT()
struct FSTLaneRuntimeState
{
    GENERATED_BODY()

    int32 SpawnsDone = 0;
    float NextSpawnTime = 0.0f;
};

UCLASS()
class ACTIONTOWERDEFENSE_API ASTSpawner : public AActor
{
    GENERATED_BODY()

public:
    ASTSpawner();

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWaveStarted OnWaveStarted;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnNextWaveScheduled OnNextWaveScheduled;

    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void RequestNextWaveNow();

    // If you’re calling this from GameController or internally:
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void ScheduleNextWave(float DelaySeconds);

    UFUNCTION(BlueprintCallable, Category = "Spawner|State")
    float GetTimeUntilNextWave() const { return TimeUntilNextWave; }

    UFUNCTION(BlueprintCallable, Category = "Spawner|State")
    int32 GetNumWaves() const
    {
        return (WaveSet ? WaveSet->Waves.Num() : 0);
    }

    UFUNCTION(BlueprintCallable, Category = "Spawner|State")
    bool IsWaveRunning() const { return bWaveRunning; }

    UFUNCTION(BlueprintCallable, Category = "Spawner|State")
    bool HasMoreWaves() const;
    
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;
    
    // Timer used to delay the start of the next wave
    FTimerHandle NextWaveTimerHandle;

    /** Wave configuration for this level */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Config")
    USTWaveSet* WaveSet = nullptr;

    /** Should we loop back to wave 0 after the last wave? */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Config")
    bool bLoopWaves = false;

    /** If true, waves automatically start one after another */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Config")
    bool bAutoStartNextWave = true;

    /** Automatically start at wave 0 in BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Config")
    bool bStartOnBeginPlay = true;

    /** Optional offset added to the spawner's transform when spawning */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Config")
    FVector SpawnOffset = FVector::ZeroVector;

    /** Debug logging */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner|Debug")
    bool bLogSpawns = true;

    /** Current wave index (0-based) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    int32 CurrentWaveIndex = INDEX_NONE;

    /** Time since current wave started */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    float WaveClock = 0.0f;

    /** Are we currently running a wave? */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    bool bWaveRunning = false;

    /** Runtime state per lane */
    UPROPERTY()
    TArray<FSTLaneRuntimeState> LaneStates;

    /** How long until the next wave starts (if scheduled). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner|State")
    float TimeUntilNextWave = 0.0f;

protected:
    bool GetNextWaveIndex(int32& OutNextWaveIndex) const;
    
    /** Returns true when all lanes in the current wave have finished spawning */
    bool IsCurrentWaveFinished() const;

    /** Start the given wave index (0-based) */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StartWave(int32 WaveIndex);

    /** Start the next wave, if available */
    UFUNCTION(BlueprintCallable, Category = "Spawner")
    void StartNextWave();

    /**
     * Spawns a single enemy for lane `LaneIndex`.
     * Override in Blueprint if you want custom spawn locations / FX.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Spawner")
    AActor* SpawnEnemy(const FSTSpawnLane& Lane, int32 LaneIndex);
    virtual AActor* SpawnEnemy_Implementation(const FSTSpawnLane& Lane, int32 LaneIndex);
};
