// Fill out your copyright notice in the Description page of Project Settings.


// STGameController.cpp


#include "STGameController.h"
#include "STGameState.h"
#include "STSpawner.h"
#include "EngineUtils.h"
#include "USTEndGameWidget.h"
#include "Blueprint/UserWidget.h"

ASTGameController::ASTGameController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASTGameController::BeginPlay()
{
    Super::BeginPlay();

    // Cache GameState
    if (UWorld* World = GetWorld())
    {
        STGameStateRef = World->GetGameState<ASTGameState>();
    }

    // Sync internal score from GameState (if any)
    if (STGameStateRef)
    {
        ScoreInternal = static_cast<float>(STGameStateRef->Score);

        // NEW: initialise lives + flags
        STGameStateRef->Lives = StartLives;
        STGameStateRef->bIsGameOver = false;
        STGameStateRef->bPlayerWon = false;
    }

    // Also reset local flags
    bIsGameOver = false;
    bPlayerWon = false;
    NumEnemiesAlive = 0;

    // Bind to spawner events if we have a reference
    if (SpawnerRef)
    {
        SpawnerRef->OnWaveStarted.AddDynamic(
            this, &ASTGameController::HandleWaveStarted);

        SpawnerRef->OnNextWaveScheduled.AddDynamic(
            this, &ASTGameController::HandleNextWaveScheduled);
    }

    // Initial HUD values
    if (SpawnerRef && STGameStateRef)
    {
        const int32 NumWaves = SpawnerRef->GetNumWaves();
        if (NumWaves > 0)
        {
            STGameStateRef->TotalWaves = NumWaves;
        }

        const float InitialDelay = SpawnerRef->GetTimeUntilNextWave();
        if (InitialDelay > 0.0f)
        {
            STGameStateRef->TimeToNextWave = InitialDelay;
        }

        STGameStateRef->CurrentSpeed = CurrentSpeed;
    }
}

void ASTGameController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (STGameStateRef && SpawnerRef)
    {
        STGameStateRef->TimeToNextWave = SpawnerRef->GetTimeUntilNextWave();
    }

    if (!bIsGameOver)
    {
        ApplyReverseScoreCost(DeltaSeconds);
    }

    if (bIsReversing)
    {
        CurrentReverseMeter -= ReverseDrainPerSecond * DeltaSeconds;

        if (CurrentReverseMeter <= 0.f)
        {
            CurrentReverseMeter = 0.f;
            StopReverse(); // auto-stop
        }
    }

    SyncScoreToGameState();
}

ASTGameController* ASTGameController::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ASTGameController> It(World); It; ++It)
    {
        return *It; // assume only one
    }

    return nullptr;
}

void ASTGameController::SetSpeedMode(EGameSpeedMode NewMode)
{
    if (bIsGameOver)
    {
        return;
    }

    SpeedMode = NewMode;

    switch (SpeedMode)
    {
    case EGameSpeedMode::Reverse:  CurrentSpeed = -3.f; break;
    case EGameSpeedMode::Normal:   CurrentSpeed = 1.f; break;
    case EGameSpeedMode::Fast:     CurrentSpeed = 3.f; break;
    case EGameSpeedMode::VeryFast: CurrentSpeed = 5.f; break;
    default:                       CurrentSpeed = 1.f; break;
    }

    // Mirror to GameState for HUD
    if (!STGameStateRef)
    {
        if (UWorld* World = GetWorld())
        {
            STGameStateRef = World->GetGameState<ASTGameState>();
        }
    }

    if (STGameStateRef)
    {
        STGameStateRef->CurrentSpeed = CurrentSpeed;
    }
}

void ASTGameController::StartReverse()
{
    if (bIsGameOver)
        return;

    if (bIsReversing)
        return;

    if (!CanUseReverse())
        return;

    bIsReversing = true;

    // store what we return to afterwards
    PreviousNonReverseSpeed = CurrentSpeed;

    CurrentSpeed = ReverseSpeed;

    // Mirror to GameState
    if (STGameStateRef)
    {
        STGameStateRef->CurrentSpeed = CurrentSpeed;
    }
}

