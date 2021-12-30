// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_Throw.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Actors/Equipment/Throwables/ThowableItem.h"
#include "Components/CharacterComponents/CharacterAttributeComponent.h"

UBTService_Throw::UBTService_Throw()
{
	NodeName = "Throw";
}

void UBTService_Throw::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (bOutOfAmmo)
	{
		return;
	}

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
	UCharacterEquipmentComponent* EquipmentComponent = Character->GetCharacterEquipmentComponent_Mutable();
	if (!IsValid(EquipmentComponent))
	{
		return;
	}	

	AGCBaseCharacter* CurrentTarget = Cast<AGCBaseCharacter>(Blackboard->GetValueAsObject(TargetKey.SelectedKeyName));
	if (!IsValid(CurrentTarget))
	{
		return;
	}
	if (!CurrentTarget->GetCharacterAttributesComponent_Mutable()->IsAlive())
	{
		return;
	}
	float DistSq = FVector::DistSquared(CurrentTarget->GetActorLocation(), Character->GetActorLocation());
	if (DistSq > FMath::Square(MaxThrowDistance) || DistSq < FMath::Square(MinThrowDistance))
	{		
		return;
	}
	
	if (IsValid(CurrentThrowableItem) && CurrentThrowableItem->GetItemAmount() == 0)
	{
		EquipmentComponent->EquipItemInSlot(EEquipmentSlots::PrimaryWeapon);
		bOutOfAmmo = true;
		return;
	}

	Character->EquipPrimaryItem();
	CurrentThrowableItem = EquipmentComponent->GetCurrentThrowableItem();
}
