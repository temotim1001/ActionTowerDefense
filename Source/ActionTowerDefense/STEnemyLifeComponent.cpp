// Fill out your copyright notice in the Description page of Project Settings.


#include "STEnemyLifeComponent.h"
#include "GameFramework/Actor.h"

USTEnemyLifeComponent::USTEnemyLifeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USTEnemyLifeComponent::MarkEnemyRemoved(bool bReachedGoal)
{
    if (bHasBeenRemoved)
    {
        return;
    }

    bHasBeenRemoved = true;

    AActor* OwnerActor = GetOwner();

    UE_LOG(LogTemp, Log,
        TEXT("LifeComp: MarkEnemyRemoved called on %s, ReachedGoal=%s"),
        OwnerActor ? *OwnerActor->GetName() : TEXT("nullptr"),
        bReachedGoal ? TEXT("true") : TEXT("false"));

    OnEnemyRemoved.Broadcast(OwnerActor, bReachedGoal);
}

