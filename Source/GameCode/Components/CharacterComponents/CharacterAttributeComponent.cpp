// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterAttributeComponent.h"
#include "../../Characters/GCBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "../../Subsystems/DebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameCodeTypes.h"
#include "../../Components/MovementComponents/GCBaseCharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UCharacterAttributeComponent::UCharacterAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UCharacterAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCharacterAttributeComponent, Health);
}

void UCharacterAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(MaxHealth > 0.0f, TEXT("UCharacterAttributeComponent::BeginPlay max health can't be equal to 0"));
	checkf(GetOwner()->IsA<AGCBaseCharacter>(), TEXT("UCharacterAttributeComponent::BeginPlay UCharacterAttributeComponent can be used only with AGCBaseCharacter"));
	CachedBaseCharacterOwner = StaticCast<AGCBaseCharacter*>(GetOwner());
	GCBaseCharacterMovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(CachedBaseCharacterOwner->GetMovementComponent());

	Health = MaxHealth;
	if (GetOwner()->HasAuthority())
	{
		CachedBaseCharacterOwner->OnTakeAnyDamage.AddDynamic(this, &UCharacterAttributeComponent::OnTakeAnyDamage);
	}	

	CurrentStamina = MaxStamina;	
	Oxygen = MaxOxygen;
}

void UCharacterAttributeComponent::OnRep_Health()
{
	OnHealthChanged();
}

void UCharacterAttributeComponent::OnHealthChanged()
{
	if (OnHealthChangedEvent.IsBound())
	{
		OnHealthChangedEvent.Broadcast(GetHealthPercent());
	}

	if (Health <= 0.0f)
	{
		if (OnDeathEvent.IsBound())
		{
			OnDeathEvent.Broadcast();
		}
	}
}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
void UCharacterAttributeComponent::DebugDrawAttributes()
{
	UDebugSubsystem* DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
	if (!DebugSubsystem->IsCategoryEnabled(DebugCategoryCharacterAttributes))
	{
		return;
	}

	FVector HealthBarLocation = CachedBaseCharacterOwner->GetActorLocation() + (CachedBaseCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.0f) * FVector::UpVector;
	DrawDebugString(GetWorld(), HealthBarLocation, FString::Printf(TEXT("Health: %.2f"), Health), nullptr, FColor::Green, 0.0f, true);

	FVector StaminaBarLocation = HealthBarLocation - DebugTextInterval * FVector::UpVector;
	DrawDebugString(GetWorld(), StaminaBarLocation, FString::Printf(TEXT("Stamina: %.2f"), CurrentStamina), nullptr, FColor::Blue, 0.0f, true);

	FVector OxygenBarLocation = StaminaBarLocation - DebugTextInterval * FVector::UpVector;
	DrawDebugString(GetWorld(), OxygenBarLocation, FString::Printf(TEXT("Oxygen: %.2f"), Oxygen), nullptr, FColor::Purple, 0.0f, true);
}
#endif

void UCharacterAttributeComponent::OnTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsAlive())
	{
		return;
	}	
	
	UE_LOG(LogDamage, Warning, TEXT("UCharacterAttributeComponent::OnTakeAnyDamage %s recieved %.2f amount of damage from %s"), *CachedBaseCharacterOwner->GetName(), Damage, *DamageCauser->GetName());
	Health = FMath::Clamp(Health - Damage, 0.0f, MaxHealth);
	OnHealthChanged();
}

void UCharacterAttributeComponent::UpdateStaminaValue(float DeltaTime)
{
	if (CurrentStamina == 0.0f)
	{		
		if (OutOfStaminaEvent.IsBound())
		{
			OutOfStaminaEvent.Broadcast(true);
		}
		CachedBaseCharacterOwner->GetBaseCharacterMovementComponent()->StopSprint();
		CachedBaseCharacterOwner->OnSprintEnd();
	}

	if (!GCBaseCharacterMovementComponent->IsSprinting())
	{
		CurrentStamina += StaminaRestoreVelocity * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
	}
	else
	{
		CurrentStamina -= SprintStaminaConsumptionVelocity * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, MaxStamina);
	}

	if (OnStaminaChangedEvent.IsBound())
	{
		OnStaminaChangedEvent.Broadcast(CurrentStamina / MaxStamina);
	}

	if (CurrentStamina == MaxStamina && GCBaseCharacterMovementComponent->IsOutOfStamina())
	{		
		if (OutOfStaminaEvent.IsBound())
		{
			OutOfStaminaEvent.Broadcast(false);
		}
	}	
}

void UCharacterAttributeComponent::UpdateOxygenValue(float DeltaTime)
{
	if (Oxygen == 0.0f && !GetWorld()->GetTimerManager().IsTimerActive(OutOfOxygenDamageTimer))
	{
		GetWorld()->GetTimerManager().SetTimer(OutOfOxygenDamageTimer, this, &UCharacterAttributeComponent::TakeOutOfOxygenDamage, OutOfOxygenDamageInterval, true, 0.0f);				
	}
	
	if (!CachedBaseCharacterOwner->IsSwimmingUnderWater())
	{
		GetWorld()->GetTimerManager().ClearTimer(OutOfOxygenDamageTimer);
		Oxygen += OxygenRestoreVelocity * DeltaTime;
		Oxygen = FMath::Clamp(Oxygen, 0.0f, MaxOxygen);
	}
	else
	{
		Oxygen -= SwimOxygenConsumptionVelocity * DeltaTime;
		Oxygen = FMath::Clamp(Oxygen, 0.0f, MaxOxygen);
	}

	if (OnOxygenChangedEvent.IsBound())
	{
		OnOxygenChangedEvent.Broadcast(Oxygen / MaxOxygen);
	}
}

void UCharacterAttributeComponent::TakeOutOfOxygenDamage()
{	
	APhysicsVolume* Volume = CachedBaseCharacterOwner->GetCharacterMovement()->GetPhysicsVolume();	
	OnTakeAnyDamage(CachedBaseCharacterOwner.Get(), OutOfOxygenDamage, nullptr, nullptr, (AActor*)Volume);
}

void UCharacterAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateStaminaValue(DeltaTime);
	UpdateOxygenValue(DeltaTime);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	DebugDrawAttributes();
#endif
}

float UCharacterAttributeComponent::GetHealthPercent() const
{
	return Health / MaxHealth;
}

void UCharacterAttributeComponent::AddHealth(float HealthToAdd)
{
	Health = FMath::Clamp(Health + HealthToAdd, 0.0f, MaxHealth);
	OnHealthChanged();
}

void UCharacterAttributeComponent::RestoreFullStamina()
{
	CurrentStamina = MaxStamina;
}

void UCharacterAttributeComponent::OnLevelDeserialized_Implementation()
{
	OnHealthChanged();
}

