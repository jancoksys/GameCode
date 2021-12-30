// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryAmmoItem.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"

EAmunitionType UInventoryAmmoItem::GetAmunitionType() const
{
	return AmunitionType;
}

int32 UInventoryAmmoItem::GetAmmoAmount() const
{
	return AmmoToAdd;
}

void UInventoryAmmoItem::SetDataTableID(const FName& DataTableID_In)
{
	DataTableID = DataTableID_In;
}

void UInventoryAmmoItem::SetAmmoAmount(int32 AmmoAmount_In)
{
	AmmoToAdd = AmmoAmount_In;
}

void UInventoryAmmoItem::SetAmunitionType(EAmunitionType AmunitionType_In)
{
	AmunitionType = AmunitionType_In;
}