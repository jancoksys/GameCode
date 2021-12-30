// Fill out your copyright notice in the Description page of Project Settings.


#include "GCBaseCharacterMovementComponent.h"
#include "../../Characters/GCBaseCharacter.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveVector.h"
#include "Actors/Interactive/Ladder.h"
#include "Actors/Interactive/Zipline.h"
#include "GameCodeTypes.h"
#include "Net/UnrealNetwork.h"

void UGCBaseCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	/*
		FLAG_Reserved_1 = 0x04,	// Reserved for future use
		FLAG_Reserved_2 = 0x08,	// Reserved for future use
		// Remaining bit masks are available for custom flags.
		FLAG_Custom_0 = 0x10, - Sprint flag
		FLAG_Custom_1 = 0x20, - mantling
		FLAG_Custom_2 = 0x40, - aiming
		FLAG_Custom_3 = 0x80,
	 */

	bIsAiming = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
	
	bool bWasMantling = GetBaseCharacterOwner()->bIsMantling;
	bIsSprintng = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bool bIsMantling = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;

	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		if (!bWasMantling && bIsMantling)
		{
			GetBaseCharacterOwner()->Mantle(true);
		}
	}
}

FNetworkPredictionData_Client* UGCBaseCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UGCBaseCharacterMovementComponent* MutableThis = const_cast<UGCBaseCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Character_GC(*this);
	}
	return ClientPredictionData;
}

float UGCBaseCharacterMovementComponent::GetMaxSpeed() const
{
	float Result = Super::GetMaxSpeed();
	if (bIsSprintng)
	{
		Result = SprintSpeed;
	}
	else if (bIsOutOfStamina)
	{
		Result = OutOfStaminaSpeed;
	}
	else if (bIsProne)
	{
		Result = MaxProneSpeed;
	}
	else if (IsOnLadder())
	{
		Result = ClimbingLadderMaxSpeed;
	}	
	else if (IsOnZipline())
	{
		Result = ZiplineSpeed;
	}
	else if (bIsWallRunning)
	{
		Result = MaxWallRunSpeed;
	}
	else if (bIsAiming)
	{
		Result = GetBaseCharacterOwner()->GetAimingMovementSpeed();
	}	
	return Result;
}

void UGCBaseCharacterMovementComponent::StartSprint()
{
	float Placeholder;
	if (IsCrouching() && 
		!CanIncreaseCapsuleHalfHeight(GetBaseCharacterOwner()->GetDefaultHalfHeight(), GetBaseCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), Placeholder))
	{
		GetBaseCharacterOwner()->StopSprint();
		GetBaseCharacterOwner()->OnSprintEnd();
		return;
	}
	ApplyContextMovementSettings(SprintMovementSettings);
	bIsSprintng = true;
	bForceMaxAccel = 1;
}

void UGCBaseCharacterMovementComponent::StopSprint()
{	
	RemoveContextMovementSettings(SprintMovementSettings);
	bIsSprintng = false;
	bForceMaxAccel = 0;
}

void UGCBaseCharacterMovementComponent::SetIsAiming(bool bIsAiming_In)
{	
	bIsAiming = bIsAiming_In;
}

void UGCBaseCharacterMovementComponent::SetIsOutOfStamina(bool bIsOutOfStamina_In)
{
	bIsOutOfStamina = bIsOutOfStamina_In;
	if (bIsOutOfStamina)
	{
		ApplyContextMovementSettings(OutOfStaminaMovementSettings);	
	}
	else
	{
		RemoveContextMovementSettings(OutOfStaminaMovementSettings);
	}
}

void UGCBaseCharacterMovementComponent::Crouch(bool bClientSimulation)
{
	Super::Crouch(bClientSimulation);
	ApplyContextMovementSettings(CrouchMovementSettings);
}

void UGCBaseCharacterMovementComponent::UnCrouch(bool bClientSimulation /* = false */)
{
	Super::UnCrouch(bClientSimulation);
	RemoveContextMovementSettings(CrouchMovementSettings);
}

void UGCBaseCharacterMovementComponent::SetWantsToProne(bool bWantsToProne_In)
{
	bWantsToProne = bWantsToProne_In;
}

