// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UObject;

/**
 * Small helper struct for querying game speed and scaled delta time.
 * Centralizes all interaction with ASTGameState::Get().
 */
struct FSTGameSpeedHelpers
{
    /** Get the current game speed (-3, 1, 3, 5, etc.). Defaults to 1 if no GameState. */
    static float GetGameSpeed(const UObject* WorldContextObject);

    /** Get the current game speed, but clamped to be non-negative. */
    static float GetPositiveSpeed(const UObject* WorldContextObject);

    /** Get DeltaSeconds scaled by current game speed. */
    static float GetScaledDeltaSeconds(const UObject* WorldContextObject, float DeltaSeconds);
};
