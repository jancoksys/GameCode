// Fill out your copyright notice in the Description page of Project Settings.


#include "GCAICharacterController.h"
#include "AI/Characters/GCAICharacter.h"
#include "Perception/AISense_Sight.h"
#include "Components/CharacterComponents/AIPatrollingComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameCodeTypes.h"
#include "Components/CharacterComponents/CharacterAttributeComponent.h"

void AGCAICharacterController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
	if (IsValid(InPawn))
	{
		checkf(InPawn->IsA<AGCAICharacter>(), TEXT("AGCAICharacterController::SetPawn AICharacterController can possess only GCAICharacter"));
		CachedAICharacter = StaticCast<AGCAICharacter*>(InPawn);
		RunBehaviorTree(CachedAICharacter->GetBehaviorTree());
		SetupPatrolling();
	}
	else
	{
		CachedAICharacter = nullptr;
	}
}

void AGCAICharacterController::ActorsPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	Super::ActorsPerceptionUpdated(UpdatedActors);
	if (!CachedAICharacter.IsValid())
	{
		return;
	}
	TryMoveToNextTarget();
}

void AGCAICharacterController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	if (!Result.IsSuccess())
	{
		return;
	}
	TryMoveToNextTarget();
}

void AGCAICharacterController::TryMoveToNextTarget()
{
	UAIPatrollingComponent* PatrollingComponent = CachedAICharacter->GetPatrollingComponent();
	bool bIsTargetDead = false;
	AActor* ClosestActor = GetClosestSensedActor(UAISense_Sight::StaticClass());
	AGCBaseCharacter* ClosestBaseCharacter = Cast<AGCBaseCharacter>(ClosestActor);
	
	if (IsValid(ClosestActor) && IsValid(Blackboard))
	{
		if (IsValid(ClosestBaseCharacter))
		{
			if (ClosestBaseCharacter->GetCharacterAttributesComponent_Mutable()->IsAlive())
			{
				Blackboard->SetValueAsObject(BB_CurrentTarget, ClosestActor);
				SetFocus(ClosestActor, EAIFocusPriority::Gameplay);
			}
			else
			{
				bIsTargetDead = true;
			}
			bIsPatrolling = false;
		}		
	}

	if (PatrollingComponent->CanPatrol() && !(IsValid(ClosestActor) ^ bIsTargetDead))
	{
		FVector Waypoint = bIsPatrolling ? PatrollingComponent->SelectNextWaypoint() : FVector::ZeroVector;
		if (IsValid(Blackboard))
		{
			ClearFocus(EAIFocusPriority::Gameplay);
			Blackboard->SetValueAsVector(BB_NextLocation, Waypoint);
			Blackboard->SetValueAsObject(BB_CurrentTarget, nullptr);
		}
		bIsPatrolling = true;
	}
}

bool AGCAICharacterController::IsTargetReached(FVector TargetLocation) const
{
	return (TargetLocation - CachedAICharacter->GetActorLocation()).SizeSquared() <= FMath::Square(TargetReachRadius);
}

void AGCAICharacterController::SetupPatrolling()
{
	UAIPatrollingComponent* PatrollingComponent = CachedAICharacter->GetPatrollingComponent();
	if (PatrollingComponent->CanPatrol())
	{
		FVector ClosestWaypoint = PatrollingComponent->SelectClosestWaypoint();
		if (IsValid(Blackboard))
		{
			Blackboard->SetValueAsVector(BB_NextLocation, ClosestWaypoint);
			Blackboard->SetValueAsObject(BB_CurrentTarget, nullptr);
		}
		bIsPatrolling = true;
	}
}
