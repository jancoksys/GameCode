// Fill out your copyright notice in the Description page of Project Settings.


#include "GCAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Damage.h"

AGCAIController::AGCAIController()
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
}

AActor* AGCAIController::GetClosestSensedActor(TSubclassOf<UAISense> SenseClass) const
{
	if (!IsValid(GetPawn()))
	{
		return nullptr;
	}
	
	TArray<AActor*> SensedActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(SenseClass, SensedActors);
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Damage::StaticClass(), SensedActors);

	AActor* ClosestActor = nullptr;
	float MinSquaredDistance = FLT_MAX;
	FVector PawnLocation = GetPawn()->GetActorLocation();

	for (AActor* SeenActor : SensedActors)
	{
		float CurrentSquaredDistance = (PawnLocation - SeenActor->GetActorLocation()).SizeSquared();
		if (CurrentSquaredDistance < MinSquaredDistance)
		{
			MinSquaredDistance = CurrentSquaredDistance;
			ClosestActor = SeenActor;
		}
	}
	return ClosestActor;
}
