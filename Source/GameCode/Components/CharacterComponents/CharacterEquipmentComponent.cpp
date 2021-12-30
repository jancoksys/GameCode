// Fill out your copyright notice in the Description page of Project Settings.

#include "CharacterEquipmentComponent.h"
#include "CharacterInventoryComponent.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Characters/GCBaseCharacter.h"
#include "GameCodeTypes.h"
#include "Actors/Equipment/Throwables/ThowableItem.h"
#include "Actors/Equipment/Weapons/Bow.h"
#include "Actors/Equipment/Weapons/MeleeWeaponItem.h"
#include "Inventory/Items/Equipables/InventoryAmmoItem.h"
#include "Net/UnrealNetwork.h"
#include "UI/Equipment/EquipmentViewWidget.h"
#include "UI/Equipment/WeaponWheelWidget.h"

UCharacterEquipmentComponent::UCharacterEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCharacterEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCharacterEquipmentComponent, CurrentEquippedSlot);
	DOREPLIFETIME(UCharacterEquipmentComponent, AmunitionArray);
	DOREPLIFETIME(UCharacterEquipmentComponent, ItemsArray);
}

EEquipableItemType UCharacterEquipmentComponent::GetCurrentEquippedItemType() const
{
	EEquipableItemType Result = EEquipableItemType::None;
	if (IsValid(CurrentEquippedItem))
	{
		Result = CurrentEquippedItem->GetItemType();
	}
	return Result;
}

ARangeWeaponItem* UCharacterEquipmentComponent::GetCurrentRangeWeapon() const
{
	return CurrentEquippedWeapon;
}

AThowableItem* UCharacterEquipmentComponent::GetCurrentThrowableItem() const
{
	return CurrentThrowableItem;
}

void UCharacterEquipmentComponent::ReloadCurrentWeapon()
{
	check(IsValid(CurrentEquippedWeapon));
	int32 AvailableAmunition = GetAvailableAmunitionForCurrentWeapon();
	if (AvailableAmunition <= 0)
	{
		return;
	}
	CurrentEquippedWeapon->StartReload();	
}

void UCharacterEquipmentComponent::AddAmmoToAmunition(int32 Ammo, EAmunitionType AmunitionType)
{
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
	{
		return;
	}
	ChangeAmunitionAmount(AmunitionType, Ammo);
}

void UCharacterEquipmentComponent::EquipItemInSlot(EEquipmentSlots Slot)
{	
	if (bIsEquipping)
	{
		return;
	}	

	UnequipCurrentItem();		
	CurrentEquippedItem = ItemsArray[(uint32)Slot];
	CurrentEquippedWeapon = Cast<ARangeWeaponItem>(CurrentEquippedItem);
	CurrentThrowableItem = Cast<AThowableItem>(CurrentEquippedItem);
	CurrentMeleeWeapon = Cast<AMeleeWeaponItem>(CurrentEquippedItem);

	if (Slot == EEquipmentSlots::PrimaryItemSlot && CurrentThrowableItem->GetItemAmount() == 0)
	{
		return;
	}

	if (IsValid(CurrentEquippedItem))
	{
		UAnimMontage* EquipMontage = CurrentEquippedItem->GetCharacterEquipAnimMontage();
		if (IsValid(EquipMontage))
		{
			bIsEquipping = true;
			UAnimInstance* CharacterAnimInstance = CachedBaseCharacter->GetMesh()->GetAnimInstance();
			float EquipDuration = CharacterAnimInstance->Montage_Play(EquipMontage, 1.0f, EMontagePlayReturnType::Duration);
			GetWorld()->GetTimerManager().SetTimer(EquipTimer, this, &UCharacterEquipmentComponent::EquipAnimationFinished, EquipDuration, false);
		}
		else
		{
			AttachCurrentItemToEquippedSocket();
		}		
		
		CurrentEquippedItem->Equip();
	}		

	if (IsValid(CurrentThrowableItem))
	{
		OnCurrentThrowablesAmountChangedHandle = CurrentThrowableItem->OnItemAmountChanged.AddUFunction(this, FName("OnCurrentThrowablesAmountChanged"));
	}

	if (IsValid(CurrentEquippedWeapon))
	{
		OnCurrentWeaponAmmoChangedHandle = CurrentEquippedWeapon->OnAmmoChanged.AddUFunction(this, FName("OnCurrentWeaponAmmoChanged"));
		OnCurrentWeaponReloadedHandle = CurrentEquippedWeapon->OnReloadComplete.AddUFunction(this, FName("OnWeaponReloadComplete"));
		OnCurrentWeaponAmmoChanged(CurrentEquippedWeapon->GetAmmo());
		CachedBaseCharacter->SetCurrentAimingMovementSpeed(CurrentEquippedWeapon->GetAimMovementMaxSpeed());
	}

	if (OnEquippedItemChanged.IsBound())
	{
		OnEquippedItemChanged.Broadcast(CurrentEquippedItem);
	}

	CurrentEquippedSlot = Slot;

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_EquipItemInSlot(CurrentEquippedSlot);
	}
}

