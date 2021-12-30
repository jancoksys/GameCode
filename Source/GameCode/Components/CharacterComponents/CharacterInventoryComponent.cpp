// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterInventoryComponent.h"
#include "CharacterEquipmentComponent.h"
#include "Characters/GCBaseCharacter.h"
#include "UI/Inventory/InventoryViewWidget.h"
#include "Inventory/Items/InventoryItem.h"
#include "Inventory/Items/Equipables/InventoryAmmoItem.h"
#include "Utils/GCDataTableUtils.h"

void UCharacterInventoryComponent::OpenViewInventory(APlayerController* PlayerController)
{
	if (!IsValid(InventoryViewWidget))
	{
		CreateViewWidget(PlayerController);
	}

	if (!InventoryViewWidget->IsVisible())
	{
		InventoryViewWidget->AddToViewport();
	}
}

void UCharacterInventoryComponent::CloseViewInventory()
{
	if (InventoryViewWidget->IsVisible())
	{
		InventoryViewWidget->RemoveFromParent();
	}
}

bool UCharacterInventoryComponent::IsViewVisible() const
{
	bool Result = false;
	if (IsValid(InventoryViewWidget))
	{
		Result = InventoryViewWidget->IsVisible();
	}
	return Result;
}

int32 UCharacterInventoryComponent::GetCapacity() const
{
	return Capacity;
}

bool UCharacterInventoryComponent::HasFreeSlot() const
{
	return ItemsInInventory < Capacity;
}

UInventoryItem* UCharacterInventoryComponent::FindAmmoItem(UInventoryAmmoItem*& AmmoItem_In)
{
	for (FInventorySlot& Slot : InventorySlots)
	{
		if (!IsValid(Slot.Item.Get()))
		{
			continue;
		}
		UInventoryAmmoItem* AmmoItem = Cast<UInventoryAmmoItem>(Slot.Item);
		if (IsValid(AmmoItem))
		{
			if (AmmoItem->GetAmunitionType() == AmmoItem_In->GetAmunitionType())
			{
				return AmmoItem;
			}
		}
	}
	return nullptr;
}

bool UCharacterInventoryComponent::AddItem(TWeakObjectPtr<UInventoryItem> ItemToAdd, int32 Count)
{
	if (!ItemToAdd.IsValid() || Count < 0)
	{
		return false;
	}

	bool Result = false;

	FInventorySlot* FreeSlot = FindFreeSlot();

	if (FreeSlot != nullptr)
	{
		FreeSlot->Item = ItemToAdd;
		FreeSlot->Count = Count;
		if (FreeSlot->OnSlotCountChangedEvent.IsBound())
		{
			FreeSlot->OnSlotCountChangedEvent.Broadcast(Count);
		}
		ItemsInInventory++;
		Result = true;
		FreeSlot->UpdateSlotState();
	}
	UInventoryAmmoItem* AmmoItem = Cast<UInventoryAmmoItem>(ItemToAdd);
	if (IsValid(AmmoItem))
	{
		UCharacterEquipmentComponent* EquipmentComponent = CachedCharacterOwner->GetCharacterEquipmentComponent_Mutable();
		EquipmentComponent->AddAmmoToAmunition(Count, AmmoItem->GetAmunitionType());
	}
	
	return Result;
}

bool UCharacterInventoryComponent::RemoveItem(FName ItemID)
{
	FInventorySlot* ItemSlot = FindItemSlot(ItemID);
	if (ItemSlot != nullptr)
	{
		InventorySlots.RemoveAll([=](const FInventorySlot& Slot) { return Slot.Item->GetDataTableID() == ItemID; });
		return true;
	}
	return false;
}

bool UCharacterInventoryComponent::ChangeItemCount(UInventoryItem* Item_In, int32 NewCount)
{
	if (!IsValid(Item_In))
	{
		return false;
	}
	for (FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.Item == Item_In)
		{
			UInventoryAmmoItem* AmmoItem = Cast<UInventoryAmmoItem>(Item_In);
			if (IsValid(AmmoItem))
			{
				AmmoItem->SetAmmoAmount(NewCount);
			}
			Slot.Count = NewCount;
			Slot.OnSlotCountChangedEvent.Broadcast(Slot.Count);
			return true;
		}
	}
	return false;
}

TArray<FInventorySlot> UCharacterInventoryComponent::GetAllItemsCopy() const
{
	return InventorySlots;
}

TArray<FText> UCharacterInventoryComponent::GetAllItemsNames() const
{
	TArray<FText> Result;
	for (const FInventorySlot& Slot : InventorySlots)
	{
		if (Slot.Item.IsValid())
		{
			Result.Add(Slot.Item->GetDescription().Name);
		}
	}
	return Result;
}

void UCharacterInventoryComponent::AddInitialAmmoItems(UInventoryAmmoItem* ItemToAdd)
{
	InitialItems.Add(ItemToAdd);
}

void UCharacterInventoryComponent::BeginPlay()
{
	Super::BeginPlay();	
	checkf(GetOwner()->IsA<AGCBaseCharacter>(), TEXT("UCharacterInventoryComponent::BeginPlay UCharacterInventoryComponent can be used only with GCBaseCharacter"));
	CachedCharacterOwner = StaticCast<AGCBaseCharacter*>(GetOwner());
	InventorySlots.AddDefaulted(Capacity);
	InitItems();
}

void UCharacterInventoryComponent::CreateViewWidget(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController) || !IsValid(InventoryViewWidgetClass) || IsValid(InventoryViewWidget))
	{
		return;
	}

	InventoryViewWidget = CreateWidget<UInventoryViewWidget>(PlayerController, InventoryViewWidgetClass);
	InventoryViewWidget->InitializeViewWidget(InventorySlots);
}

FInventorySlot* UCharacterInventoryComponent::FindItemSlot(FName ItemID)
{
	return InventorySlots.FindByPredicate([=](const FInventorySlot& Slot) { return Slot.Item->GetDataTableID() == ItemID; });
}

FInventorySlot* UCharacterInventoryComponent::FindFreeSlot()
{
	return InventorySlots.FindByPredicate([=](const FInventorySlot& Slot) { return !Slot.Item.IsValid(); });
}

void UCharacterInventoryComponent::InitItems()
{
	for (UInventoryAmmoItem*& Item : InitialItems)
	{
		FAmmoTableRow* AmmoData = GCDataTableUtils::FindAmmoTableData(Item->GetDataTableID());

		if (AmmoData == nullptr)
		{
			UCharacterEquipmentComponent* EquipmentComponent = CachedCharacterOwner->GetCharacterEquipmentComponent_Mutable();
			EquipmentComponent->AddAmmoToAmunition(Item->GetAmmoAmount(), Item->GetAmunitionType());
			continue;
		}
		Item->Initialize(Item->GetDataTableID(), AmmoData->InventoryItemDescription);
		
		AddItem(Item, Item->GetAmmoAmount());
	}
}

void FInventorySlot::BindOnInventorySlotUpdate(const FInventorySlotUpdate& Callback) const
{
	OnInventorySlotUpdate = Callback;
}

void FInventorySlot::UnbindOnInventorySlotUpdate()
{
	OnInventorySlotUpdate.Unbind();
}

void FInventorySlot::UpdateSlotState()
{
	OnInventorySlotUpdate.ExecuteIfBound();
}

void FInventorySlot::ClearSlot()
{
	Item = nullptr;
	Count = 0;
	UpdateSlotState();
	
}
