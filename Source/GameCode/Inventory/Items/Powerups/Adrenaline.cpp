// Fill out your copyright notice in the Description page of Project Settings.


#include "Adrenaline.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterAttributeComponent.h"

bool UAdrenaline::Consume(AGCBaseCharacter* ConsumeTarger)
{
	ConsumeTarger->GetCharacterAttributesComponent_Mutable()->RestoreFullStamina();
	this->ConditionalBeginDestroy();
	return true;
}
