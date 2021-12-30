// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySlotWidget.h"
#include "InventoryViewWidget.h"
#include "Actors/Interactive/Pickables/PickableWeapon.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "Inventory/Items/InventoryItem.h"
#include "Characters/GCBaseCharacter.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Utils/GCDataTableUtils.h"

void UInventorySlotWidget::InitializeItemSlot(FInventorySlot& InventorySlot, UInventoryViewWidget* ParentViewWidget)
{
	LinkedSlot = &InventorySlot;
	ParentView = ParentViewWidget;
	FInventorySlot::FInventorySlotUpdate OnInventorySlotUpdate;
	OnInventorySlotUpdate.BindUObject(this, &UInventorySlotWidget::UpdateView);
	LinkedSlot->BindOnInventorySlotUpdate(OnInventorySlotUpdate);
	LinkedSlot->OnSlotCountChangedEvent.AddUFunction(this, FName("UpdateItemCount"));
}

void UInventorySlotWidget::UpdateView()
{
	if (LinkedSlot == nullptr)
	{
		ImageItemIcon->SetBrushFromTexture(nullptr);
		UpdateItemCount(0);
		return;
	}

	if (LinkedSlot->Item.IsValid())
	{
		const FInventoryItemDescription& Description = LinkedSlot->Item->GetDescription();
		ImageItemIcon->SetBrushFromTexture(Description.Icon);
		UInventoryAmmoItem* AmmoItem = Cast<UInventoryAmmoItem>(LinkedSlot->Item);
		if (IsValid(AmmoItem))
		{
			UpdateItemCount(AmmoItem->GetAmmoAmount());
		}
		else
		{
			UpdateItemCount(1);
		}
	}
	else
	{
		ImageItemIcon->SetBrushFromTexture(nullptr);
		UpdateItemCount(0);
	}
}

void UInventorySlotWidget::SetItemIcon(UTexture2D* Icon)
{
	ImageItemIcon->SetBrushFromTexture(Icon);
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (LinkedSlot == nullptr)
	{
		return FReply::Handled();
	}

	if (!LinkedSlot->Item.IsValid())
	{
		return FReply::Handled();
	}

	FKey MouseBtn = InMouseEvent.GetEffectingButton();
	if (MouseBtn == EKeys::RightMouseButton)
	{
		/* Some simplification, so as not to complicate the architecture
		 * - on instancing item, we use the current pawn as an outer one.
		 * In real practice we need use callback for inform item holder what action was do in UI */

		TWeakObjectPtr<UInventoryItem> LinkedSlotItem = LinkedSlot->Item;
		AGCBaseCharacter* ItemOwner = Cast<AGCBaseCharacter>(LinkedSlotItem->GetOuter());

		if (LinkedSlotItem->Consume(ItemOwner))
		{
			LinkedSlot->ClearSlot();
		}
		return FReply::Handled();
	}

	FEventReply Reply = UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
	return Reply.NativeReply;
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UDragDropOperation* DragOperation = Cast<UDragDropOperation>(UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass()));

	/* Some simplification for not define new widget for drag and drop operation  */
	UInventorySlotWidget* DragWidget = CreateWidget<UInventorySlotWidget>(GetOwningPlayer(), GetClass());
	DragWidget->ImageItemIcon->SetBrushFromTexture(LinkedSlot->Item->GetDescription().Icon);
	DragWidget->UpdateItemCount(ItemCount);
	LinkedSlot->Count = ItemCount;

	DragOperation->DefaultDragVisual = DragWidget;
	DragOperation->Pivot = EDragPivot::MouseDown;	

	UInventoryAmmoItem* AmmoItem = Cast<UInventoryAmmoItem>(LinkedSlot->Item.Get());
	if (IsValid(AmmoItem))
	{
		AmmoItem->SetAmmoAmount(LinkedSlot->Count);
		DragOperation->Payload = AmmoItem;
	}
	else
	{
		DragOperation->Payload = LinkedSlot->Item.Get();
	}
	
	OutOperation = DragOperation;
	LinkedSlot->ClearSlot();
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!LinkedSlot->Item.IsValid())
	{
		LinkedSlot->Item = TWeakObjectPtr<UInventoryItem>(Cast<UInventoryItem>(InOperation->Payload));
		LinkedSlot->UpdateSlotState();
		return true;
	}

	return false;
}

void UInventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{	
	const FVector2D CursorPosition = InDragDropEvent.GetScreenSpacePosition();
	
	TWeakObjectPtr<UInventoryItem> InventoryItem = TWeakObjectPtr<UInventoryItem>(Cast<UInventoryItem>(InOperation->Payload));	
	if (ParentView->IsPositionOutOfBounds(CursorPosition))
	{
		
		FName DataTableID = InventoryItem->GetDataTableID();
		FWeaponTableRow* WeaponRow = GCDataTableUtils::FindWeaponData(DataTableID);
		FItemTableRow* ItemRow = GCDataTableUtils::FindInventoryItemData(DataTableID);
		TSubclassOf<APickableItem> ClassToSpawn;
		
		if (WeaponRow != nullptr)
		{
			ClassToSpawn = WeaponRow->PickableActorClass;	
		}
		else if (ItemRow != nullptr)
		{
			ClassToSpawn = ItemRow->PickableActorClass;
		}
		FVector PlayerViewPoint;
		FRotator PlayerViewRotation;
		GetOwningPlayer()->GetPlayerViewPoint(PlayerViewPoint, PlayerViewRotation);	
		
		FVector ViewDirection = PlayerViewRotation.RotateVector(FVector::ForwardVector);
		FVector SpawnLocation = PlayerViewPoint + ViewDirection * ItemSpawnDistance;

		GetWorld()->SpawnActor<APickableItem>(ClassToSpawn, SpawnLocation, FRotator::ZeroRotator);
		LinkedSlot->ClearSlot();
	}
	else
	{
		LinkedSlot->Item = InventoryItem;	
		LinkedSlot->UpdateSlotState();
	}	
}

void UInventorySlotWidget::UpdateItemCount(int32 NewItemCount)
{
	ItemCount = NewItemCount;
}