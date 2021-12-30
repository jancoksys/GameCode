// Fill out your copyright notice in the Description page of Project Settings.


#include "PickableItem.h"

APickableItem::APickableItem()
{
	SetReplicates(true);
}

const FName& APickableItem::GetDataTableID() const
{
	return DataTableID;
}

void APickableItem::SetPickablePoolIndex(int32 Index_In)
{
	PickablePoolIndex = Index_In;
}
