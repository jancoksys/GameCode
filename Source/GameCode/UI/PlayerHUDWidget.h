// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

class UHighlightInteractable;

UCLASS()
class GAMECODE_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	class UReticleWidget* GetReticleWidget();
	class UAmmoWidget* GetAmmoWidget();
	class UCharacterAttributesWidget* GetCharacterAttributesWidget();

	void SetHighlightInteractableVisibility(bool bIsVisible);
	void SetHighlightInteractableActionText(FName KeyName);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget names")
	FName ReticleWidgetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget names")
	FName AmmoWidgetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget names")
	FName CharacterAttributesWidgetName;

	UPROPERTY(meta = (BindWidget))
	UHighlightInteractable* InteractableKey;
};
