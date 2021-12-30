// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameCodeTypes.h"
#include "Subsystems/SaveSubsystem/SaveSubsystemInterface.h"
#include "CharacterEquipmentComponent.generated.h"


class UInventoryAmmoItem;
class UWeaponWheelWidget;
typedef TArray<int32, TInlineAllocator<(uint32)EAmunitionType::MAX>> TAmunitionArray;
typedef TArray<class AEquipableItem*, TInlineAllocator<(uint32)EEquipmentSlots::MAX>> TItemsArray;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCurrentWeaponAmmoChanged, int32, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCurrentThrowablesAmountChanged, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquippedItemChanged, const AEquipableItem*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDamageChangedSignature, float);

class AEquipableItem;
class ARangeWeaponItem;
class AThowableItem;
class AMeleeWeaponItem;
class UEquipmentViewWidget;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMECODE_API UCharacterEquipmentComponent : public UActorComponent, public ISaveSubsystemInterface
{
	GENERATED_BODY()

public:
	UCharacterEquipmentComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	EEquipableItemType GetCurrentEquippedItemType() const;

	ARangeWeaponItem* GetCurrentRangeWeapon() const;	
	AThowableItem* GetCurrentThrowableItem() const;

	FOnCurrentWeaponAmmoChanged OnCurrentWeaponAmmoChangedEvent;
	FOnCurrentThrowablesAmountChanged OnCurrentThrowablesAmountChangedEvent;
	FOnEquippedItemChanged OnEquippedItemChanged;
	FOnDamageChangedSignature OnDamageChanged;

	void ReloadCurrentWeapon();
	void AddAmmoToAmunition(int32 Ammo, EAmunitionType AmunitionType);

	void EquipItemInSlot(EEquipmentSlots Slot);		

	void EquipNextItem();
	void EquipPreviousItem();
	void AttachCurrentItemToEquippedSocket();
	bool IsEquipping() const;
	void ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo = 0, bool bCheckIsFull = false);
	void LaunchCurrentThrowableItem();
	AMeleeWeaponItem* GetCurrentMeleeWeapon() const;

	int32 GetAmunitionAmount(EAmunitionType AmunitionType) const;

	bool AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem>& EquipableItemClass, int32 SlotIndex);
	void RemoveItemFromSlot(int32 SlotIndex);

	void OpenViewEquipment(APlayerController* PlayerController);
	void CloseViewEquipment();
	bool IsViewVisible() const;

	const TArray<AEquipableItem*>& GetItems() const;

	void OpenWeaponWheel(APlayerController* PlayerController);
	bool IsSelectingWeapon() const;
	void ConfirmWeaponSelection() const;

	virtual void OnLevelDeserialized_Implementation() override;
protected:
	virtual void BeginPlay() override;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EAmunitionType, int32> MaxAmunitionAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TMap<EEquipmentSlots, TSubclassOf<class AEquipableItem>> ItemsLoadout;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TSet<EEquipmentSlots> IgnoreSlotsWhileSwitching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loadout")
	EEquipmentSlots AutoEquipItemInSlot = EEquipmentSlots::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View")
	TSubclassOf<UEquipmentViewWidget> ViewWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "View")
	TSubclassOf<UWeaponWheelWidget> WeaponWheelClass;

	void CreateEquipmentWidgets(APlayerController* PlayerController);
private:
	UFUNCTION(Server, Reliable)
	void Server_EquipItemInSlot(EEquipmentSlots Slot);

	UPROPERTY(Replicated, SaveGame)
	TArray<int32> AmunitionArray;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ChangeAmmoAmount(EAmunitionType AmunitionType, int32 AmountDelta);

	UFUNCTION(Server, Reliable)
	void Server_ChangeAmmoAmount(EAmunitionType AmunitionType, int32 AmountDelta);

	UFUNCTION(Server, Reliable)
	void Server_ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo = 0, bool bCheckIsFull = false);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo = 0, bool bCheckIsFull = false);

	UPROPERTY(ReplicatedUsing=OnRep_ItemsArray, SaveGame)
	TArray<AEquipableItem*> ItemsArray;

	UFUNCTION()
	void OnRep_ItemsArray();

	UFUNCTION()
	void OnWeaponReloadComplete();
	void CreateLoadout();	
	void AutoEquip();

	TWeakObjectPtr<class AGCBaseCharacter> CachedBaseCharacter;

	AEquipableItem* CurrentEquippedItem;
	
	EEquipmentSlots PreviousEquippedSlot;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentEquippedSlot, SaveGame)
	EEquipmentSlots CurrentEquippedSlot;

	UFUNCTION()
	void OnRep_CurrentEquippedSlot(EEquipmentSlots CurrentEquippedSlot_Old);

	AThowableItem* CurrentThrowableItem;
	AMeleeWeaponItem* CurrentMeleeWeapon;
	ARangeWeaponItem* CurrentEquippedWeapon;

	UFUNCTION()
	void OnCurrentWeaponAmmoChanged(int32 Ammo);
	int32 GetAvailableAmunitionForCurrentWeapon();

	FDelegateHandle OnCurrentWeaponAmmoChangedHandle;
	FDelegateHandle OnCurrentWeaponReloadedHandle;
	FDelegateHandle OnCurrentThrowablesAmountChangedHandle;
	
	uint32 NextItemsArraySlotIndex(uint32 CurrentSlotIndex);
	uint32 PreviousItemsArraySlotIndex(uint32 CurrentSlotIndex);
	void UnequipCurrentItem();

	bool bIsEquipping = false;
	FTimerHandle EquipTimer;
	void EquipAnimationFinished();

	UFUNCTION()
	void OnCurrentThrowablesAmountChanged(int32 Amount);

	void SetThrowablesAmountToItem(class AEquipableItem* Item);	

	UPROPERTY()
	UEquipmentViewWidget* ViewWidget = nullptr;

	UPROPERTY()
	UWeaponWheelWidget* WeaponWheelWidget = nullptr;

	TArray<TWeakObjectPtr<UInventoryAmmoItem>> AmmoItemsInInventory;
	void ChangeAmunitionAmount(EAmunitionType AmunitionType, int32 AmountDelta);
};
