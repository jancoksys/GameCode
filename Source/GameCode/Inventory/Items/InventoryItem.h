// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameCodeTypes.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "InventoryItem.generated.h"

class APickableItem;
class AEquipableItem;
class UInventoryItem;
class AGCBaseCharacter;

USTRUCT(BlueprintType)
struct FInventoryItemDescription : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	UTexture2D* Icon;
};

USTRUCT(BlueprintType)
struct FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	TSubclassOf<APickableItem> PickableActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	FInventoryItemDescription InventoryItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	TSubclassOf<UInventoryItem> InventoryItemClass;
};

USTRUCT(BlueprintType)
struct FWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon view")
	TSubclassOf<APickableItem> PickableActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon view")
	TSubclassOf<AEquipableItem> EquipableActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon view")
	FInventoryItemDescription WeaponItemDescription;
};

USTRUCT(BlueprintType)
struct FAmmoTableRow : public FTableRowBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	TSubclassOf<APickableItem> PickableActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	EAmunitionType AmunitionType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view", meta = (UIMin = 0, ClampMin = 0))
	int32 AmmoToAdd;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	FInventoryItemDescription InventoryItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item view")
	TSubclassOf<UInventoryItem> InventoryItemClass;
};

UCLASS(Blueprintable)
class GAMECODE_API UInventoryItem : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(FName DataTableID_In, const FInventoryItemDescription& Description_In);

	FName GetDataTableID() const;
	const FInventoryItemDescription& GetDescription() const;

	virtual bool IsEquipable() const;
	virtual bool IsConsumable() const;
	
	virtual bool Consume(AGCBaseCharacter* ConsumeTarger) PURE_VIRTUAL(UInventoryItem::Consume, return false;);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory item")
	FName DataTableID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory item")
	FInventoryItemDescription Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory item")
	bool bIsEquipable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory item")
	bool bIsConsumable = false;

private:
	bool bIsInitialized = false;
};
