// Fill out your copyright notice in the Description page of Project Settings.


#include "GCAICharacter.h"
#include "Components/CharacterComponents/AIPatrollingComponent.h"
#include <AI/Controllers/GCAIController.h>

AGCAICharacter::AGCAICharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIPatrollingComponent = CreateDefaultSubobject<UAIPatrollingComponent>(TEXT("AIPatrolling"));
}

UAIPatrollingComponent* AGCAICharacter::GetPatrollingComponent() const
{
	return AIPatrollingComponent;
}

UBehaviorTree* AGCAICharacter::GetBehaviorTree() const
{
	return BehaviorTree;
}