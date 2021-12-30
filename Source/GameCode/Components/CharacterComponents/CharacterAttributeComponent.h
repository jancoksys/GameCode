// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Subsystems/SaveSubsystem/SaveSubsystemInterface.h"
#include "CharacterAttributeComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnDeathEventSignature);
DECLARE_MULTICAST_DELEGATE_OneParam(FOutOfStaminaEventSignature, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignature, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaminaChangedSignature, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOxygenChangedSignature, float);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMECODE_API UCharacterAttributeComponent : public UActorComponent, public ISaveSubsystemInterface
{
	GENERATED_BODY()

public:		
	UCharacterAttributeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	bool IsAlive() { return Health > 0.0f; }

	FOnDeathEventSignature OnDeathEvent;
	FOutOfStaminaEventSignature OutOfStaminaEvent;
	FOnHealthChangedSignature OnHealthChangedEvent;
	FOnStaminaChangedSignature OnStaminaChangedEvent;
	FOnOxygenChangedSignature OnOxygenChangedEvent;

	float GetHealthPercent() const;

	void AddHealth(float HealthToAdd);
	void RestoreFullStamina();

	virtual void OnLevelDeserialized_Implementation() override;
protected:	
	virtual void BeginPlay() override;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (UIMin = 0.0f))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxStamina = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float StaminaRestoreVelocity = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SprintStaminaConsumptionVelocity = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (UIMin = 0.0f))
	float MaxOxygen = 50.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (UIMin = 0.0f))
	float OxygenRestoreVelocity = 15.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (UIMin = 0.0f))
	float SwimOxygenConsumptionVelocity = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (UIMin = 0.0f))
	float OutOfOxygenDamage = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Oxygen", meta = (UIMin = 0.0f))
	float OutOfOxygenDamageInterval = 2.0f;

private:
	UPROPERTY(ReplicatedUsing=OnRep_Health, SaveGame)
	float Health = 0.0f;

	UFUNCTION()
	void OnRep_Health();

	void OnHealthChanged();

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	void DebugDrawAttributes();
	float DebugTextInterval = 8.0f;
#endif

	UFUNCTION()
	void OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	TWeakObjectPtr<class AGCBaseCharacter> CachedBaseCharacterOwner;
	class UGCBaseCharacterMovementComponent* GCBaseCharacterMovementComponent;

	void UpdateStaminaValue(float DeltaTime);
	float CurrentStamina;	

	float Oxygen = 0.0f;
	void UpdateOxygenValue(float DeltaTime);
	FTimerHandle OutOfOxygenDamageTimer;
	void TakeOutOfOxygenDamage();
};
