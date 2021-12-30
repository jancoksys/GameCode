// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Interactive/Pickables/PickableItem.h"
#include "PickableAmmo.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class GAMECODE_API APickableAmmo : public APickableItem
{
	GENERATED_BODY()
	
public:
	APickableAmmo();

	virtual void Interact(AGCBaseCharacter* Character) override;
	virtual FName GetActionEventName() const override;

protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AmmoMesh;
};
