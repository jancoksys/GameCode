// Fill out your copyright notice in the Description page of Project Settings.


#include "PatrollingPath.h"

const TArray<FVector>& APatrollingPath::GetWaypoints() const
{
	return WayPoints;
}