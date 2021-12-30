// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_SpawnArrow.h"

#include "Actors/Equipment/Weapons/Bow.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"

void UAnimNotify_SpawnArrow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	AGCBaseCharacter* CharacterOwner = Cast<AGCBaseCharacter>(MeshComp->GetOwner());
	if (!IsValid(CharacterOwner))
	{
		return;
	}
	ABow* CurrentBow = StaticCast<ABow*>(CharacterOwner->GetCharacterEquipmentComponent_Mutable()->GetCurrentRangeWeapon());
	CurrentBow->SetArrowVisibility(true);
}
