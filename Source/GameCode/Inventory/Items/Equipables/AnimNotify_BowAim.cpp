// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_BowAim.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"

void UAnimNotify_BowAim::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	AGCBaseCharacter* CharacterOwner = Cast<AGCBaseCharacter>(MeshComp->GetOwner());
	if (IsValid(CharacterOwner))
	{
		CharacterOwner->GetCharacterEquipmentComponent_Mutable()->GetCurrentRangeWeapon()->EndReload(true);
	}	
}
