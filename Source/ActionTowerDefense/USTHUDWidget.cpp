// Fill out your copyright notice in the Description page of Project Settings.


#include "USTHUDWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

#include "STGameState.h"
#include "STGameController.h"

void USTHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Cache GameState and GameController
    if (UWorld* World = GetWorld())
    {
        GameStateRef = World->GetGameState<ASTGameState>();

        TArray<AActor*> Controllers;
        UGameplayStatics::GetAllActorsOfClass(World, ASTGameController::StaticClass(), Controllers);
        if (Controllers.Num() > 0)
        {
            GameControllerRef = Cast<ASTGameController>(Controllers[0]);
        }
    }

    // Bind buttons
    if (SpeedReverseButton)
    {
        SpeedReverseButton->OnPressed.AddDynamic(this, &USTHUDWidget::HandleReversePressed);
        SpeedReverseButton->OnReleased.AddDynamic(this, &USTHUDWidget::HandleReverseReleased);
    }
    if (Speed1xButton)
    {
        Speed1xButton->OnClicked.AddDynamic(this, &USTHUDWidget::HandleSpeed1xClicked);
    }
    if (Speed3xButton)
    {
        Speed3xButton->OnClicked.AddDynamic(this, &USTHUDWidget::HandleSpeed3xClicked);
    }
    if (Speed5xButton)
    {
        Speed5xButton->OnClicked.AddDynamic(this, &USTHUDWidget::HandleSpeed5xClicked);
    }
    if (CallNextWaveButton)
    {
        CallNextWaveButton->OnClicked.AddDynamic(this, &USTHUDWidget::HandleCallNextWaveClicked);
    }
}

void USTHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // If GameState disappeared (map reload etc.), try to reacquire
    if (!GameStateRef)
    {
        if (UWorld* World = GetWorld())
        {
            GameStateRef = World->GetGameState<ASTGameState>();
        }
    }

    // If GameController disappeared, try to reacquire
    if (!GameControllerRef)
    {
        if (UWorld* World = GetWorld())
        {
            TArray<AActor*> Controllers;
            UGameplayStatics::GetAllActorsOfClass(World, ASTGameController::StaticClass(), Controllers);
            if (Controllers.Num() > 0)
            {
                GameControllerRef = Cast<ASTGameController>(Controllers[0]);
            }
        }
    }

    RefreshFromGameState();
    RefreshSpeedButtons();
}

void USTHUDWidget::RefreshFromGameState()
{
    if (!GameStateRef) return;

    if (ScoreText)
    {
        const FString S = FString::Printf(TEXT("Score: %d"), GameStateRef->Score);
        ScoreText->SetText(FText::FromString(S));
    }

    if (LivesText)
    {
        const FString S = FString::Printf(TEXT("Lives: %d"), GameStateRef->Lives);
        LivesText->SetText(FText::FromString(S));
    }

    if (WaveText)
    {
        const int32 WaveIndexOneBased = GameStateRef->CurrentWaveIndex + 1;
        const FString S = FString::Printf(TEXT("Wave: %d / %d"), WaveIndexOneBased, GameStateRef->TotalWaves);
        WaveText->SetText(FText::FromString(S));
    }

    if (NextWaveTimeText)
    {
        const FString S = FString::Printf(TEXT("Next wave: %s"), *FormatSeconds(GameStateRef->TimeToNextWave));
        NextWaveTimeText->SetText(FText::FromString(S));
    }

    if (TotalTimeText)
    {
        const FString S = FString::Printf(TEXT("Time: %s"), *FormatSeconds(GameStateRef->TotalTimeElapsed));
        TotalTimeText->SetText(FText::FromString(S));
    }
}

FString USTHUDWidget::FormatSeconds(float Seconds) const
{
    const int32 Total = FMath::Max(0, static_cast<int32>(Seconds));
    const int32 Minutes = Total / 60;
    const int32 Secs = Total % 60;
    return FString::Printf(TEXT("%02d:%02d"), Minutes, Secs);
}

// --- button handlers (just forward to GameController) ---

void USTHUDWidget::HandleSpeedReverseClicked()
{
    if (GameControllerRef)
    {
        GameControllerRef->SetSpeedMode(EGameSpeedMode::Reverse);
    }
}

void USTHUDWidget::HandleSpeed1xClicked()
{
    if (GameControllerRef)
    {
        GameControllerRef->SetSpeedMode(EGameSpeedMode::Normal);
    }
}

void USTHUDWidget::HandleSpeed3xClicked()
{
    if (GameControllerRef)
    {
        GameControllerRef->SetSpeedMode(EGameSpeedMode::Fast);
    }
}

void USTHUDWidget::HandleSpeed5xClicked()
{
    if (GameControllerRef)
    {
        GameControllerRef->SetSpeedMode(EGameSpeedMode::VeryFast);
    }
}

void USTHUDWidget::HandleCallNextWaveClicked()
{
    if (GameControllerRef)
    {
        GameControllerRef->Command_RequestNextWave();
    }
}

void USTHUDWidget::RefreshSpeedButtons()
{
    if (!GameControllerRef)
    {
        return;
    }

    const EGameSpeedMode Mode = GameControllerRef->SpeedMode;

    auto SetButtonOpacity = [](UButton* Button, bool bActive)
        {
            if (Button)
            {
                Button->SetRenderOpacity(bActive ? 1.0f : 0.4f);
            }
        };

    if (SpeedReverseButton)
    {
        SpeedReverseButton->SetIsEnabled(
            GameControllerRef->CurrentReverseMeter > 0.1f
        );
    }

    SetButtonOpacity(SpeedReverseButton, Mode == EGameSpeedMode::Reverse);
    SetButtonOpacity(Speed1xButton, Mode == EGameSpeedMode::Normal);
    SetButtonOpacity(Speed3xButton, Mode == EGameSpeedMode::Fast);
    SetButtonOpacity(Speed5xButton, Mode == EGameSpeedMode::VeryFast);
}

void USTHUDWidget::HandleReversePressed()
{
    if (GameControllerRef)
    {
        GameControllerRef->StartReverse();
    }
}

void USTHUDWidget::HandleReverseReleased()
{
    if (GameControllerRef)
    {
        GameControllerRef->StopReverse();
    }
}