void ASTGameController::StopReverse()
{
    if (!bIsReversing)
        return;

    bIsReversing = false;

    CurrentSpeed = PreviousNonReverseSpeed;

    if (STGameStateRef)
    {
        STGameStateRef->CurrentSpeed = CurrentSpeed;
    }
}

void ASTGameController::Command_SetGameSpeed(float NewSpeed)
{
    if (bIsGameOver)
    {
        return; // ignore UI input after game end
    }
    
    CurrentSpeed = NewSpeed;

    if (!STGameStateRef)
    {
        if (UWorld* World = GetWorld())
        {
            STGameStateRef = World->GetGameState<ASTGameState>();
        }
    }

    if (STGameStateRef)
    {
        STGameStateRef->CurrentSpeed = NewSpeed;
    }
}

void ASTGameController::Command_RequestNextWave()
{
    if (SpawnerRef)
    {
        SpawnerRef->RequestNextWaveNow();
    }
}

void ASTGameController::HandleWaveStarted(int32 WaveIndex, int32 TotalWaves)
{
    if (!STGameStateRef)
    {
        if (UWorld* World = GetWorld())
        {
            STGameStateRef = World->GetGameState<ASTGameState>();
        }
    }

    if (STGameStateRef)
    {
        STGameStateRef->CurrentWaveIndex = WaveIndex;
        STGameStateRef->TotalWaves = TotalWaves;
        STGameStateRef->TimeToNextWave = 0.f;
    }
}

void ASTGameController::HandleNextWaveScheduled(float TimeUntilNextWave)
{
    if (!STGameStateRef)
    {
        if (UWorld* World = GetWorld())
        {
            STGameStateRef = World->GetGameState<ASTGameState>();
        }
    }

    if (STGameStateRef)
    {
        STGameStateRef->TimeToNextWave = TimeUntilNextWave;
    }
}

void ASTGameController::ApplyReverseScoreCost(float DeltaSeconds)
{
    if (CurrentSpeed >= 0.f)
    {
        return; // no cost outside reverse or pause
    }

    if (ReverseScoreCostPerSecond <= 0.f || DeltaSeconds <= 0.f)
    {
        return;
    }

    const float SpeedMagnitude = FMath::Abs(CurrentSpeed); // -3 -> 3
    const float Drain = ReverseScoreCostPerSecond * SpeedMagnitude * DeltaSeconds;

    AddRawScore(-Drain);
}

void ASTGameController::SyncScoreToGameState()
{
    if (!STGameStateRef)
    {
        if (UWorld* World = GetWorld())
        {
            STGameStateRef = World->GetGameState<ASTGameState>();
        }
    }

    if (STGameStateRef)
    {
        // Clamp to non-negative and mirror as int32
        const int32 NewIntScore = FMath::RoundToInt(ScoreInternal);
        STGameStateRef->Score = NewIntScore;
    }
}

void ASTGameController::AddRawScore(float ScoreDelta)
{
    if (ScoreDelta == 0.f)
    {
        return;
    }

    ScoreInternal += ScoreDelta;

    // Never allow negative internal score
    /*
    if (ScoreInternal < 0.f)
    {
        ScoreInternal = 0.f;
    }
    */
}

void ASTGameController::AwardScoreForEnemy(float BaseEnemyScore)
{
    if (BaseEnemyScore <= 0.f)
    {
        return;
    }

    // Rule: no score when rewinding or paused
    if (CurrentSpeed <= 0.f)
    {
        return;
    }

    // Speed-based multiplier: 1x / 3x / 5x etc.
    const float SpeedFactor = CurrentSpeed;
    const float ScoreToAdd = BaseEnemyScore * SpeedFactor;

    AddRawScore(ScoreToAdd);

    CurrentReverseMeter = FMath::Clamp(
        CurrentReverseMeter + ReverseGainPerKill,
        0.f,
        MaxReverseMeter
    );
}

