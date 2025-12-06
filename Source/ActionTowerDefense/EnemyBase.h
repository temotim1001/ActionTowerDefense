// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageableTarget.h" 
#include "EnemyBase.generated.h"

class UStaticMeshComponent;
class USplineComponent;
class USTEnemyLifeComponent;

UCLASS()
class ACTIONTOWERDEFENSE_API ASTEnemyBase : public AActor, public IDamageableTarget          
{
    GENERATED_BODY()

public:
    ASTEnemyBase();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USTEnemyLifeComponent* LifeComponent;

    // --- Stats / Health ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Stats")
    float CurrentHealth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
    float BaseScoreValue = 100.f;    // ← NEW: tweak per enemy type in BP

    // --- Movement along spline/path ---
    /** Units per second along the spline at 1x game speed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 300.f;

    /** If true, enemy rotates to face along spline direction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bOrientRotationToSpline = true;

    /** How far along the spline we are (in distance units). */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement")
    float DistanceAlongSpline = 0.f;

    /** The actor that owns the spline (e.g. BP_Path), set by spawner. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Movement")
    AActor* SplineActor = nullptr;

    /** Cached spline component from SplineActor. */
    UPROPERTY(Transient)
    USplineComponent* CachedSpline = nullptr;

    void UpdateMovement(float DeltaSeconds);
    void HandleReachedGoal();

    // Handle taking damage
    void ApplyDamage(float Amount);

    // IDamageableTarget interface implementation
    virtual void ReceiveTowerDamage_Implementation(float Amount) override;

public:
    /** Called by spawner after spawn to assign the path/spline. */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetSplineActor(AActor* InSplineActor);

    /** Kill this enemy, notify GameController via LifeComponent, then destroy. */
    UFUNCTION(BlueprintCallable, Category = "Enemy")
    void KillEnemy(bool bReachedGoal);

    // Optional hooks for VFX / SFX / BP logic
    UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
    void BP_OnReachedGoal();

    UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
    void BP_OnKilled();
};
