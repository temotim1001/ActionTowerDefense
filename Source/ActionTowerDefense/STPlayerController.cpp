// Fill out your copyright notice in the Description page of Project Settings.


#include "STPlayerController.h"
#include "TowerBase.h"       // adjust include name to your actual header
#include "AttackTowerBase.h" 
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "USTHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "STGameController.h"
#include "USTTowerActionPanelWidget.h"

ASTPlayerController::ASTPlayerController()
{
    UE_LOG(LogTemp, Warning, TEXT("STPlayerController::Constructor"));

    // Show mouse cursor by default
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ASTPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (TowerActionPanel && SelectedTower)
    {
        FVector2D ScreenPos;
        const FVector WorldLocation = SelectedTower->GetActorLocation();

        if (ProjectWorldLocationToScreen(WorldLocation, ScreenPos))
        {
            // Slight offset so it appears under the tower
            ScreenPos += TowerPanelScreenOffset;

            TowerActionPanel->SetPositionInViewport(ScreenPos, true);
        }
    }
}

void ASTPlayerController::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("STPlayerController::BeginPlay"));

    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    if (HUDWidgetClass)
    {
        HUDWidget = CreateWidget<USTHUDWidget>(this, HUDWidgetClass);
        if (HUDWidget)
        {
            HUDWidget->AddToViewport();
        }
    }

    // --- NEW: create the tower action panel ---
    if (TowerActionPanelClass)
    {
        TowerActionPanel = CreateWidget<USTTowerActionPanelWidget>(this, TowerActionPanelClass);
        if (TowerActionPanel)
        {
            TowerActionPanel->AddToViewport();
            TowerActionPanel->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void ASTPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    check(InputComponent);

    UE_LOG(LogTemp, Warning, TEXT("STPlayerController::SetupInputComponent"));

    InputComponent->BindAction("Select", IE_Pressed, this, &ASTPlayerController::OnSelectPressed);

    // Old bindings, to be deleted
    // Bind via Action Mapping
    //InputComponent->BindAction("Select", IE_Pressed, this, &ASTPlayerController::OnSelectPressed);

    // Also bind directly to the key, bypassing mappings (for debugging)
    //InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASTPlayerController::OnSelectPressed);
}

void ASTPlayerController::OnSelectPressed()
{
    UE_LOG(LogTemp, Log, TEXT("OnSelectPressed fired"));

    FHitResult Hit;
    bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (!bHit)
    {
        UE_LOG(LogTemp, Log, TEXT("No hit under cursor -> clear selection"));
        ClearSelection();
        return;
    }

    AActor* HitActor = Hit.GetActor();
    if (!HitActor)
    {
        UE_LOG(LogTemp, Log, TEXT("Hit result had no actor -> clear selection"));
        ClearSelection();
        return;
    }

    ATowerBase* HitTower = Cast<ATowerBase>(HitActor);

    // 1) Clicked a tower?
    if (HitTower)
    {
        UE_LOG(LogTemp, Log, TEXT("Clicked tower: %s"), *HitTower->GetName());

        // Case A: clicked a player tower -> (re)select it
        if (HitTower->Team == ETowerTeam::Player)
        {
            // Simple toggle behavior
            if (SelectedTower == HitTower)
            {
                UE_LOG(LogTemp, Log, TEXT("Deselected tower: %s"), *HitTower->GetName());
                ClearSelection();
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Selected tower: %s"), *HitTower->GetName());
                SetSelectedTower(HitTower);
            }

            return;
        }

        // Case B: clicked a neutral tower while a player tower is selected -> order capture
        if (HitTower->Team == ETowerTeam::Neutral && SelectedTower)
        {
            AAttackTowerBase* AttackTower = Cast<AAttackTowerBase>(SelectedTower);
            if (AttackTower)
            {
                UE_LOG(LogTemp, Log, TEXT("Issuing capture order: %s -> %s"),
                    *AttackTower->GetName(),
                    *HitTower->GetName());

                AttackTower->OrderCaptureTower(HitTower);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Selected tower is not an AAttackTowerBase"));
            }

            return;
        }

        // Other teams (enemy towers etc.): for now just clear selection
        ClearSelection();
        return;
    }

    // 2) Clicked non-tower actor -> clear selection
    UE_LOG(LogTemp, Log, TEXT("Clicked non-tower actor: %s"), *HitActor->GetName());
    ClearSelection();
}

// Helpers
void ASTPlayerController::SetSelectedTower(ATowerBase* NewTower)
{
    if (SelectedTower == NewTower)
        return;

    // Turn off selection on previous tower
    if (SelectedTower)
    {
        SelectedTower->SetSelected(false);
    }

    SelectedTower = NewTower;

    // Turn on selection on new tower
    if (SelectedTower)
    {
        SelectedTower->SetSelected(true);
    }

    // --- NEW: update the tower action panel ---
    if (TowerActionPanel)
    {
        TowerActionPanel->SetBoundTower(SelectedTower);

        if (SelectedTower)
        {
            TowerActionPanel->SetVisibility(ESlateVisibility::Visible);

            // Snap to correct position immediately
            FVector2D ScreenPos;
            const FVector WorldLocation = SelectedTower->GetActorLocation();
            if (ProjectWorldLocationToScreen(WorldLocation, ScreenPos))
            {
                ScreenPos += TowerPanelScreenOffset;
                TowerActionPanel->SetPositionInViewport(ScreenPos, true);
            }
        }
        else
        {
            TowerActionPanel->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void ASTPlayerController::ClearSelection()
{
    SetSelectedTower(nullptr);
}

void ASTPlayerController::RequestSpeedMode(EGameSpeedMode NewMode)
{
    if (ASTGameController* GC = ASTGameController::Get(this))
    {
        GC->SetSpeedMode(NewMode);
    }
}

EGameSpeedMode ASTPlayerController::GetCurrentSpeedMode() const
{
    if (const ASTGameController* GC = ASTGameController::Get(this))
    {
        return GC->SpeedMode;
    }

    return EGameSpeedMode::Normal;
}