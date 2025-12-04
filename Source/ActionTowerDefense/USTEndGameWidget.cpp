// Fill out your copyright notice in the Description page of Project Settings.


#include "USTEndGameWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"

void USTEndGameWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (RestartButton)
    {
        RestartButton->OnClicked.AddDynamic(this, &USTEndGameWidget::HandleRestartClicked);
    }

    if (QuitButton)
    {
        QuitButton->OnClicked.AddDynamic(this, &USTEndGameWidget::HandleQuitClicked);
    }
}

void USTEndGameWidget::InitEndScreen(bool bPlayerWon, int32 FinalScore, float FinalTimeSeconds)
{
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(bPlayerWon ? TEXT("Victory") : TEXT("Defeat")));
    }

    if (ScoreText)
    {
        ScoreText->SetText(FText::FromString(
            FString::Printf(TEXT("Score: %d"), FinalScore)));
    }

    if (TimeText)
    {
        TimeText->SetText(FText::FromString(
            FString::Printf(TEXT("Time: %s"), *FormatSeconds(FinalTimeSeconds))));
    }
}

FString USTEndGameWidget::FormatSeconds(float Seconds) const
{
    const int32 Total = FMath::Max(0, static_cast<int32>(Seconds));
    const int32 Minutes = Total / 60;
    const int32 Secs = Total % 60;
    return FString::Printf(TEXT("%02d:%02d"), Minutes, Secs);
}

void USTEndGameWidget::HandleRestartClicked()
{
    if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName())); // reload current level
    }
}

void USTEndGameWidget::HandleQuitClicked()
{
    UGameplayStatics::OpenLevel(this, FName("MainMenu")); // or whatever later
} 