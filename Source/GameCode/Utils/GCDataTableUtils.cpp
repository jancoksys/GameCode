// Fill out your copyright notice in the Description page of Project Settings.


#include "GCDataTableUtils.h"
#include "GameCodeTypes.h"
#include "Engine/DataTable.h"
#include "Inventory/Items/InventoryItem.h"


FWeaponTableRow* GCDataTableUtils::FindWeaponData(const FName WeaponID)
{
	static const FString contextString(TEXT("Find Weapon Data"));

	UDataTable* weaponDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/GameCode/Core/Data/DataTables/DT_WeaponList.DT_WeaponList"));

	if (weaponDataTable == nullptr)
	{
		return nullptr;
	}

	return weaponDataTable->FindRow<FWeaponTableRow>(WeaponID, contextString);
}

FItemTableRow* GCDataTableUtils::FindInventoryItemData(const FName ItemID)
{
	static const FString contextString(TEXT("Find Item Data"));

	UDataTable* InventoryItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/GameCode/Core/Data/DataTables/DT_InventoryItemList.DT_InventoryItemList"));

	if (InventoryItemDataTable == nullptr)
	{
		return nullptr;
	}

	return InventoryItemDataTable->FindRow<FItemTableRow>(ItemID, contextString);
}

FAmmoTableRow* GCDataTableUtils::FindAmmoTableData(const FName ItemID)
{
	static const FString contextString(TEXT("Find Ammo Data"));

	UDataTable* AmmoItemDataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/GameCode/Core/Data/DataTables/DT_AmmoList.DT_AmmoList"));

	if (AmmoItemDataTable == nullptr)
	{
		return nullptr;
	}

	return AmmoItemDataTable->FindRow<FAmmoTableRow>(ItemID, contextString);
}