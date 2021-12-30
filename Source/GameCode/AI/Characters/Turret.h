// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameCodeTypes.h"
#include "Turret.generated.h"

UENUM(BlueprintType)
enum class ETurretState : uint8
{
	Searching,
	Firing
};

class UWeaponBarellComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDestroyedEventSignature);

UCLASS()
class GAMECODE_API ATurret : public APawn
{
	GENERATED_BODY()

public:
	ATurret();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void OnCurrentTargetSet();

	virtual FVector GetPawnViewLocation() const override;

	virtual FRotator GetViewRotation() const override;

	virtual void PossessedBy(AController* NewController) override;

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	bool IsAlive() const;

	UPROPERTY(BlueprintAssignable)
	FOnDestroyedEventSignature OnDestroyedEvent;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget)
	AActor* CurrentTarget = nullptr;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* TurretBaseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* TurretBarellComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWeaponBarellComponent* WeaponBarell;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float BaseSearchingRotationRate = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float BaseFiringInterpSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float BarellPitchRotationRate = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", meta = (ClampMin = -90.0f, UIMin = -90.0f, UIMax = 90.0f, ClampMax = 90.0f))
	float MaxBarellPitchAngle = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f, UIMax = 90.0f, ClampMax = 90.0f))
	float MinBarellPitchAngle = -30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Fire", meta = (ClampMin = 1.0f, UIMin = 1.0f))
	float RateOfFire = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Fire", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float BulletSpreadAngle = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Fire", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float FireDelayTime = 3.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turret parameters | Team")
	ETeams Team = ETeams::Enemy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHealth = 100.0f;

private:
	void SetCurrentTurretState(ETurretState NewState);
	ETurretState CurrentTurretState = ETurretState::Searching;

	void SearchingMovement(float DeltaTime);
	void FiringMovement(float DeltaTime);

	UFUNCTION()
	void OnRep_CurrentTarget();

	FTimerHandle ShotTimer;

	float GetFireInterval() const;
	void MakeShot();

	UFUNCTION()
	void OnTakeAnyDamageCallback(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	float Health = 0.0f;
};