void UCharacterEquipmentComponent::AttachCurrentItemToEquippedSocket()
{
	if (IsValid(CurrentEquippedItem))
	{
		CurrentEquippedItem->AttachToComponent(CachedBaseCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, CurrentEquippedItem->GetEquippedSocketName());
	}
}

bool UCharacterEquipmentComponent::IsEquipping() const
{
	return bIsEquipping;
}

void UCharacterEquipmentComponent::EquipAnimationFinished()
{
	bIsEquipping = false;
	AttachCurrentItemToEquippedSocket();
}

void UCharacterEquipmentComponent::OnCurrentThrowablesAmountChanged(int32 Amount)
{
	if (OnCurrentThrowablesAmountChangedEvent.IsBound())
	{
		OnCurrentThrowablesAmountChangedEvent.Broadcast(Amount);
	}
}

void UCharacterEquipmentComponent::SetThrowablesAmountToItem(class AEquipableItem* Item)
{
	AThowableItem* CurrentItem = StaticCast<AThowableItem*>(Item);
	if (IsValid(CurrentItem))
	{
		OnCurrentThrowablesAmountChangedHandle = CurrentItem->OnItemAmountChanged.AddUFunction(this, FName("OnCurrentThrowablesAmountChanged"));
		CurrentItem->SetItemAmount(GetAmunitionAmount(EAmunitionType::FragGrenades));
	}
}

void UCharacterEquipmentComponent::ChangeAmunitionAmount(EAmunitionType AmunitionType, int32 AmountDelta)
{
	AmunitionArray[(uint32)AmunitionType] += AmountDelta;
	UCharacterInventoryComponent* CharacterInventory = CachedBaseCharacter->GetCharacterInventoryComponent_Mutable();	
	if (IsValid(CharacterInventory) && AmmoItemsInInventory.Num() != 0)
	{
		UInventoryItem* AmmoItem = StaticCast<UInventoryItem*>(AmmoItemsInInventory[(uint32)AmunitionType].Get());
		CharacterInventory->ChangeItemCount(AmmoItem, AmunitionArray[(uint32)AmunitionType]);
	}
	if (IsValid(GetCurrentRangeWeapon()))
	{
		OnCurrentWeaponAmmoChanged(GetCurrentRangeWeapon()->GetAmmo());
	}	
}

void UCharacterEquipmentComponent::LaunchCurrentThrowableItem()
{	
	if (CurrentThrowableItem && CurrentThrowableItem->GetItemAmount() > 0)
	{		
		CurrentThrowableItem->Throw();		
		CurrentThrowableItem->SetItemAmount(CurrentThrowableItem->GetItemAmount() - 1);
		bIsEquipping = false;
		EquipItemInSlot(PreviousEquippedSlot);
	}
}

AMeleeWeaponItem* UCharacterEquipmentComponent::GetCurrentMeleeWeapon() const
{
	return CurrentMeleeWeapon;
}

int32 UCharacterEquipmentComponent::GetAmunitionAmount(EAmunitionType AmunitionType) const
{	
	return AmunitionArray[(uint32)AmunitionType];
}

bool UCharacterEquipmentComponent::AddEquipmentItemToSlot(const TSubclassOf<AEquipableItem>& EquipableItemClass, int32 SlotIndex)
{
	if (!IsValid(EquipableItemClass))
	{
		return false;
	}

	AEquipableItem* DefaultItemObject = EquipableItemClass->GetDefaultObject<AEquipableItem>();
	if (!DefaultItemObject->IsSlotCompatible((EEquipmentSlots)SlotIndex))
	{
		return false;
	}

	if (!IsValid(ItemsArray[SlotIndex]))
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = CachedBaseCharacter.Get();
		AEquipableItem* Item = GetWorld()->SpawnActor<AEquipableItem>(EquipableItemClass, SpawnParameters);
		Item->AttachToComponent(CachedBaseCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Item->GetUnequippedSocketName());
		
		Item->Unequip();
		ItemsArray[SlotIndex] = Item;
		if (Item->GetItemType() == EEquipableItemType::Thowable)
		{
			SetThrowablesAmountToItem(Item);
		}
	}
	else if (DefaultItemObject->IsA<ARangeWeaponItem>())
	{
		ARangeWeaponItem* RangeWeaponObject = StaticCast<ARangeWeaponItem*>(DefaultItemObject);
		int32 AmmoSlotIndex = (int32)RangeWeaponObject->GetAmmoType();
		ChangeAmunitionAmount((EAmunitionType)SlotIndex, RangeWeaponObject->GetMaxAmmo());
	}
	return true;
}

void UCharacterEquipmentComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	if ((uint32)CurrentEquippedSlot == SlotIndex)
	{
		UnequipCurrentItem();
	}
	ItemsArray[SlotIndex]->Destroy();
	ItemsArray[SlotIndex] = nullptr;
}

void UCharacterEquipmentComponent::OpenViewEquipment(APlayerController* PlayerController)
{
	if (!IsValid(ViewWidget))
	{
		CreateEquipmentWidgets(PlayerController);
	}

	if (!ViewWidget->IsVisible())
	{
		ViewWidget->AddToViewport();
	}
}

void UCharacterEquipmentComponent::CloseViewEquipment()
{
	if (ViewWidget->IsVisible())
	{
		ViewWidget->RemoveFromParent();
	}
}

bool UCharacterEquipmentComponent::IsViewVisible() const
{
	bool Result = false;
	if (IsValid(ViewWidget))
	{
		Result = ViewWidget->IsVisible();
	}
	return Result;
}

const TArray<AEquipableItem*>& UCharacterEquipmentComponent::GetItems() const
{
	return ItemsArray;
}

void UCharacterEquipmentComponent::OpenWeaponWheel(APlayerController* PlayerController)
{
	if (!IsValid(WeaponWheelWidget))
	{
		CreateEquipmentWidgets(PlayerController);
	}
	if (!WeaponWheelWidget->IsVisible())
	{
		WeaponWheelWidget->AddToViewport();
	}
}

bool UCharacterEquipmentComponent::IsSelectingWeapon() const
{
	if (IsValid(WeaponWheelWidget))
	{
		return WeaponWheelWidget->IsVisible();
	}
	return false;
}

void UCharacterEquipmentComponent::ConfirmWeaponSelection() const
{
	WeaponWheelWidget->ConfirmSelection();
}

void UCharacterEquipmentComponent::OnLevelDeserialized_Implementation()
{
	EquipItemInSlot(CurrentEquippedSlot);
}

void UCharacterEquipmentComponent::UnequipCurrentItem()
{
	if (IsValid(CurrentEquippedItem))
	{
		CurrentEquippedItem->AttachToComponent(CachedBaseCharacter->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, CurrentEquippedItem->GetUnequippedSocketName());
		CurrentEquippedItem->Unequip();
	}
	if (IsValid(CurrentEquippedWeapon))
	{
		CurrentEquippedWeapon->StopFire();
		CurrentEquippedWeapon->EndReload(false);
		CurrentEquippedWeapon->OnAmmoChanged.Remove(OnCurrentWeaponAmmoChangedHandle);
		CurrentEquippedWeapon->OnReloadComplete.Remove(OnCurrentWeaponReloadedHandle);
	}
	if (IsValid(CurrentThrowableItem))
	{
		CurrentThrowableItem->OnItemAmountChanged.Remove(OnCurrentThrowablesAmountChangedHandle);
	}
	PreviousEquippedSlot = CurrentEquippedSlot;
	CurrentEquippedSlot = EEquipmentSlots::None;
}

void UCharacterEquipmentComponent::EquipNextItem()
{
	if (CachedBaseCharacter->IsPlayerControlled())
	{
		if (IsSelectingWeapon())
		{
			WeaponWheelWidget->NextSegment();
		}
		else
		{
			APlayerController* PlayerController = CachedBaseCharacter->GetController<APlayerController>();
			OpenWeaponWheel(PlayerController);
		}
		return;
	}

	uint32 CurrentSlotIndex = (uint32)CurrentEquippedSlot;
	uint32 NextSlotIndex = NextItemsArraySlotIndex(CurrentSlotIndex);	

	while (CurrentSlotIndex != NextSlotIndex 
			&& IgnoreSlotsWhileSwitching.Contains((EEquipmentSlots)NextSlotIndex)
			&& IsValid(ItemsArray[NextSlotIndex]))
	{
		NextSlotIndex = NextItemsArraySlotIndex(NextSlotIndex);
	}
	if (CurrentSlotIndex != NextSlotIndex)
	{
		EquipItemInSlot((EEquipmentSlots)NextSlotIndex);
	}
}