void UGCBaseCharacterMovementComponent::Prone()
{	
	ApplyContextMovementSettings(ProneMovementSettings);
	bIsProne = true;
	bWantsToCrouch = false;	
	AGCBaseCharacter* GCBaseCharacterOwner = Cast<AGCBaseCharacter>(CharacterOwner);
	ACharacter* DefaultCharacter = GCBaseCharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());

	const float ComponentScale = GCBaseCharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = GCBaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float OldUnscaledRadius = GCBaseCharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius();

	GCBaseCharacterOwner->GetCapsuleComponent()->SetCapsuleSize(ProneCapsuleRadius, ProneCapsuleHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - ProneCapsuleHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -ScaledHalfHeightAdjust), UpdatedComponent->GetComponentQuat(), true, nullptr, EMoveComponentFlags::MOVECOMP_NoFlags, ETeleportType::TeleportPhysics);
		
	HalfHeightAdjust = (DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - ProneCapsuleHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
		
	GCBaseCharacterOwner->OnStartProne(HalfHeightAdjust, ScaledHalfHeightAdjust);			
}

void UGCBaseCharacterMovementComponent::UnProne(bool bWantsToStand)
{		
	RemoveContextMovementSettings(ProneMovementSettings);
	ACharacter* DefaultCharacter = GetBaseCharacterOwner()->GetClass()->GetDefaultObject<ACharacter>();
	
	const float CurrentProneHalfHeight = GetBaseCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float StandingCapsuleHalfHeight = DefaultCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float ComponentScale = GetBaseCharacterOwner()->GetCapsuleComponent()->GetShapeScale();
	float HalfHeightAdjust;

	if (!CanIncreaseCapsuleHalfHeight(StandingCapsuleHalfHeight, CurrentProneHalfHeight, HalfHeightAdjust))
	{
		bWantsToProne = true;		
		return;
	}	

	if (bWantsToStand)
	{
		GetBaseCharacterOwner()->GetCapsuleComponent()->SetCapsuleHalfHeight(StandingCapsuleHalfHeight);
		GetBaseCharacterOwner()->OnStandAfterProne(HalfHeightAdjust, HalfHeightAdjust * ComponentScale);
		bIsProne = false;
		bWantsToProne = false;
	}
	else
	{
		GetBaseCharacterOwner()->GetCapsuleComponent()->SetCapsuleHalfHeight(CrouchedHalfHeight);
		GetBaseCharacterOwner()->OnEndProne(HalfHeightAdjust, HalfHeightAdjust * ComponentScale);
		bIsProne = false;
	}
}

void UGCBaseCharacterMovementComponent::StartMantle(const FMantlingMovementParameters& MantlingParameters)
{
	CurrentMantlingParameters = MantlingParameters;
	SetMovementMode(EMovementMode::MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Mantling);
}

void UGCBaseCharacterMovementComponent::EndMantle()
{
	GetBaseCharacterOwner()->bIsMantling = false;
	SetMovementMode(MOVE_Walking);
}

bool UGCBaseCharacterMovementComponent::IsMantling() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Mantling;
}

void UGCBaseCharacterMovementComponent::AttachToLadder(const class ALadder* Ladder)
{
	CurrentLadder = Ladder;	
	FRotator TargetOrientationRotation = CurrentLadder->GetActorForwardVector().ToOrientationRotator();
	TargetOrientationRotation.Yaw += 180.0f;
	
	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	FVector LadderForwardVector = CurrentLadder->GetActorForwardVector();
	float Projection = GetActorToCurrentLadderProjection(GetActorLocation());

	FVector NewCharacterLocation = CurrentLadder->GetActorLocation() + Projection * LadderUpVector + LadderToCharacterOffset * LadderForwardVector;
	if (CurrentLadder->GetIsOnTop())
	{
		NewCharacterLocation = CurrentLadder->GetAttachFromTopAnimMontageStartingLocation();
	}

	GetOwner()->SetActorLocation(NewCharacterLocation);
	GetOwner()->SetActorRotation(TargetOrientationRotation);	

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Ladder);
}

