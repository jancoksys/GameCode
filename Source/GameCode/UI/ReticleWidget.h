// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameCodeTypes.h"
#include "ReticleWidget.generated.h"

class UProgressBar;
/**
 * 
 */
class AEquipableItem;

UCLASS()
class GAMECODE_API UReticleWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintNativeEvent)
	void OnAimingStateChanged(bool bIsAiming);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reticle")
	EReticleType CurrentReticle = EReticleType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float DamagePercent = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Damage")
	ESlateVisibility IsDamageBarVisible() const;

	UFUNCTION(BlueprintNativeEvent)
	void OnEquippedItemChanged(const AEquipableItem* EquippedItem);

private:
	TWeakObjectPtr<const AEquipableItem> CurrentEquippedItem;
	
	void SetupCurrentReticle();

	UFUNCTION()
	void UpdateDamagePercent(float NewDamagePercent);
};