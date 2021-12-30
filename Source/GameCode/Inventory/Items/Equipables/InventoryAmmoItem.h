// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/Items/InventoryItem.h"
#include "InventoryAmmoItem.generated.h"

/**
 * 
 */
UCLASS()
class GAMECODE_API UInventoryAmmoItem : public UInventoryItem
{
	GENERATED_BODY()

public:
	EAmunitionType GetAmunitionType() const;
	int32 GetAmmoAmount() const;
	void SetDataTableID(const FName& DataTableID_In);

	void SetAmmoAmount(int32 AmmoAmount_In);
	void SetAmunitionType(EAmunitionType AmunitionType_In);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Amunition")
	EAmunitionType AmunitionType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Amunition", meta = (UIMin = 0, ClampMin = 0))
	int32 AmmoToAdd;
};