float UGCBaseCharacterMovementComponent::GetActorToCurrentLadderProjection(const FVector& Location) const
{
	checkf(IsValid(CurrentLadder), TEXT("UGCBaseCharacterMovementComponent::GetCharacterToCurrentLadderProjection() cant be invoked when current ladder is null"));
	FVector LadderUpVector = CurrentLadder->GetActorUpVector();
	FVector LadderToCharacterDistance = Location - CurrentLadder->GetActorLocation();
	return FVector::DotProduct(LadderUpVector, LadderToCharacterDistance);
}

void UGCBaseCharacterMovementComponent::DetachFromLadder(EDetachFromLadderMethod DetachFromLadderMethod)
{
	switch (DetachFromLadderMethod)
	{
	case EDetachFromLadderMethod::JumpOff:
	{
		FVector JumpDirection = CurrentLadder->GetActorForwardVector();
		SetMovementMode(MOVE_Falling);

		FVector JumpVelocity = JumpDirection * JumpOffFromLadderSpeed;

		ForcedTargetRotation = JumpDirection.ToOrientationRotator();
		bForceRotation = true;

		Launch(JumpVelocity);
		break;
	}
	case EDetachFromLadderMethod::ReachingTheTop:
	{
		GetBaseCharacterOwner()->Mantle(true);
		break;
	}
	case EDetachFromLadderMethod::ReachingTheBottom:
	{
		SetMovementMode(MOVE_Walking);
		break;
	}
	case EDetachFromLadderMethod::Fall:
	default:
	{
		SetMovementMode(MOVE_Falling);
	}
		break;
	}
}

bool UGCBaseCharacterMovementComponent::IsOnLadder() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Ladder;
}

const class ALadder* UGCBaseCharacterMovementComponent::GetCurrentLadder()
{
	return CurrentLadder;
}

void UGCBaseCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (bForceRotation)
	{
		FRotator CurrentRotation = UpdatedComponent->GetComponentRotation(); // Normalized
		CurrentRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): CurrentRotation"));

		FRotator DeltaRot = GetDeltaRotation(DeltaTime);
		DeltaRot.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): GetDeltaRotation"));

		// Accumulate a desired new rotation.
		const float AngleTolerance = 1e-3f;

		if (!CurrentRotation.Equals(ForcedTargetRotation, AngleTolerance))
		{
			FRotator DesiredRotation = ForcedTargetRotation;
			// PITCH
			if (!FMath::IsNearlyEqual(CurrentRotation.Pitch, DesiredRotation.Pitch, AngleTolerance))
			{
				DesiredRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
			}

			// YAW
			if (!FMath::IsNearlyEqual(CurrentRotation.Yaw, DesiredRotation.Yaw, AngleTolerance))
			{
				DesiredRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
			}

			// ROLL
			if (!FMath::IsNearlyEqual(CurrentRotation.Roll, DesiredRotation.Roll, AngleTolerance))
			{
				DesiredRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
			}

			// Set the new rotation.
			DesiredRotation.DiagnosticCheckNaN(TEXT("CharacterMovementComponent::PhysicsRotation(): DesiredRotation"));
			MoveUpdatedComponent(FVector::ZeroVector, DesiredRotation, /*bSweep*/ false);
		}
		else
		{
			ForcedTargetRotation = FRotator::ZeroRotator;
			bForceRotation = false;
		}
		return;
	}
	if (IsOnLadder())
	{
		return;
	}
	Super::PhysicsRotation(DeltaTime);
}

float UGCBaseCharacterMovementComponent::GetLadderSpeedRatio() const
{
	checkf(IsValid(CurrentLadder), TEXT("UGCBaseCharacterMovementComponent::GetLadderSpeedRatio() cant be invoked when current ladder is null"));
	FVector LadderUpVector = CurrentLadder->GetActorUpVector();

	return FVector::DotProduct(LadderUpVector, Velocity) / ClimbingLadderMaxSpeed;
}

