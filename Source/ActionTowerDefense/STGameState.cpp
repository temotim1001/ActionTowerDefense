// Fill out your copyright notice in the Description page of Project Settings.


#include "STGameState.h"
#include "STGameController.h"
#include "Engine/World.h"


// Sets default values
ASTGameState::ASTGameState()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASTGameState::BeginPlay()
{
    Super::BeginPlay();
}

void ASTGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    TotalTimeElapsed += DeltaSeconds;
    // Do NOT modify TimeToNextWave here anymore
}

ASTGameState* ASTGameState::Get(const UObject* WorldContextObject)
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

    return World->GetGameState<ASTGameState>();
}

