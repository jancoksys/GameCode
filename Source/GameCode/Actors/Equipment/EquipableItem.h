// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameCodeTypes.h"
#include "EquipableItem.generated.h"

class UAnimMontage;
class AGCBaseCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentStateChanged, bool, bIsEquipped);

UCLASS(Abstract, NotBlueprintable)
class GAMECODE_API AEquipableItem : public AActor
{
	GENERATED_BODY()

public:
	AEquipableItem();

	virtual void SetOwner(AActor* NewOwner) override;

	EEquipableItemType GetItemType() const;

	FName GetUnequippedSocketName() const;
	FName GetEquippedSocketName() const;

	UAnimMontage* GetCharacterEquipAnimMontage() const;

	virtual void Equip();
	virtual void Unequip();

	virtual EReticleType GetReticleType() const;

	FName GetDataTableID() const;

	bool IsSlotCompatible(EEquipmentSlots Slot);

protected:
	UPROPERTY(BlueprintAssignable)
	FOnEquipmentStateChanged OnEquipmentStateChanged;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable item")
	EEquipableItemType ItemType = EEquipableItemType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable item")
	FName UnequippedSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable item")
	FName EquippedSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable item")
	UAnimMontage* CharacterEquipAnimMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippable item")
	TArray<EEquipmentSlots> CompatableEquipmentSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reticle")
	EReticleType ReticleType = EReticleType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName DataTableID = NAME_None;

	AGCBaseCharacter* GetCharacterOwner() const;

private:
	TWeakObjectPtr<AGCBaseCharacter> CachedCharacterOwner;
};
