// Fill out your copyright notice in the Description page of Project Settings.


#include "AITurretController.h"
#include "Perception/AISense_Sight.h"
#include "AIController.h"
#include "AI/Characters/Turret.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Damage.h"

void AAITurretController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	if (IsValid(InPawn))
	{
		checkf(InPawn->IsA<ATurret>(), TEXT("AAITurretController::SetPawn AAIturretController can possess only Turrets"));
		CachedTurret = StaticCast<ATurret*>(InPawn);
	}
	else
	{
		CachedTurret = nullptr;
	}
}

void AAITurretController::ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	Super::ActorsPerceptionUpdated(UpdatedActors);
	if (!CachedTurret.IsValid())
	{
		return;
	}
	
	AActor* ClosestActor = GetClosestSensedActor(UAISense_Sight::StaticClass());
	CachedTurret->CurrentTarget = ClosestActor;
	CachedTurret->OnCurrentTargetSet();
}
