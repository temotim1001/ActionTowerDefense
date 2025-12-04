// Fill out your copyright notice in the Description page of Project Settings.


#include "STSpawner.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "STGameController.h"

ASTSpawner::ASTSpawner()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASTSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (bStartOnBeginPlay && WaveSet && WaveSet->Waves.Num() > 0)
    {
        const float FirstDelay = WaveSet->Waves[0].TimeBeforeWave;
        ScheduleNextWave(FirstDelay);
    }
}

void ASTSpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!WaveSet)
    {
        return;
    }

    // Read global speed from GameController
    float Speed = 1.f;
    if (ASTGameController* GC = ASTGameController::Get(this))
    {
        Speed = GC->GetGameSpeed();   // -3, 1, 3, 5
    }

    const float PositiveSpeed = (Speed > 0.f) ? Speed : 0.f;

    // --- PHASE 1: waiting for next wave to start ---
    if (!bWaveRunning)
    {
        if (TimeUntilNextWave > 0.f && PositiveSpeed > 0.f)
        {
            TimeUntilNextWave = FMath::Max(0.f, TimeUntilNextWave - DeltaTime * PositiveSpeed);

            if (TimeUntilNextWave <= 0.f)
            {
                StartNextWave();
            }
        }

        // Not currently in a wave, so we’re done
        return;
    }

    // --- PHASE 2: a wave is running, spawn enemies inside it ---

    if (!WaveSet->Waves.IsValidIndex(CurrentWaveIndex))
    {
        bWaveRunning = false;
        return;
    }

    // No new spawns while rewinding or paused
    if (Speed <= 0.f)
    {
        return;
    }

    const float EffectiveDelta = DeltaTime * Speed;

    const FSTWave& Wave = WaveSet->Waves[CurrentWaveIndex];

    WaveClock += EffectiveDelta;

    // For each lane, check whether we should spawn more enemies
    for (int32 LaneIndex = 0; LaneIndex < Wave.Lanes.Num(); ++LaneIndex)
    {
        const FSTSpawnLane& Lane = Wave.Lanes[LaneIndex];
        FSTLaneRuntimeState& State = LaneStates[LaneIndex];

        if (State.SpawnsDone >= Lane.NumToSpawn)
        {
            continue;
        }

        while (State.SpawnsDone < Lane.NumToSpawn &&
            WaveClock >= State.NextSpawnTime)
        {
            SpawnEnemy(Lane, LaneIndex);
            State.SpawnsDone++;

            const float RandomJitter =
                (Lane.Jitter != 0.0f)
                ? FMath::FRandRange(-Lane.Jitter, Lane.Jitter)
                : 0.0f;

            const float Step = FMath::Max(Lane.Interval + RandomJitter, 0.01f);
            const float Base = FMath::Max(WaveClock, State.NextSpawnTime);
            State.NextSpawnTime = Base + Step;
        }
    }

    if (IsCurrentWaveFinished())
    {
        bWaveRunning = false;

        if (bAutoStartNextWave && WaveSet)
        {
            int32 NextWaveIndex = INDEX_NONE;
            if (!GetNextWaveIndex(NextWaveIndex))
            {
                if (bLogSpawns)
                {
                    UE_LOG(LogTemp, Log, TEXT("STSpawner: No more waves."));
                }
                return;
            }

            const float Delay = WaveSet->Waves[NextWaveIndex].TimeBeforeWave;
            ScheduleNextWave(Delay);
        }
    }
}

void ASTSpawner::StartWave(int32 WaveIndex)
{
    if (!WaveSet || !WaveSet->Waves.IsValidIndex(WaveIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("STSpawner: Invalid wave index %d"), WaveIndex);
        bWaveRunning = false;
        return;
    }

    CurrentWaveIndex = WaveIndex;
    WaveClock = 0.0f;
    bWaveRunning = true;

    const FSTWave& Wave = WaveSet->Waves[CurrentWaveIndex];

    LaneStates.SetNum(Wave.Lanes.Num());

    // Initialize lane runtime state
    for (int32 LaneIndex = 0; LaneIndex < Wave.Lanes.Num(); ++LaneIndex)
    {
        const FSTSpawnLane& Lane = Wave.Lanes[LaneIndex];
        FSTLaneRuntimeState& State = LaneStates[LaneIndex];

        State.SpawnsDone = 0;
        State.NextSpawnTime = Lane.FirstSpawnDelay;
    }

    if (bLogSpawns)
    {
        UE_LOG(LogTemp, Log, TEXT("STSpawner: Starting wave %d (%s)"),
            CurrentWaveIndex,
            *Wave.WaveName.ToString());
    }

    // Tell the outside world a new wave started
    OnWaveStarted.Broadcast(CurrentWaveIndex, WaveSet->Waves.Num());
}

