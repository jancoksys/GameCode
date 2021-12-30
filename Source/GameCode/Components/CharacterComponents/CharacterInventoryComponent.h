// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Items/Equipables/InventoryAmmoItem.h"
#include "CharacterInventoryComponent.generated.h"

class AGCBaseCharacter;
class UInventoryItem;

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE(FInventorySlotUpdate)
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlotCountChangedSignature, int32);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<UInventoryItem> Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;

	void BindOnInventorySlotUpdate(const FInventorySlotUpdate& Callback) const;
	void UnbindOnInventorySlotUpdate();
	void UpdateSlotState();
	void ClearSlot();

	FOnSlotCountChangedSignature OnSlotCountChangedEvent;
private:
	mutable FInventorySlotUpdate OnInventorySlotUpdate;
};

class UInventoryViewWidget;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMECODE_API UCharacterInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	void OpenViewInventory(APlayerController* PlayerController);
	void CloseViewInventory();
	bool IsViewVisible() const;

	int32 GetCapacity() const;
	bool HasFreeSlot() const;
	UInventoryItem* FindAmmoItem(UInventoryAmmoItem*& AmmoItem_In);
	bool AddItem(TWeakObjectPtr<UInventoryItem> ItemToAdd, int32 Count);
	bool RemoveItem(FName ItemID);
	bool ChangeItemCount(UInventoryItem* Item_In, int32 NewCount);

	TArray<FInventorySlot> GetAllItemsCopy() const;
	TArray<FText> GetAllItemsNames() const;
	void AddInitialAmmoItems(UInventoryAmmoItem* ItemToAdd);
protected:
	UPROPERTY(EditAnywhere, Category = "Items")
	TArray<FInventorySlot> InventorySlots;

	UPROPERTY(EditAnywhere, Category = "View settings")
	TSubclassOf<UInventoryViewWidget> InventoryViewWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory settings", meta = (ClampMin = 1, UIMin = 1))
	int32 Capacity = 16;

	virtual void BeginPlay() override;	

	void CreateViewWidget(APlayerController* PlayerController);

	FInventorySlot* FindItemSlot(FName ItemID);

	FInventorySlot* FindFreeSlot();

private:
	UPROPERTY()
	UInventoryViewWidget* InventoryViewWidget;

	int32 ItemsInInventory;

	TWeakObjectPtr<AGCBaseCharacter> CachedCharacterOwner;
	TArray<UInventoryAmmoItem*> InitialItems;
	void InitItems();
};
