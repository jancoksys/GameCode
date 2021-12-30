// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterAttributesWidget.generated.h"

/**
 * 
 */
UCLASS()
class GAMECODE_API UCharacterAttributesWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float HealthPercent = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Health")
	ESlateVisibility IsHealthBarVisible() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina")
	float StaminaPercent = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Stamina")
	ESlateVisibility IsStaminaBarVisible() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen")
	float OxygenPercent = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "Oxygen")
	ESlateVisibility IsOxygenBarVisible() const;

private:
	UFUNCTION()
	void UpdateHealthPercent(float NewHealthPercent);

	UFUNCTION()
	void UpdateStaminaPercent(float NewStaminaPercent);

	UFUNCTION()
	void UpdateOxygenPercent(float NewOxygenPercent);
};