void UGCBaseCharacterMovementComponent::AttachToZipline(const class AZipline* Zipline)
{				
	checkf(IsValid(Zipline), TEXT("UGCBaseCharacterMovementComponent::AttachToZipline cant be invoked when current zipline is null"));

	ZiplineMovingTime = Zipline->GetZiplineLength() / ZiplineSpeed;	
	ZiplineDirection = Zipline->GetActorRotation().RotateVector(Zipline->GetZiplineCableDirection());	
	
	FVector ZiplinePolesStraightDirection(ZiplineDirection.X, ZiplineDirection.Y, 0.0f);
	FRotator TargetOrientationRotation = ZiplinePolesStraightDirection.ToOrientationRotator();
	GetOwner()->SetActorRotation(TargetOrientationRotation);	

	const UStaticMeshComponent* Cable = Zipline->GetZiplineCable();
	FVector CableDirection = ZiplineDirection;
	CableDirection.Normalize();
	FVector CableToCharacterDistance = GetOwner()->GetActorLocation() - Cable->GetComponentLocation();
	float Projection = FVector::DotProduct(CableToCharacterDistance, CableDirection);
	FVector NewCharacterLocation = Cable->GetComponentLocation() + Projection * CableDirection;
	
	NewCharacterLocation.Z -= (GetBaseCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + ZiplineZAdjustment);
	GetOwner()->SetActorLocation(NewCharacterLocation);

	ZiplineMovingStartLocation = GetOwner()->GetActorLocation();

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Zipline);
}

bool UGCBaseCharacterMovementComponent::IsOnZipline() const
{
	return UpdatedComponent && MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Zipline;
}

void UGCBaseCharacterMovementComponent::DetachFromZipline()
{
	SetMovementMode(MOVE_Falling);
}

void UGCBaseCharacterMovementComponent::StartWallRun(const EWallRunSide& WallRunSide, const FVector& Direction)
{
	bIsWallRunning = true;
	CurrentWallRunSide = WallRunSide;
	CurrentWallRunDirection = Direction;
	StandartJumpZVelocity = JumpZVelocity;
	JumpZVelocity = WallRunJumpZVelocity;
	WallRunStartLocation = GetOwner()->GetActorLocation();

	FRotator WallRunOrientationRotator = CurrentWallRunDirection.ToOrientationRotator();
	GetOwner()->SetActorRotation(WallRunOrientationRotator);	

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_WallRun);
}

void UGCBaseCharacterMovementComponent::EndWallRun()
{
	bIsWallRunning = false;
	JumpZVelocity = StandartJumpZVelocity;	
	SetMovementMode(MOVE_Falling);
}

bool UGCBaseCharacterMovementComponent::IsWallRunning() const
{
	return bIsWallRunning;
}

void UGCBaseCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
	if (bWantsToProne)
	{
		Prone();
	}
	if (bIsProne && !bWantsToProne)
	{
		UnProne(false);
	}
}

const EWallRunSide UGCBaseCharacterMovementComponent::GetCurrentWallRunSide() const
{
	return CurrentWallRunSide;
}

void UGCBaseCharacterMovementComponent::GetWallRunSideAndDirection(const FVector& HitNormal, EWallRunSide& OutSide, FVector& Direction) const
{
	if (FVector::DotProduct(HitNormal, GetOwner()->GetActorRightVector()) > 0)
	{
		OutSide = EWallRunSide::Left;
		Direction = FVector::CrossProduct(HitNormal, FVector::UpVector).GetSafeNormal();
	}
	else
	{
		OutSide = EWallRunSide::Right;
		Direction = FVector::CrossProduct(FVector::UpVector, HitNormal).GetSafeNormal();
	}
}

bool UGCBaseCharacterMovementComponent::IsSliding() const
{
	return bIsSliding;
}

void UGCBaseCharacterMovementComponent::StartSlide()
{
	StopSprint();
	bIsSliding = true;		
	SlidingDirection = GetOwner()->GetActorForwardVector();
	SlidingHorizontalVelocityComponent = SlidingDirection * SlideSpeed;

	const float ComponentScale = GetBaseCharacterOwner()->GetCapsuleComponent()->GetShapeScale();
	const float CurrentHalfHeight = GetBaseCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();	

	GetBaseCharacterOwner()->GetCapsuleComponent()->SetCapsuleHalfHeight(SlideCaspsuleHalfHeight);
	float HalfHeightAdjust = (CurrentHalfHeight - SlideCaspsuleHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	
	UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -ScaledHalfHeightAdjust), UpdatedComponent->GetComponentQuat(), true);	
	
	SlidingGroundCheckingDistance = SlideCaspsuleHalfHeight + 5.0f;
	SlidingStartLocation = GetActorLocation();
	GetBaseCharacterOwner()->OnStartSliding(HalfHeightAdjust, ScaledHalfHeightAdjust);
	GetWorld()->GetTimerManager().SetTimer(SlidingTimer, this, &UGCBaseCharacterMovementComponent::StopSlide, SlideMaxTime, false);
	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Sliding);
}

