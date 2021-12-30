// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GCPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GAMECODE_API AGCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetPawn(APawn* InPawn) override;

	bool GetIgnoreCameraPitch() const;

	void SetIgnoreCameraPitch(bool bIgnoreCameraPitch_In);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<class UPlayerHUDWidget> PlayerHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);
	void ChangeCrouchState();
	void ChangeProneState();
	void Mantle();
	void Jump();
	void StartSprint();
	void StopSprint();
	void SwimForward(float Value);
	void SwimRight(float Value);
	void SwimUp(float Value);
	void ClimbLadderUp(float Value);
	void InteractWithLadder();
	void InteractWithZipline();
	void Slide();
	void PlayerStartFire();
	void PlayerStopFire();
	void StartAiming();
	void StopAiming();
	void Reload();
	void NextItem();
	void PreviousItem();
	void EquipPrimaryItem();
	void PrimaryMeleeAttack();
	void SecondaryMeleeAttack();
	void ChangeFireMode();
	void Interact();
	void UseInventory();
	void ConfirmWeaponWheelSelection();
	void QuickSaveGame();
	void QuickLoadGame();

	TSoftObjectPtr<class AGCBaseCharacter> CachedBaseCharacter;

	bool bIgnoreCameraPitch = false;

	class UPlayerHUDWidget* PlayerHUDWidget = nullptr;

	void CreateAndInitializeWidgets();

	void ToggleMainMenu();

	UUserWidget* MainMenuWidget = nullptr;

	void OnInteractableObjectFound(FName ActionName);

	UFUNCTION(Server, Reliable)
	void Server_Interact();
};
