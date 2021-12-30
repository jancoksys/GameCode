// Fill out your copyright notice in the Description page of Project Settings.


#include "PickableWeapon.h"
#include "GameCodeTypes.h"
#include "Utils/GCDataTableUtils.h"
#include "Inventory/Items/InventoryItem.h"
#include "Characters/GCBaseCharacter.h"
#include "Inventory/Items/Equipables/WeaponInventoryItem.h"

APickableWeapon::APickableWeapon()
{
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
}

void APickableWeapon::Interact(AGCBaseCharacter* Character)
{
	FWeaponTableRow* WeaponRow = GCDataTableUtils::FindWeaponData(DataTableID);
	if (WeaponRow)
	{		
		TWeakObjectPtr<UWeaponInventoryItem> Weapon = NewObject<UWeaponInventoryItem>(Character);
		Weapon->Initialize(DataTableID, WeaponRow->WeaponItemDescription);
		Weapon->SetEquipWeaponClass(WeaponRow->EquipableActorClass);
		Character->PickupItem(Weapon);
		Destroy();
	}
}

FName APickableWeapon::GetActionEventName() const
{
	return ActionInteract;
}
