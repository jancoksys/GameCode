// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AIPatrollingComponent.generated.h"

UENUM(BlueprintType)
enum class EPatrollingType : uint8
{
	Circle,
	PingPong
};

class APatrollingPath;

USTRUCT(BlueprintType)
struct FPatrollingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	APatrollingPath* PatrollingPath;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	EPatrollingType PatrollingType = EPatrollingType::Circle;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMECODE_API UAIPatrollingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	FVector SelectClosestWaypoint();
	FVector SelectNextWaypoint();
	bool CanPatrol() const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Patrolling")
	FPatrollingSettings PatrollingSettings;

private:
	int32 CurrentWaypointIndex = -1;

	bool bSelectWaypointsBackwards = false;
};
