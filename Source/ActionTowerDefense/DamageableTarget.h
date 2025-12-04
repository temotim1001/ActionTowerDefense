// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageableTarget.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class ACTIONTOWERDEFENSE_API UDamageableTarget : public UInterface
{
    GENERATED_BODY()
};

class ACTIONTOWERDEFENSE_API IDamageableTarget
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Damage")
    void ReceiveTowerDamage(float Amount);
};
