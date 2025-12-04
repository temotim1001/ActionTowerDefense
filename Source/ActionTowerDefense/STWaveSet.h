// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"          // Needed for AActor
#include "STWaveSet.generated.h"

/**
 * One lane within a wave (e.g. "top path", "mid path", etc.)
 */
USTRUCT(BlueprintType)
struct FSTSpawnLane
{
    GENERATED_BODY()

    // Which enemy to spawn on this lane
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> EnemyClass = nullptr;

    // How many enemies to spawn on this lane for this wave
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NumToSpawn = 0;

    // Time (sec) from wave start to first spawn on this lane
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FirstSpawnDelay = 0.0f;

    // Base interval (sec) between spawns
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Interval = 1.0f;

    // Random +/- jitter applied to each interval
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Jitter = 0.0f;
};

/**
 * A single wave containing multiple lanes.
 */
USTRUCT(BlueprintType)
struct FSTWave
{
    GENERATED_BODY()

    // Optional: name or label for the wave
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName WaveName = NAME_None;

    // How long to wait before this wave starts
    // - Wave 0: delay from game start
    // - Wave N: delay after previous wave finishes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float TimeBeforeWave = 0.0f;

    // Lanes in this wave
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FSTSpawnLane> Lanes;
};

/**
 * Data asset that holds all waves for a level.
 * Create one asset per level and assign it on the spawner.
 */
UCLASS(BlueprintType)
class ACTIONTOWERDEFENSE_API USTWaveSet : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FSTWave> Waves;
};
