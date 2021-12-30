// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_Throw.generated.h"

/**
 * 
 */
UCLASS()
class GAMECODE_API UBTService_Throw : public UBTService
{
	GENERATED_BODY()
	
public:
	UBTService_Throw();

protected:
	
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float MinThrowDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float MaxThrowDistance = 800.0f;

private:
	bool bOutOfAmmo = false;

	class AThowableItem* CurrentThrowableItem = nullptr;
};
