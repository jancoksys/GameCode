// Fill out your copyright notice in the Description page of Project Settings.


#include "AIPatrollingComponent.h"
#include "Actors/Navigation/PatrollingPath.h"

FVector UAIPatrollingComponent::SelectClosestWaypoint()
{
	FVector OwnerLocation = GetOwner()->GetActorLocation();
	const TArray<FVector> WayPoints = PatrollingSettings.PatrollingPath->GetWaypoints();
	FTransform PathTransform = PatrollingSettings.PatrollingPath->GetActorTransform();

	FVector ClosestWaypoint;
	float MinSqDistance = FLT_MAX;
	for (int32 i = 0; i < WayPoints.Num(); ++i)
	{
		FVector WayPointWorld = PathTransform.TransformPosition(WayPoints[i]);
		float CurrentSqDistance = (OwnerLocation - WayPointWorld).SizeSquared();
		if (CurrentSqDistance < MinSqDistance)
		{
			MinSqDistance = CurrentSqDistance;
			ClosestWaypoint = WayPointWorld;
			CurrentWaypointIndex = i;
		}
	}
	return ClosestWaypoint;
}

FVector UAIPatrollingComponent::SelectNextWaypoint()
{
	const TArray<FVector> WayPoints = PatrollingSettings.PatrollingPath->GetWaypoints();
	
	CurrentWaypointIndex += bSelectWaypointsBackwards ? -1 : 1;

	if (CurrentWaypointIndex == WayPoints.Num())
	{
		CurrentWaypointIndex = PatrollingSettings.PatrollingType == EPatrollingType::Circle ? 0 : CurrentWaypointIndex - 2;
		bSelectWaypointsBackwards = PatrollingSettings.PatrollingType == EPatrollingType::Circle ? false : true;
	}
	else if (CurrentWaypointIndex == -1)
	{
		CurrentWaypointIndex = 1;
		bSelectWaypointsBackwards = false;
	}

	FTransform PathTransform = PatrollingSettings.PatrollingPath->GetActorTransform();
	FVector WayPoint = PathTransform.TransformPosition(WayPoints[CurrentWaypointIndex]);
	return WayPoint;
}

bool UAIPatrollingComponent::CanPatrol() const
{
	return IsValid(PatrollingSettings.PatrollingPath) && PatrollingSettings.PatrollingPath->GetWaypoints().Num() > 0;
}