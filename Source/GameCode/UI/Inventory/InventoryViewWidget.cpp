// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryViewWidget.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "InventorySlotWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"

void UInventoryViewWidget::InitializeViewWidget(TArray<FInventorySlot>& InventorySlots)
{
	for (FInventorySlot& Item : InventorySlots)
	{
		AddItemSlotView(Item);
	}
}

bool UInventoryViewWidget::IsPositionOutOfBounds(const FVector2D& Position)
{
	UCanvasPanelSlot* CanvasPanelSlot = StaticCast<UCanvasPanelSlot*>(GridPanelItemSlots->Slot);
	if (IsValid(CanvasPanelSlot))
	{		
		FAnchors GridPanelAnchors = CanvasPanelSlot->GetAnchors();
		FVector2D GridPanelPosition = CanvasPanelSlot->GetPosition();
		FVector2D GridPanelSize = CanvasPanelSlot->GetSize();

		FIntPoint ViewportUpperRight = GEngine->GameViewport->Viewport->ViewportToVirtualDesktopPixel(GridPanelAnchors.Maximum);
		FVector2D WidgetUpperRight = FVector2D(ViewportUpperRight.X + GridPanelPosition.X, ViewportUpperRight.Y + GridPanelPosition.Y);

		if (Position.X > WidgetUpperRight.X || Position.X < WidgetUpperRight.X - GridPanelSize.X ||
		    Position.Y < WidgetUpperRight.Y || Position.Y > WidgetUpperRight.Y + GridPanelSize.Y)
		{
			return true;
		}
	}
	return false;
}

void UInventoryViewWidget::AddItemSlotView(FInventorySlot& SlotToAdd)
{
	checkf(InventorySlotWidgetClass.Get() != nullptr, TEXT("UItemContainerWidget::AddItemSlotView widget class doesn't not exist"));

	UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, InventorySlotWidgetClass);	

	if (SlotWidget != nullptr)
	{		
		SlotWidget->InitializeItemSlot(SlotToAdd, this);		
		const int32 CurrentSlotCount = GridPanelItemSlots->GetChildrenCount();
		const int32 CurrentSlotRow = CurrentSlotCount / ColumnCount;
		const int32 CurrentSlotColumn = CurrentSlotCount % ColumnCount;
		GridPanelItemSlots->AddChildToGrid(SlotWidget, CurrentSlotRow, CurrentSlotColumn);		
		
		SlotWidget->UpdateView();
	}
}
