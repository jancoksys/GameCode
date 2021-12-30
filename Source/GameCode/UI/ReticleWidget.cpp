// Fill out your copyright notice in the Description page of Project Settings.


#include "ReticleWidget.h"
#include "Actors/Equipment/EquipableItem.h"

void UReticleWidget::OnAimingStateChanged_Implementation(bool bIsAiming)
{
	SetupCurrentReticle();
}

void UReticleWidget::OnEquippedItemChanged_Implementation(const AEquipableItem* EquippedItem)
{
	CurrentEquippedItem = EquippedItem;
	SetupCurrentReticle();
}

ESlateVisibility UReticleWidget::IsDamageBarVisible() const
{
	if (CurrentReticle == EReticleType::Bow)
	{
		if (DamagePercent > 0.0f)
		{
			return ESlateVisibility::Visible;
		}
	}
	return ESlateVisibility::Hidden;
}

void UReticleWidget::SetupCurrentReticle()
{
	CurrentReticle = CurrentEquippedItem.IsValid() ? CurrentEquippedItem->GetReticleType() : EReticleType::None;
}

void UReticleWidget::UpdateDamagePercent(float NewDamagePercent)
{
	DamagePercent = NewDamagePercent;
}
