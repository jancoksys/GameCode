// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Actors/Interactive/Interface/Interactive.h"
#include "PickableItem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPickedUpEventSignature, int32);

UCLASS(Abstract, NotBlueprintable)
class GAMECODE_API APickableItem : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:
	APickableItem();
	void SetPickablePoolIndex(int32 Index_In);
	const FName& GetDataTableID() const;
	FOnPickedUpEventSignature OnPickedUpEvent;
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName DataTableID = NAME_None;

	int32 PickablePoolIndex;
};