void UCharacterEquipmentComponent::EquipPreviousItem()
{
	if (CachedBaseCharacter->IsPlayerControlled())
	{
		if (IsSelectingWeapon())
		{
			WeaponWheelWidget->PreviousSegment();
		}
		else
		{
			APlayerController* PlayerController = CachedBaseCharacter->GetController<APlayerController>();
			OpenWeaponWheel(PlayerController);
		}
		return;
	}
	uint32 CurrentSlotIndex = (uint32)CurrentEquippedSlot;
	uint32 PreviousSlotIndex = PreviousItemsArraySlotIndex(CurrentSlotIndex);

	while (CurrentSlotIndex != PreviousSlotIndex 
			&& IgnoreSlotsWhileSwitching.Contains((EEquipmentSlots)PreviousSlotIndex)
			&& IsValid(ItemsArray[PreviousSlotIndex]))
	{
		PreviousSlotIndex = PreviousItemsArraySlotIndex(PreviousSlotIndex);
	}
	if (CurrentSlotIndex != PreviousSlotIndex)
	{
		EquipItemInSlot((EEquipmentSlots)PreviousSlotIndex);
	}
}

void UCharacterEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->IsA<AGCBaseCharacter>(), TEXT("UCharacterEquipmentComponent::BeginPlay UCharacterEquipmentComponent can be used only with GCBaseCharacter"));
	CachedBaseCharacter = StaticCast<AGCBaseCharacter*>(GetOwner());
	CreateLoadout();	
	AutoEquip();

	SetThrowablesAmountToItem(ItemsArray[(uint32)EEquipmentSlots::PrimaryItemSlot]);
}

void UCharacterEquipmentComponent::CreateEquipmentWidgets(APlayerController* PlayerController)
{
	checkf(IsValid(ViewWidgetClass), TEXT("UCharacterEquipmentComponent::CreateEquipmentWidgets view widget class is not defined"));

	if (!IsValid(PlayerController))
	{
		return;
	}

	ViewWidget = CreateWidget<UEquipmentViewWidget>(PlayerController, ViewWidgetClass);
	ViewWidget->InitializeEquipmentWidget(this);

	WeaponWheelWidget = CreateWidget<UWeaponWheelWidget>(PlayerController, WeaponWheelClass);
	WeaponWheelWidget->InitializeWeaponWheelWidget(this);
}

void UCharacterEquipmentComponent::Server_EquipItemInSlot_Implementation(EEquipmentSlots Slot)
{
	EquipItemInSlot(Slot);
}

void UCharacterEquipmentComponent::Multicast_ChangeAmmoAmount_Implementation(EAmunitionType AmunitionType,
	int32 AmountDelta)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ChangeAmunitionAmount(AmunitionType, AmountDelta);
	}	
}

void UCharacterEquipmentComponent::Server_ChangeAmmoAmount_Implementation(EAmunitionType AmunitionType,
	int32 AmountDelta)
{
	Multicast_ChangeAmmoAmount(AmunitionType, AmountDelta);
	ChangeAmunitionAmount(AmunitionType, AmountDelta);
}

void UCharacterEquipmentComponent::Server_ReloadAmmoInCurrentWeapon_Implementation(int32 NumberOfAmmo,
	bool bCheckIsFull)
{
	Multicast_ReloadAmmoInCurrentWeapon(NumberOfAmmo, bCheckIsFull);
	ReloadAmmoInCurrentWeapon(NumberOfAmmo, bCheckIsFull);
}

void UCharacterEquipmentComponent::Multicast_ReloadAmmoInCurrentWeapon_Implementation(int32 NumberOfAmmo,
	bool bCheckIsFull)
{
	if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ReloadAmmoInCurrentWeapon(NumberOfAmmo, bCheckIsFull);
	}
}

void UCharacterEquipmentComponent::OnRep_ItemsArray()
{
	for (AEquipableItem* Item : ItemsArray)
	{
		if (IsValid(Item))
		{
			Item->Unequip();
		}
	}
}

void UCharacterEquipmentComponent::OnWeaponReloadComplete()
{
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Server_ReloadAmmoInCurrentWeapon();
		ReloadAmmoInCurrentWeapon();
	}
	else if (GetOwnerRole() == ROLE_Authority)
	{
		Server_ReloadAmmoInCurrentWeapon();
	}
}


