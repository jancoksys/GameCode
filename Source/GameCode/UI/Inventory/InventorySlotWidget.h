// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class UTextBlock;
class UInventoryViewWidget;
class UImage;
struct FInventorySlot;

UCLASS()
class GAMECODE_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeItemSlot(FInventorySlot& InventorySlot, UInventoryViewWidget* ParentViewWidget);
	void UpdateView();
	void SetItemIcon(UTexture2D* Icon);

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* ImageItemIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ItemCountText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ItemSpawnDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 ItemCount;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	FInventorySlot* LinkedSlot;

	UFUNCTION()
	void UpdateItemCount(int32 NewItemCount);

	UInventoryViewWidget* ParentView;
};