void UGCBaseCharacterMovementComponent::StopSlide()
{	
	bIsSliding = false;
	bWantsToCrouch = false;
	GetWorld()->GetTimerManager().ClearTimer(SlidingTimer);
	ACharacter* DefaultCharacter = GetOwner()->GetClass()->GetDefaultObject<ACharacter>();
	const float NewHalfHeight = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float CurrentHalfHeight = GetBaseCharacterOwner()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float ComponentScale = GetBaseCharacterOwner()->GetCapsuleComponent()->GetShapeScale();
	float HalfHeightAdjust;
	
	if (!CanIncreaseCapsuleHalfHeight(NewHalfHeight, CurrentHalfHeight, HalfHeightAdjust))
	{
		CrouchAfterSlideHalfHeightAdjust = HalfHeightAdjust;
		if (CanIncreaseCapsuleHalfHeight(CrouchedHalfHeight, CurrentHalfHeight, HalfHeightAdjust))
		{
			GetBaseCharacterOwner()->OnEndSliding(HalfHeightAdjust, HalfHeightAdjust * ComponentScale);
			bWantToCrouchAfterSlide = true;
			bWantsToCrouch = true;
			SetMovementMode(MOVE_Walking);
			return;
		}		
	}

	GetBaseCharacterOwner()->GetCapsuleComponent()->SetCapsuleHalfHeight(NewHalfHeight);
	GetBaseCharacterOwner()->OnEndSliding(HalfHeightAdjust, HalfHeightAdjust * ComponentScale);
	
	SetMovementMode(MOVE_Walking);
}

const float UGCBaseCharacterMovementComponent::GetCrouchAfterSlideHalfHeightAdjust() const
{
	return CrouchAfterSlideHalfHeightAdjust;
}

void UGCBaseCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	if (MovementMode == MOVE_Custom && CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Sliding)
	{			
		FHitResult HitResult;
		FVector StartPosition = GetActorLocation();
		FVector EndPosition = StartPosition - SlidingGroundCheckingDistance * FVector::UpVector;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		if (!GetWorld()->LineTraceSingleByChannel(HitResult, StartPosition, EndPosition, ECC_Visibility, QueryParams))
		{
			float Gravity = GetGravityZ();
			SlidingVerticalVelocityComponent += FVector::UpVector * DeltaTime * Gravity;			
		}
		Velocity = SlidingHorizontalVelocityComponent + SlidingVerticalVelocityComponent;
	}	
}

void UGCBaseCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	bUseControllerDesiredRotation = true;
	if (MovementMode == MOVE_Swimming)
	{		
		ApplyContextMovementSettings(SwimmingMovementSettings);
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(SwimmingCapsuleRadius, SwimmingCapsuleHalfHeight);
	}
	else if (PreviousMovementMode == MOVE_Swimming)
	{
		RemoveContextMovementSettings(SwimmingMovementSettings);
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->GetCapsuleComponent()->SetCapsuleSize(DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), true);
	}

	if (MovementMode == MOVE_Custom)
	{		
		GetBaseCharacterOwner()->bUseControllerRotationYaw = false;
	}

	if (PreviousMovementMode == MOVE_Custom)
	{
		const ACharacter* Character = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		GetBaseCharacterOwner()->bUseControllerRotationYaw = Character->bUseControllerRotationYaw;
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::CMOVE_Ladder)
	{
		CurrentLadder = nullptr;
	}	

	if (MovementMode == MOVE_Walking || (MovementMode == MOVE_Custom && CustomMovementMode != (uint8)ECustomMovementMode::CMOVE_WallRun))
	{
		CurrentWallRunSide = EWallRunSide::None;
	}
	
	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == (uint8)ECustomMovementMode::CMOVE_Sliding)
	{
		SlidingHorizontalVelocityComponent = FVector::ZeroVector;
		SlidingVerticalVelocityComponent = FVector::ZeroVector;
	}

	if (MovementMode == MOVE_Custom)
	{
		Velocity = FVector::ZeroVector;
		switch (CustomMovementMode)
		{
		case (uint8)ECustomMovementMode::CMOVE_Mantling:
		{
			GetWorld()->GetTimerManager().SetTimer(MantlingTimer, this, &UGCBaseCharacterMovementComponent::EndMantle, CurrentMantlingParameters.Duration, false);
			break;
		}
		case (uint8)ECustomMovementMode::CMOVE_Zipline:
		{
			bUseControllerDesiredRotation = false;
			GetWorld()->GetTimerManager().SetTimer(ZiplineTimer, this, &UGCBaseCharacterMovementComponent::DetachFromZipline, ZiplineMovingTime, false);
			break;
		}
		case (uint8)ECustomMovementMode::CMOVE_WallRun:
		{
			GetWorld()->GetTimerManager().SetTimer(WallRunTimer, this, &UGCBaseCharacterMovementComponent::EndWallRun, MaxWallRunTime, false);
			break;			
		}
		case (uint8)ECustomMovementMode::CMOVE_Sliding:
		{
			bUseControllerDesiredRotation = false;
			break;
		}
		default:
			break;
		}
	}
}

void UGCBaseCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (GetBaseCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	switch (CustomMovementMode)
	{
	case (uint8)ECustomMovementMode::CMOVE_Mantling:
	{
		PhysMantling(deltaTime, Iterations);
		break;
	}
	case (uint8)ECustomMovementMode::CMOVE_Ladder:
	{
		PhysLadder(deltaTime, Iterations);
		break;
	}
	case (uint8)ECustomMovementMode::CMOVE_Zipline:
	{
		PhysZipline(deltaTime, Iterations);
		break;
	}
	case (uint8)ECustomMovementMode::CMOVE_WallRun:
	{
		PhysWallRun(deltaTime, Iterations);
		break;
	}
	case (uint8)ECustomMovementMode::CMOVE_Sliding:
	{
		PhysSliding(deltaTime, Iterations);
		break;
	}
	default:
		break;
	}	
	Super::PhysCustom(deltaTime, Iterations);
}

void UGCBaseCharacterMovementComponent::PhysMantling(float deltaTime, int32 Iterations)
{
	float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(MantlingTimer) + CurrentMantlingParameters.Starttime;

	FVector MantlingCurveValue = CurrentMantlingParameters.MantlingCurve->GetVectorValue(ElapsedTime);

	float PositionAlpha = MantlingCurveValue.X;
	float XYCorrectionAlpha = MantlingCurveValue.Y;
	float ZCorrectionAlpha = MantlingCurveValue.Z;

	FVector CorrectedInitialLocation = FMath::Lerp(CurrentMantlingParameters.InitialLocation, CurrentMantlingParameters.InitialAnimationLocation, XYCorrectionAlpha);
	CorrectedInitialLocation.Z = FMath::Lerp(CurrentMantlingParameters.InitialLocation.Z, CurrentMantlingParameters.InitialAnimationLocation.Z, ZCorrectionAlpha);

	FVector NewLocation = FMath::Lerp(CorrectedInitialLocation, CurrentMantlingParameters.TargetLocation, PositionAlpha);
	FRotator NewRotation = FMath::Lerp(CurrentMantlingParameters.InitialRotation, CurrentMantlingParameters.TargetRotation, PositionAlpha);	
	
	FVector MovingLedgeOffset = CurrentMantlingParameters.TargetToLedgeDistance - (CurrentMantlingParameters.TargetLocation - CurrentMantlingParameters.MantlingPrimitiveComponent->GetOwner()->GetActorLocation());
	NewLocation += MovingLedgeOffset;
	
	FVector Delta = NewLocation - GetActorLocation();
	Velocity = Delta / deltaTime;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, NewRotation, false, Hit);
}

void UGCBaseCharacterMovementComponent::PhysLadder(float deltaTime, int32 Iterations)
{
	CalcVelocity(deltaTime, 1.0f, false, ClimbingLadderBrakingDeceleration);
	FVector Delta = Velocity * deltaTime;
	
	if (HasAnimRootMotion())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), false, Hit);
		return;
	}

	FVector NewPos = GetActorLocation() + Delta;
	float NewPosProjection = GetActorToCurrentLadderProjection(NewPos);

	if(NewPosProjection < MinLadderBottomOffset)
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheBottom);
		return;
	}
	else if (NewPosProjection > (CurrentLadder->GetLadderHeight() - MaxLadderTopOffset))
	{
		DetachFromLadder(EDetachFromLadderMethod::ReachingTheTop);
		return;
	}

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), true, Hit);
}