void UCharacterEquipmentComponent::ReloadAmmoInCurrentWeapon(int32 NumberOfAmmo, bool bCheckIsFull)
{
	int32 AvailableAmunition = GetAvailableAmunitionForCurrentWeapon();
	int32 CurrentAmmo = CurrentEquippedWeapon->GetAmmo();
	int32 AmmoToReload = CurrentEquippedWeapon->GetMaxAmmo() - CurrentAmmo;
	int32 ReloadedAmmo = FMath::Min(AvailableAmunition, AmmoToReload);
	if (NumberOfAmmo > 0)
	{
		ReloadedAmmo = FMath::Min(ReloadedAmmo, NumberOfAmmo);
	}	
	
	CurrentEquippedWeapon->SetAmmo(ReloadedAmmo + CurrentAmmo);

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Server_ChangeAmmoAmount(CurrentEquippedWeapon->GetAmmoType(), -ReloadedAmmo);
		ChangeAmunitionAmount(CurrentEquippedWeapon->GetAmmoType(), -ReloadedAmmo);	
	}
	else if (GetOwnerRole() == ROLE_Authority)
	{
		Server_ChangeAmmoAmount(CurrentEquippedWeapon->GetAmmoType(), -ReloadedAmmo);
	}

	if (bCheckIsFull)
	{
		AvailableAmunition = AmunitionArray[(uint32)CurrentEquippedWeapon->GetAmmoType()];
		bool bIsFullyReloaded = CurrentEquippedWeapon->GetAmmo() == CurrentEquippedWeapon->GetMaxAmmo();
		if (AvailableAmunition == 0 || bIsFullyReloaded)
		{
			CurrentEquippedWeapon->EndReload(true);
		}
	}
}

void UCharacterEquipmentComponent::CreateLoadout()
{
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
	{
		return;
	}
	AmmoItemsInInventory.AddZeroed((uint32)EAmunitionType::MAX);
	AmunitionArray.AddZeroed((uint32)EAmunitionType::MAX);
	for (const TPair<EAmunitionType, int32>& AmmoPair : MaxAmunitionAmount)
	{
		int32 AmunitionAmount = FMath::Max(AmmoPair.Value, 0);
		TWeakObjectPtr<UInventoryAmmoItem> AmmoItem = NewObject<UInventoryAmmoItem>();		
		AmmoItem->SetAmmoAmount(AmunitionAmount);
		AmmoItem->SetAmunitionType(AmmoPair.Key);

		AmmoItem->SetDataTableID(AmmoItemDataTableIDs[(uint32)AmmoPair.Key - 1]);
		AmmoItemsInInventory[(uint32)AmmoPair.Key] = AmmoItem;
		CachedBaseCharacter->GetCharacterInventoryComponent_Mutable()->AddInitialAmmoItems(AmmoItem.Get());
	}

	ItemsArray.AddZeroed((uint32)EEquipmentSlots::MAX);
	for (const TPair<EEquipmentSlots, TSubclassOf<AEquipableItem>>& ItemPair : ItemsLoadout)
	{
		if (!IsValid(ItemPair.Value))
		{
			continue;
		}
		AddEquipmentItemToSlot(ItemPair.Value, (int32)ItemPair.Key);
	}	
}

void UCharacterEquipmentComponent::AutoEquip()
{
	if (AutoEquipItemInSlot != EEquipmentSlots::None)
	{
		EquipItemInSlot(AutoEquipItemInSlot);
	}
}

void UCharacterEquipmentComponent::OnRep_CurrentEquippedSlot(EEquipmentSlots CurrentEquippedSlot_Old)
{
	EquipItemInSlot(CurrentEquippedSlot);
}

void UCharacterEquipmentComponent::OnCurrentWeaponAmmoChanged(int32 Ammo)
{
	if (OnCurrentWeaponAmmoChangedEvent.IsBound())
	{
		OnCurrentWeaponAmmoChangedEvent.Broadcast(Ammo, GetAvailableAmunitionForCurrentWeapon());
	}
}

int32 UCharacterEquipmentComponent::GetAvailableAmunitionForCurrentWeapon()
{
	check(GetCurrentRangeWeapon());
	return AmunitionArray[(uint32)GetCurrentRangeWeapon()->GetAmmoType()];
}

uint32 UCharacterEquipmentComponent::NextItemsArraySlotIndex(uint32 CurrentSlotIndex)
{
	if (CurrentSlotIndex == ItemsArray.Num() - 1)
	{
		return 0;
	}
	else
	{
		return CurrentSlotIndex + 1;
	}
}

uint32 UCharacterEquipmentComponent::PreviousItemsArraySlotIndex(uint32 CurrentSlotIndex)
{
	if (CurrentSlotIndex == 0)
	{
		return ItemsArray.Num() - 1;
	}
	else
	{
		return CurrentSlotIndex - 1;
	}
}

