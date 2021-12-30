// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_Fire.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Components/CharacterComponents/CharacterAttributeComponent.h"

UBTService_Fire::UBTService_Fire()
{
	NodeName = "Fire";
}

void UBTService_Fire::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	
	if (!IsValid(AIController) || !IsValid(Blackboard))
	{
		return;
	}

	AGCBaseCharacter* Character = Cast<AGCBaseCharacter>(AIController->GetPawn());
	if (!IsValid(Character))
	{
		return;
	}
	if (!Character->GetCharacterAttributesComponent_Mutable()->IsAlive())
	{
		return;
	}

	const UCharacterEquipmentComponent* EquipmentComponent = Character->GetCharacterEquipmentComponent();
	ARangeWeaponItem* RangeWeapon = EquipmentComponent->GetCurrentRangeWeapon();
	if (!IsValid(RangeWeapon))
	{
		return;
	}

	AGCBaseCharacter* CurrentTarget = Cast<AGCBaseCharacter>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!IsValid(CurrentTarget))
	{
		Character->StopFire();
		return;
	}
	if (!CurrentTarget->GetCharacterAttributesComponent_Mutable()->IsAlive())
	{
		Character->StopFire();
		return;
	}

	float DistSq = FVector::DistSquared(CurrentTarget->GetActorLocation(), Character->GetActorLocation());
	if (DistSq > FMath::Square(MaxFireDistance))
	{
		Character->StopFire();
		return;
	}

	if (!(RangeWeapon->IsReloading() || RangeWeapon->IsFiring()))
	{
		Character->StartFire();
	}
}
