// Fill out your copyright notice in the Description page of Project Settings.


#include "GCBaseCharacterAnimInstance.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/MovementComponents/GCBaseCharacterMovementComponent.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"

void UGCBaseCharacterAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	checkf(TryGetPawnOwner()->IsA<AGCBaseCharacter>(), TEXT("UGCBaseCharacterAnimInstance::NativeBeginPlay() UGCBaseCharacterAnimInstance can be used only with AGCBaseCharacter"));
	CachedBaseCharacter = StaticCast<AGCBaseCharacter*>(TryGetPawnOwner());
}

void UGCBaseCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!CachedBaseCharacter.IsValid())
	{
		return;
	}
	bIsAiming = CachedBaseCharacter->IsAiming();

	UGCBaseCharacterMovementComponent* CharacterMovement = CachedBaseCharacter->GetBaseCharacterMovementComponent();
	Speed = CharacterMovement->Velocity.Size();
	bIsFalling = CharacterMovement->IsFalling();
	bIsCrouching = CharacterMovement->IsCrouching();
	bIsSprining = CharacterMovement->IsSprinting();
	bIsOutOfStamina = CharacterMovement->IsOutOfStamina();
	bIsProne = CharacterMovement->IsProne();
	bIsSwimming = CharacterMovement->IsSwimming();
	bIsOnZipline = CharacterMovement->IsOnZipline();
	
	bIsWallRunning = CharacterMovement->IsWallRunning();
	if (bIsWallRunning)
	{
		bIsWallRunSideLeft = CharacterMovement->GetCurrentWallRunSide() == EWallRunSide::Left ? true : false;
	}
	
	bIsOnLadder = CharacterMovement->IsOnLadder();	
	if (bIsOnLadder)
	{
		LadderSpeedRatio = CharacterMovement->GetLadderSpeedRatio();
	}

	bIsStrafing = !CharacterMovement->bOrientRotationToMovement;
	Direction = CalculateDirection(CharacterMovement->Velocity, CachedBaseCharacter->GetActorRotation());

	BodyOffset = CachedBaseCharacter->GetIKBodyOffset();		
	if (FMath::Abs(CachedBaseCharacter->GetIKRightFootOffset()) > FMath::Abs(CachedBaseCharacter->GetIKLeftFootOffset()))
	{
		LeftFootEffectorLocation = FVector(-BodyOffset, 0.0f, 0.0f);
		RightFootEffectorLocation = FVector(0.0f, 0.0f, 0.0f);

		LeftFootEffectorLocationInCrouch = FVector(0.0f, 0.0f, 0.0f);
		RightFootEffectorLocationInCrouch = FVector(-BodyOffset, 0.0f, 0.0f);
	}
	else
	{
		LeftFootEffectorLocation = FVector(0.0f, 0.0f, 0.0f);
		RightFootEffectorLocation = FVector(BodyOffset, 0.0f, 0.0f);

		LeftFootEffectorLocationInCrouch = FVector(BodyOffset, 0.0f, 0.0f);
		RightFootEffectorLocationInCrouch = FVector(0.0f, 0.0f, 0.0f);
	}	

	AimRotation = CachedBaseCharacter->GetAimOffset();

	const UCharacterEquipmentComponent* CharacterEquipment = CachedBaseCharacter->GetCharacterEquipmentComponent();
	CurrentEquippedItemType = CharacterEquipment->GetCurrentEquippedItemType();

	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipment->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		ForeGripSocketTransform = CurrentRangeWeapon->GetFABRIKEffectorTransform();
	}
}
