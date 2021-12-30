// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterAttributesWidget.h"

ESlateVisibility UCharacterAttributesWidget::IsHealthBarVisible() const
{
	if (HealthPercent < 1.0f)
	{
		return ESlateVisibility::Visible;
	}
	return ESlateVisibility::Hidden;
}

ESlateVisibility UCharacterAttributesWidget::IsStaminaBarVisible() const
{
	if (StaminaPercent < 1.0f)
	{
		return ESlateVisibility::Visible;
	}
	return ESlateVisibility::Hidden;
}

ESlateVisibility UCharacterAttributesWidget::IsOxygenBarVisible() const
{
	if (OxygenPercent < 1.0f)
	{
		return ESlateVisibility::Visible;
	}
	return ESlateVisibility::Hidden;
}

void UCharacterAttributesWidget::UpdateHealthPercent(float NewHealthPercent)
{
	HealthPercent = NewHealthPercent;
}

void UCharacterAttributesWidget::UpdateStaminaPercent(float NewStaminaPercent)
{
	StaminaPercent = NewStaminaPercent;
}

void UCharacterAttributesWidget::UpdateOxygenPercent(float NewOxygenPercent)
{
	OxygenPercent = NewOxygenPercent;
}