void ASTSpawner::StartNextWave()
{
    int32 NextWaveIndex = INDEX_NONE;
    if (!GetNextWaveIndex(NextWaveIndex))
    {
        if (bLogSpawns)
        {
            UE_LOG(LogTemp, Log, TEXT("STSpawner: No more waves."));
        }
        return;
    }

    StartWave(NextWaveIndex);
}

bool ASTSpawner::IsCurrentWaveFinished() const
{
    if (!WaveSet || !WaveSet->Waves.IsValidIndex(CurrentWaveIndex))
    {
        return true;
    }

    const FSTWave& Wave = WaveSet->Waves[CurrentWaveIndex];

    for (int32 LaneIndex = 0; LaneIndex < Wave.Lanes.Num(); ++LaneIndex)
    {
        const FSTSpawnLane& Lane = Wave.Lanes[LaneIndex];
        const FSTLaneRuntimeState& State = LaneStates[LaneIndex];

        if (State.SpawnsDone < Lane.NumToSpawn)
        {
            return false;
        }
    }

    return true;
}

AActor* ASTSpawner::SpawnEnemy_Implementation(const FSTSpawnLane& Lane, int32 LaneIndex)
{
    if (!GetWorld())
    {
        return nullptr;
    }

    if (!Lane.EnemyClass)
    {
        if (bLogSpawns)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("STSpawner: Lane %d has no EnemyClass set."), LaneIndex);
        }
        return nullptr;
    }

    const FTransform SpawnTransform = FTransform(
        GetActorRotation(),
        GetActorLocation() + SpawnOffset,
        FVector::OneVector
    );

    AActor* Spawned = GetWorld()->SpawnActor<AActor>(
        Lane.EnemyClass,
        SpawnTransform
    );

    if (Spawned)
    {
        // 🔹 NEW: tell the GameController an enemy was spawned (pure C++)
        if (ASTGameController* GC = ASTGameController::Get(this))
        {
            GC->NotifyEnemySpawned();
        }

        if (bLogSpawns)
        {
            UE_LOG(LogTemp, Log,
                TEXT("STSpawner: Spawned %s on lane %d (wave %d)"),
                *Spawned->GetName(), LaneIndex, CurrentWaveIndex);
        }
    }

    return Spawned;
}

void ASTSpawner::RequestNextWaveNow()
{
    if (!WaveSet)
    {
        return;
    }

    // Cancel any pending delay
    TimeUntilNextWave = 0.0f;

    // Only start if nothing is currently running
    if (!bWaveRunning)
    {
        StartNextWave();
    }
}

void ASTSpawner::ScheduleNextWave(float DelaySeconds)
{
    if (!WaveSet)
    {
        return;
    }

    // Store delay – Tick will count this down with game speed
    TimeUntilNextWave = DelaySeconds;

    if (bLogSpawns)
    {
        UE_LOG(LogTemp, Log, TEXT("STSpawner: Next wave scheduled in %.2f seconds"), DelaySeconds);
    }

    // Inform GameController / HUD
    OnNextWaveScheduled.Broadcast(DelaySeconds);
}

bool ASTSpawner::GetNextWaveIndex(int32& OutNextWaveIndex) const
{
    if (!WaveSet || WaveSet->Waves.Num() == 0)
    {
        return false;
    }

    int32 NextWaveIndex = CurrentWaveIndex + 1;

    if (!WaveSet->Waves.IsValidIndex(NextWaveIndex))
    {
        if (bLoopWaves)
        {
            NextWaveIndex = 0;
        }
        else
        {
            return false;
        }
    }

    OutNextWaveIndex = NextWaveIndex;
    return true;
}

bool ASTSpawner::HasMoreWaves() const
{
    int32 DummyIndex;
    return GetNextWaveIndex(DummyIndex);
}

