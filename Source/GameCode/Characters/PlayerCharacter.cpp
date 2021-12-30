// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "../Components/MovementComponents/GCBaseCharacterMovementComponent.h"
#include "GCBaseCharacter.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Subsystems/Streaming/StreamingSubsystemUtils.h"

APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring arm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->bUsePawnControlRotation = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;

	GetCharacterMovement()->bOrientRotationToMovement = 1;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	Team = ETeams::Player;
}

void APlayerCharacter::MoveForward(float Value)
{
	Super::MoveForward(Value);
	if ((GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling()) && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator YawRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		FVector ForwardVector = YawRotator.RotateVector(FVector::ForwardVector);
		AddMovementInput(ForwardVector, Value);		
	}
}

void APlayerCharacter::MoveRight(float Value)
{
	Super::MoveRight(Value);
	if ((GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling()) && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator YawRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		FVector RightVector = YawRotator.RotateVector(FVector::RightVector);
		AddMovementInput(RightVector, Value);		
	}
}

void APlayerCharacter::Turn(float Value)
{
	float TurnModifier = 1.0f;
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();	
	if (IsValid(CurrentRangeWeapon) && bIsAiming)
	{
		TurnModifier = CurrentRangeWeapon->GetAimTurnModifier();
	}
	if (!FMath::IsNearlyZero(Value) && IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopRecoilRollback();
	}
	AddControllerYawInput(Value * TurnModifier);
}

void APlayerCharacter::TurnAtRate(float Value)
{
	float TurnModifier = 1.0f;
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon) && bIsAiming)
	{
		TurnModifier = CurrentRangeWeapon->GetAimTurnModifier();
	}
	if (!FMath::IsNearlyZero(Value) && IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopRecoilRollback();
	}
	AddControllerYawInput(Value * TurnModifier * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::LookUp(float Value)
{
	float LookUpModifier = 1.0f;
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon) && bIsAiming)
	{
		LookUpModifier = CurrentRangeWeapon->GetAimLookUpModifier();
	}
	if (!FMath::IsNearlyZero(Value) && IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopRecoilRollback();
	}
	AddControllerPitchInput(Value * LookUpModifier);
}

void APlayerCharacter::LookUpAtRate(float Value)
{
	float LookUpModifier = 1.0f;
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon) && bIsAiming)
	{
		LookUpModifier = CurrentRangeWeapon->GetAimLookUpModifier();
	}
	if (!FMath::IsNearlyZero(Value) && IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopRecoilRollback();
	}
	AddControllerPitchInput(Value * LookUpModifier * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void APlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	SpringArmComponent->TargetOffset += FVector(0.0f, 0.0f, HalfHeightAdjust);
}

void APlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	SpringArmComponent->TargetOffset -= FVector(0.0f, 0.0f, HalfHeightAdjust);
}

void APlayerCharacter::SwimForward(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator PitchYawRotator(GetControlRotation().Pitch, GetControlRotation().Yaw, 0.0f);
		FVector ForwardVector = PitchYawRotator.RotateVector(FVector::ForwardVector);
		AddMovementInput(ForwardVector, Value);
	}
}

void APlayerCharacter::SwimRight(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f))
	{
		FRotator YawRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		FVector RightVector = YawRotator.RotateVector(FVector::RightVector);
		AddMovementInput(RightVector, Value);
	}
}

void APlayerCharacter::SwimUp(float Value)
{
	if (GetCharacterMovement()->IsSwimming() && !FMath::IsNearlyZero(Value, 1e-6f) && !FMath::IsNearlyZero(ForwardMovingInput, 1e-6f))
	{		
		AddMovementInput(FVector::UpVector, Value);
	}
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FOVChangingTimeline.TickTimeline(DeltaTime);
}

void APlayerCharacter::OnStartAimingInternal()
{
	Super::OnStartAimingInternal();
	APlayerController* PlayerController = GetController<APlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (IsValid(CameraManager))
	{
		ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();		
		if (IsValid(FOVChangingCurve))
		{
			FOnTimelineFloat TimelineProgress;
			TimelineProgress.BindUFunction(this, FName("FOVChangingTimelineProgress"));
			FOVChangingTimeline.AddInterpFloat(FOVChangingCurve, TimelineProgress);			
			FOVChangingTimeline.PlayFromStart();			
		}
		else
		{
			CameraManager->SetFOV(CurrentRangeWeapon->GetAimFOV());
		}				
	}
}

void APlayerCharacter::OnStopAimingInternal()
{
	Super::OnStopAimingInternal();
	APlayerController* PlayerController = GetController<APlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}
	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	if (IsValid(CameraManager))
	{
		ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
		if (IsValid(FOVChangingCurve))
		{
			FOnTimelineFloat TimelineProgress;
			TimelineProgress.BindUFunction(this, FName("FOVChangingTimelineProgress"));
			FOVChangingTimeline.AddInterpFloat(FOVChangingCurve, TimelineProgress);			
			FOVChangingTimeline.Reverse();
		}
		else
		{
			CameraManager->UnlockFOV();
		}		
	}
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	UStreamingSubsystemUtils::CheckCharacterOverlapStreamingSubsystemVolume(this);
}

void APlayerCharacter::FOVChangingTimelineProgress(float Value)
{
	APlayerController* PlayerController = GetController<APlayerController>();	
	if (!IsValid(PlayerController))
	{
		return;
	}	

	APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();

	if (!IsValid(CameraManager) || !IsValid(CurrentRangeWeapon))
	{
		return;
	}

	float Alpha = FOVChangingCurve->GetFloatValue(FOVChangingTimeline.GetPlaybackPosition());
	float NewFOV = FMath::Lerp(CameraManager->DefaultFOV, CurrentRangeWeapon->GetAimFOV(), Alpha);	
	
	CameraManager->SetFOV(NewFOV);
}

bool APlayerCharacter::CanJumpInternal_Implementation() const
{	
	if (GCBaseCharacterMovementComponent->IsOutOfStamina() || GCBaseCharacterMovementComponent->IsMantling())
	{
		return false;
	}
	return bIsCrouched || Super::CanJumpInternal_Implementation();
}

void APlayerCharacter::OnJumped_Implementation()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
}