void UGCBaseCharacterMovementComponent::PhysZipline(float DeltaTime, int32 Iterations)
{		
	FVector EndLocation = ZiplineDirection + ZiplineMovingStartLocation;
	
	float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(ZiplineTimer);
	float PositionAlpha = ElapsedTime / ZiplineMovingTime;	
	
	FVector NewLocation = FMath::Lerp(ZiplineMovingStartLocation, EndLocation, PositionAlpha);

	FVector Delta = NewLocation - GetActorLocation();
	Velocity = Delta / DeltaTime;
	FHitResult Hit;
	
	SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), true, Hit);
	if (Hit.Actor != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(ZiplineTimer);
		DetachFromZipline();		
		return;
	}
}

void UGCBaseCharacterMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{
	if (!AreWallRunRequiredKeysDown(CurrentWallRunSide))
	{
		EndWallRun();
		GetWorld()->GetTimerManager().ClearTimer(WallRunTimer);
		return;
	}	

	FHitResult HitResult;
	FVector LineTraceDirection = CurrentWallRunSide == EWallRunSide::Right ? GetOwner()->GetActorRightVector() : -GetOwner()->GetActorRightVector();
	

	FVector StartPosition = GetActorLocation();
	FVector EndPosition = StartPosition + WallRunLineTraceLength * LineTraceDirection;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartPosition, EndPosition, ECC_WallRunnable, QueryParams))
	{
		EWallRunSide Side = EWallRunSide::None;
		FVector	Direction = FVector::ZeroVector;
		GetWallRunSideAndDirection(HitResult.ImpactNormal, Side, Direction);

		if (Side != CurrentWallRunSide)
		{
			EndWallRun();
			GetWorld()->GetTimerManager().ClearTimer(WallRunTimer);
			return;
		}
		else
		{			 
			CurrentWallRunDirection = Direction;			
			FRotator NewRotation = HitResult.ImpactNormal.ToOrientationRotator() - GetOwner()->GetActorRightVector().ToOrientationRotator();
			if (CurrentWallRunSide == EWallRunSide::Right)
			{
				NewRotation.Yaw -= 180.0f;
			}

			FHitResult Hit;
			FVector Delta = GetMaxSpeed() * CurrentWallRunDirection * deltaTime;
			Velocity = Delta / deltaTime;
			SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation() + NewRotation, true, Hit);
		}
	}
	else
	{
		EndWallRun();
		GetWorld()->GetTimerManager().ClearTimer(WallRunTimer);
		return;
	}
}

void UGCBaseCharacterMovementComponent::PhysSliding(float DeltaTime, int32 Iterations)
{	
	CalcVelocity(DeltaTime, 1.0f, false, 0.0f);
	
	FVector Delta = Velocity * DeltaTime;	
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, GetOwner()->GetActorRotation(), true, Hit);	
}

AGCBaseCharacter* UGCBaseCharacterMovementComponent::GetBaseCharacterOwner() const
{
	return StaticCast<AGCBaseCharacter*>(CharacterOwner);
}

bool UGCBaseCharacterMovementComponent::AreWallRunRequiredKeysDown(EWallRunSide Side) const
{
	float ForwardAxis = 0.0f;
	float RightAxis = 0.0f;
	GetBaseCharacterOwner()->GetPlayerMovingInput(ForwardAxis, RightAxis);
	if (ForwardAxis < 0.1f)
	{
		return false;
	}

	if (Side == EWallRunSide::Right && RightAxis < -0.1f)
	{
		return false;
	}

	if (Side == EWallRunSide::Left && RightAxis > 0.1f)
	{
		return false;
	}

	return true;
}

