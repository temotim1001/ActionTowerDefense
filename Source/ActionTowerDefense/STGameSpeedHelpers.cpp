// Fill out your copyright notice in the Description page of Project Settings.


#include "STGameSpeedHelpers.h"
#include "STGameState.h"

float FSTGameSpeedHelpers::GetGameSpeed(const UObject* WorldContextObject)
{
    if (ASTGameState* GS = ASTGameState::Get(WorldContextObject))
    {
        return GS->GetCurrentSpeed();
    }

    // Fallback if no GameState (e.g., in menus / tests)
    return 1.f;
}

float FSTGameSpeedHelpers::GetPositiveSpeed(const UObject* WorldContextObject)
{
    const float Speed = GetGameSpeed(WorldContextObject);
    return (Speed > 0.f) ? Speed : 0.f;
}

float FSTGameSpeedHelpers::GetScaledDeltaSeconds(const UObject* WorldContextObject, float DeltaSeconds)
{
    const float Speed = GetGameSpeed(WorldContextObject);
    return DeltaSeconds * Speed;
}