void ASTGameController::NotifyEnemySpawned()
{
    UE_LOG(LogTemp, Warning, TEXT("NotifyEnemySpawned() CALLED"));

    if (bIsGameOver)
    {
        return;
    }

    ++NumEnemiesAlive;

    GEngine->AddOnScreenDebugMessage(
            -1,                     // Key (-1 = create new line each time)
            5.0f,                   // Display time in seconds
            FColor::Yellow,         // Text color
            FString::Printf(TEXT("Number of Enemies: %d"), NumEnemiesAlive) // Message
        );
}

void ASTGameController::NotifyEnemyRemoved(bool bReachedGoal)
{
    if (NumEnemiesAlive > 0)
    {
        --NumEnemiesAlive;
    }

    if (bReachedGoal)
    {
        LoseLife(1);
    }

    // Life loss may have just ended the game
    if (bIsGameOver)
    {
        return;
    }

    // Victory check: no more waves AND no enemies alive
    if (!SpawnerRef || !STGameStateRef)
    {
        return;
    }

    const bool bWaveRunning = SpawnerRef->IsWaveRunning();
    const bool bHasMoreWaves = SpawnerRef->HasMoreWaves();

    const bool bNoMoreWaves = !bWaveRunning && !bHasMoreWaves;
    const bool bNoEnemiesLeft = (NumEnemiesAlive == 0);

    if (bNoMoreWaves && NumEnemiesAlive == 0)
    {
        HandleVictory();
    }

    GEngine->AddOnScreenDebugMessage(
        -1,                     // Key (-1 = create new line each time)
        5.0f,                   // Display time in seconds
        FColor::Yellow,         // Text color
        FString::Printf(TEXT("Number of Enemies: %d"), NumEnemiesAlive) // Message
    );
}

void ASTGameController::LoseLife(int32 Amount)
{
    if (!STGameStateRef || Amount <= 0 || bIsGameOver)
    {
        return;
    }

    STGameStateRef->Lives = FMath::Max(0, STGameStateRef->Lives - Amount);

    if (STGameStateRef->Lives == 0)
    {
        HandleDefeat();
    }
}

void ASTGameController::HandleVictory()
{
    if (bIsGameOver)
    {
        return;
    }

    bIsGameOver = true;
    bPlayerWon = true;

    // Freeze time
    CurrentSpeed = 0.f;
    if (STGameStateRef)
    {
        STGameStateRef->CurrentSpeed = 0.f;
        STGameStateRef->bIsGameOver = true;
        STGameStateRef->bPlayerWon = true;
    }

    ShowEndGameScreen(true);
}

void ASTGameController::HandleDefeat()
{
    if (bIsGameOver)
    {
        return;
    }

    bIsGameOver = true;
    bPlayerWon = false;

    CurrentSpeed = 0.f;
    if (STGameStateRef)
    {
        STGameStateRef->CurrentSpeed = 0.f;
        STGameStateRef->bIsGameOver = true;
        STGameStateRef->bPlayerWon = false;
    }

    ShowEndGameScreen(false);
}

void ASTGameController::ShowEndGameScreen(bool bInPlayerWon)
{
    if (EndGameWidgetInstance || !EndGameWidgetClass)
    {
        // Already shown, or no class set
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    // Get final values from GameState
    int32 FinalScore = 0;
    float FinalTime = 0.f;

    if (!STGameStateRef)
    {
        STGameStateRef = World->GetGameState<ASTGameState>();
    }

    if (STGameStateRef)
    {
        FinalScore = STGameStateRef->Score;
        FinalTime = STGameStateRef->TotalTimeElapsed;
    }

    EndGameWidgetInstance = CreateWidget<USTEndGameWidget>(PC, EndGameWidgetClass);
    if (!EndGameWidgetInstance)
    {
        return;
    }

    EndGameWidgetInstance->AddToViewport(100); // high Z-order
    EndGameWidgetInstance->InitEndScreen(bInPlayerWon, FinalScore, FinalTime);

    // Lock input to UI
    PC->bShowMouseCursor = true;

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(EndGameWidgetInstance->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(InputMode);

    // Optional: hard pause
    // UGameplayStatics::SetGamePaused(World, true);
}
