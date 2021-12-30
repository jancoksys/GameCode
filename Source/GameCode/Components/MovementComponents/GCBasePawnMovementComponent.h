// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GCBasePawnMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class GAMECODE_API UGCBasePawnMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

public:
	void JumpStart();
	
	virtual bool IsFalling() const override;

protected:
	UPROPERTY(EditAnywhere)
	float MaxSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 0.0f, UIMIn = 0.0f))
	float InitialJumpVelocity = 500.0f;

	UPROPERTY(EditAnywhere)
	bool bEnableGravity;

private:
	FVector VerticalVelocity = FVector::ZeroVector;
	bool bIsFalling = false;

	AActor* CurrentSurfaceActor = nullptr;
	TWeakObjectPtr<UPrimitiveComponent> CurrentSurfacePrimitve = nullptr;
	FVector CurrentSurfaceNormal = FVector::ZeroVector;
};