bool UGCBaseCharacterMovementComponent::CanIncreaseCapsuleHalfHeight(float NewHalfHeight, float CurrentHalfHeight, float& OutHalfHeightAdjust) const
{	
	const float HalfHeightAdjust = NewHalfHeight - CurrentHalfHeight;	
	const FVector PawnLocation = UpdatedComponent->GetComponentLocation();

	const UWorld* MyWorld = GetWorld();
	const float SweepInflation = KINDA_SMALL_NUMBER * 10.f;
	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(IncreaseTrace), false, GetOwner());
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);

	const FCollisionShape NewCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - HalfHeightAdjust);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	FVector NewLocation = PawnLocation + FVector(0.f, 0.f, NewCapsuleShape.GetCapsuleHalfHeight() - CurrentHalfHeight);
	OutHalfHeightAdjust = HalfHeightAdjust;
	
	return !MyWorld->OverlapBlockingTestByChannel(NewLocation, FQuat::Identity, CollisionChannel, NewCapsuleShape, CapsuleParams, ResponseParam);
}

void UGCBaseCharacterMovementComponent::ApplyContextMovementSettings(const FContextMovementSettings& MovementSettings)
{
	bOrientRotationToMovement = MovementSettings.bOrientRotationToMovement;
	CurrentContextMovementSettings.AddUnique(&MovementSettings);
}

void UGCBaseCharacterMovementComponent::RemoveContextMovementSettings(const FContextMovementSettings& MovementSettings)
{
	CurrentContextMovementSettings.Remove(&MovementSettings);	
	if (CurrentContextMovementSettings.Num() == 0)
	{		
		const ACharacter* Character = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		bOrientRotationToMovement = Character->GetCharacterMovement()->bOrientRotationToMovement;
	}
	else
	{
		const FContextMovementSettings* Settings = CurrentContextMovementSettings.Last();
		ApplyContextMovementSettings(*Settings);
	}
}

void FSavedMove_GC::Clear()
{
	Super::Clear();
	bSavedIsSprinting = 0;
	bSavedIsMantling = 0;
	bSavedIsAiming = 0;
}

uint8 FSavedMove_GC::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	
	/*
		FLAG_Reserved_1 = 0x04,	// Reserved for future use
		FLAG_Reserved_2 = 0x08,	// Reserved for future use
		// Remaining bit masks are available for custom flags.
		FLAG_Custom_0 = 0x10, - Sprint flag
		FLAG_Custom_1 = 0x20, - Mantling
		FLAG_Custom_2 = 0x40, - aiming
		FLAG_Custom_3 = 0x80,
	 */
	
	if (bSavedIsSprinting)
	{
		Result |= FLAG_Custom_0;
	}
	if (bSavedIsMantling)
	{
		Result &= ~FLAG_JumpPressed;
		Result |= FLAG_Custom_1;
	}
	if (bSavedIsAiming)
	{
		Result |= FLAG_Custom_2;
	}
	return Result;
}

bool FSavedMove_GC::CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_GC* NewMove = StaticCast<const FSavedMove_GC*>(NewMovePtr.Get());

	if (bSavedIsSprinting != NewMove->bSavedIsSprinting
		|| bSavedIsMantling != NewMove->bSavedIsMantling
		|| bSavedIsAiming != NewMove->bSavedIsAiming)
	{
		return false;
	}

	return Super::CanCombineWith(NewMovePtr, InCharacter, MaxDelta);
}

void FSavedMove_GC::SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(InCharacter, InDeltaTime, NewAccel, ClientData);

	check(InCharacter->IsA<AGCBaseCharacter>());
	AGCBaseCharacter* InBaseCharacter = StaticCast<AGCBaseCharacter*>(InCharacter);
	UGCBaseCharacterMovementComponent* MovementComponent = InBaseCharacter->GetBaseCharacterMovementComponent();

	bSavedIsSprinting = MovementComponent->bIsSprintng;
	bSavedIsMantling = InBaseCharacter->bIsMantling;
	bSavedIsAiming = MovementComponent->bIsAiming;
}

void FSavedMove_GC::PrepMoveFor(ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UGCBaseCharacterMovementComponent* MovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(Character->GetMovementComponent());

	MovementComponent->bIsSprintng = bSavedIsSprinting;
	MovementComponent->bIsAiming = bSavedIsAiming;
}

FNetworkPredictionData_Client_Character_GC::FNetworkPredictionData_Client_Character_GC(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_Character_GC::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_GC());
}
