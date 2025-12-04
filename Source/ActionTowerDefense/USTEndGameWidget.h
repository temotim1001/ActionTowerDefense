// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "USTEndGameWidget.generated.h"

class UTextBlock;
class UButton;

UCLASS()
class ACTIONTOWERDEFENSE_API USTEndGameWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Call from GameController after creating the widget
    UFUNCTION()
    void InitEndScreen(bool bPlayerWon, int32 FinalScore, float FinalTimeSeconds);

protected:
    // Bind to widgets in the UMG designer (in the BP child)
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ScoreText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TimeText;

    // Optional buttons (hook up in BP if you want)
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* RestartButton;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* QuitButton;

    virtual void NativeConstruct() override;

private:
    FString FormatSeconds(float Seconds) const;

    UFUNCTION()
    void HandleRestartClicked();

    UFUNCTION()
    void HandleQuitClicked();
};
