// Fill out your copyright notice in the Description page of Project Settings.


#include "PickableAmmo.h"

#include "Characters/GCBaseCharacter.h"
#include "Inventory/Items/InventoryItem.h"
#include "Inventory/Items/Equipables/InventoryAmmoItem.h"
#include "Utils/GCDataTableUtils.h"

APickableAmmo::APickableAmmo()
{
	AmmoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmmoMesh"));
	SetRootComponent(AmmoMesh);
}

void APickableAmmo::Interact(AGCBaseCharacter* Character)
{
	FAmmoTableRow* AmmoData = GCDataTableUtils::FindAmmoTableData(GetDataTableID());

	if (AmmoData == nullptr)
	{
		return;
	}
	TWeakObjectPtr<UInventoryAmmoItem> Item = NewObject<UInventoryAmmoItem>(Character);
	Item->Initialize(DataTableID, AmmoData->InventoryItemDescription);
	Item->SetAmunitionType(AmmoData->AmunitionType);
	Item->SetAmmoAmount(AmmoData->AmmoToAdd);
	const bool bPickedUp = Character->PickupItem(Item);
	if (bPickedUp)
	{
		if (OnPickedUpEvent.IsBound())
		{
			OnPickedUpEvent.Broadcast(PickablePoolIndex);
		}
	}
}

FName APickableAmmo::GetActionEventName() const
{
	return ActionInteract;
}