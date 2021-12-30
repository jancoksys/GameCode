// Fill out your copyright notice in the Description page of Project Settings.


#include "GCBaseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/MovementComponents/GCBaseCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/LedgeDetectorComponent.h"
#include "Curves/CurveVector.h"
#include "Actors/Interactive/Ladder.h"
#include "Actors/Interactive/Zipline.h"
#include "GameCodeTypes.h"
#include "Components/CharacterComponents/CharacterAttributeComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Actors/Equipment/Weapons/MeleeWeaponItem.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"
#include "Actors/Interactive/Interface/Interactive.h"
#include "Components/WidgetComponent.h"
#include "UI/World/GCAttributeProgressBar.h"
#include "Components/CharacterComponents/CharacterInventoryComponent.h"
#include "Inventory/Items/Equipables/InventoryAmmoItem.h"

AGCBaseCharacter::AGCBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGCBaseCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{	
	GCBaseCharacterMovementComponent = StaticCast<UGCBaseCharacterMovementComponent*>(GetCharacterMovement()); 		
	
	IKScale = GetActorScale3D().Z;		
	IKTraceDistance = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * IKScale / 2;

	LedgeDetectorComponent = CreateDefaultSubobject<ULedgeDetectorComponent>(TEXT("LedgeDetector"));

	GetMesh()->CastShadow = true;
	GetMesh()->bCastDynamicShadow = true;

	CharacterAttributesComponent = CreateDefaultSubobject<UCharacterAttributeComponent>(TEXT("CharacterAttributes"));
	CharacterEquipmentComponent = CreateDefaultSubobject<UCharacterEquipmentComponent>(TEXT("CharacterEquipment"));
	CharacterInventoryComponent = CreateDefaultSubobject<UCharacterInventoryComponent>(TEXT("InventoryComponent"));

	HealthBarProgressComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarProgressComponent"));
	HealthBarProgressComponent->SetupAttachment(GetCapsuleComponent());

	ArrowMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	ArrowMeshComponent->SetupAttachment(GetMesh(), SocketArrowHand);
	ArrowMeshComponent->SetVisibility(false);
}

void AGCBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGCBaseCharacter, bIsMantling);
}

void AGCBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AAIController* AIController = Cast<AAIController>(NewController);
	if (IsValid(AIController))
	{
		FGenericTeamId TeamId((uint8)Team);
		AIController->SetGenericTeamId(TeamId);
	}
}

void AGCBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	USpringArmComponent* SpringArmComponent = FindComponentByClass<USpringArmComponent>();
	if (IsValid(SpringArmComponent))
	{
		DefaultSpringArmLength = SpringArmComponent->TargetArmLength;
	}

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AGCBaseCharacter::OnPlayerCapsuleHit);
	CharacterAttributesComponent->OnDeathEvent.AddUObject(this, &AGCBaseCharacter::OnDeath);
	CharacterAttributesComponent->OutOfStaminaEvent.AddUObject(this, &AGCBaseCharacter::ChangeStaminaCondition);
	
	InitializeHealthProgress();

	if (bIsSignificanceEnabled)
	{
		USignificanceManager* SignificanceManager = FSignificanceManagerModule::Get(GetWorld());
		if (IsValid(SignificanceManager))
		{
			SignificanceManager->RegisterObject(
				this,
				SignificanceTagCharacter,
				[this](USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint) -> float
				{
					return SignificanceFunction(ObjectInfo, ViewPoint);
				},
				USignificanceManager::EPostSignificanceType::Sequential,
				[this](USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
				{
					PostSignificanceFunction(ObjectInfo, OldSignificance, Significance, bFinal);
				}
			);
		}
	}
}

void AGCBaseCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (OnInteractableObjectFound.IsBound())
	{
		OnInteractableObjectFound.Unbind();
	}
	Super::EndPlay(Reason);
}

void AGCBaseCharacter::MoveForward(float Value)
{
	ForwardMovingInput = Value;
}

void AGCBaseCharacter::MoveRight(float Value)
{
	RightMovingInput = Value;
}

void AGCBaseCharacter::Jump()
{
	if (GCBaseCharacterMovementComponent->IsWallRunning())
	{
		FVector JumpDirection = FVector::ZeroVector;

		if (GCBaseCharacterMovementComponent->GetCurrentWallRunSide() == EWallRunSide::Right)
		{
			JumpDirection = FVector::CrossProduct(CurrentWallRunDirection, FVector::UpVector).GetSafeNormal();
		}
		else
		{
			JumpDirection = FVector::CrossProduct(FVector::UpVector, CurrentWallRunDirection).GetSafeNormal();
		}

		JumpDirection += FVector::UpVector;
		
		LaunchCharacter(GetCharacterMovement()->JumpZVelocity * JumpDirection.GetSafeNormal(), false, true);
		GCBaseCharacterMovementComponent->EndWallRun();
	}
	else
	{
		Super::Jump();
	}
}

void AGCBaseCharacter::ChangeCrouchState()
{
	if (GetCharacterMovement()->IsCrouching() && !GCBaseCharacterMovementComponent->IsProne() && !GCBaseCharacterMovementComponent->GetWantsToProne())
	{		
		UnCrouch();		
	}
	
	if (!GetCharacterMovement()->IsCrouching())
	{
		Crouch();
	}
}

void AGCBaseCharacter::ChangeProneState()
{
	if (GCBaseCharacterMovementComponent->IsProne())
	{		
		UnProne();		
	}
	
	if (!GCBaseCharacterMovementComponent->IsProne() && CanProne())
	{
		Prone();
	}
}

void AGCBaseCharacter::Prone()
{	
	GCBaseCharacterMovementComponent->SetWantsToProne(true);
}

void AGCBaseCharacter::UnProne()
{
	GCBaseCharacterMovementComponent->SetWantsToProne(false);
}

void AGCBaseCharacter::StartSprint()
{
	if (GCBaseCharacterMovementComponent->Velocity.Size() == 0)
	{
		return;
	}

	bIsSprintRequested = true;
	if (bIsCrouched)
	{
		UnCrouch();
	}
}

void AGCBaseCharacter::StopSprint()
{
	bIsSprintRequested = false;
}

void AGCBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);	
	UpdateFootIK(DeltaTime);
	TryChangeSprintState(DeltaTime);
	TraceLineOfSight();
}
const FMantlingSettings& AGCBaseCharacter::GetMantlingSettings(float LedgeHeight) const
{
	return LedgeHeight > LowMantleMaxHeight ? HighMantleSettings : LowMantleSettings;
}

void AGCBaseCharacter::OnPlayerCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!GetCharacterMovement()->IsFalling() || !IsSurfaceWallRunnable(OtherActor) || GetBaseCharacterMovementComponent()->IsOnZipline())
	{
		return;
	}	
	EWallRunSide NewWallRunSide = EWallRunSide::None;
	GCBaseCharacterMovementComponent->GetWallRunSideAndDirection(Hit.Normal, NewWallRunSide, CurrentWallRunDirection);
	
	if (NewWallRunSide == GCBaseCharacterMovementComponent->GetCurrentWallRunSide() && OtherActor == CurrentWallRunActor)
	{
		return;
	}
	
	CurrentWallRunActor = OtherActor;
	GCBaseCharacterMovementComponent->StartWallRun(NewWallRunSide, CurrentWallRunDirection);
}

bool AGCBaseCharacter::IsSurfaceWallRunnable(AActor* SurfaceActor) const
{
	if (!IsValid(SurfaceActor))
	{
		return false;
	}
	ECollisionResponse Responce = SurfaceActor->GetComponentsCollisionResponseToChannel(ECC_WallRunnable);
	if (Responce == ECR_Block)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void AGCBaseCharacter::SetWalkingMovementMode()
{
	GCBaseCharacterMovementComponent->SetMovementMode(MOVE_Walking);
}

void AGCBaseCharacter::EnableRagdoll()
{
	GetMesh()->SetCollisionProfileName(CollisionProfileRagdoll);
	GetMesh()->SetSimulatePhysics(true);
}

void AGCBaseCharacter::UpdateFootIK(float DeltaTime)
{
	IKRightFootToeOffset = FMath::FInterpTo(IKRightFootToeOffset, GetIKOffsetForASocket(RightFootToeSocketName), DeltaTime, IKInterpSpeed);
	IKLeftFootToeOffset = FMath::FInterpTo(IKLeftFootToeOffset, GetIKOffsetForASocket(LeftFootToeSocketName), DeltaTime, IKInterpSpeed);
	IKRightFootHeelOffset = FMath::FInterpTo(IKRightFootHeelOffset, GetIKOffsetForASocket(RightFootHeelSocketName), DeltaTime, IKInterpSpeed);
	IKLeftFootHeelOffset = FMath::FInterpTo(IKLeftFootHeelOffset, GetIKOffsetForASocket(LeftFootHeelSocketName), DeltaTime, IKInterpSpeed);

	IKLeftFootOffset = FMath::Max(IKLeftFootHeelOffset, IKLeftFootToeOffset);
	IKRightFootOffset = FMath::Max(IKRightFootHeelOffset, IKRightFootToeOffset);

	if (FMath::Abs(IKLeftFootOffset) > FMath::Abs(IKRightFootOffset))
	{
		IKBodyOffset = FMath::FInterpTo(IKBodyOffset, IKLeftFootOffset, DeltaTime, IKInterpSpeed);
	}
	else if (FMath::Abs(IKLeftFootOffset) < FMath::Abs(IKRightFootOffset))
	{
		IKBodyOffset = FMath::FInterpTo(IKBodyOffset, IKRightFootOffset, DeltaTime, IKInterpSpeed);
	}
	else
	{
		IKBodyOffset = FMath::FInterpTo(IKBodyOffset, 0.0f, DeltaTime, IKInterpSpeed);
	}
}

void AGCBaseCharacter::ChangeStaminaCondition(bool IsOutOfStamina_In)
{
	GCBaseCharacterMovementComponent->SetIsOutOfStamina(IsOutOfStamina_In);
}

void AGCBaseCharacter::Mantle(bool bForce)
{
	if (!(CanMantle() || bForce ))
	{
		return;
	}	
	FLedgeDescription LedgeDescription;
	if (LedgeDetectorComponent->DetectLedge(LedgeDescription))
	{
		bIsMantling = true;
		FMantlingMovementParameters MantlingParameters;
		
		UCapsuleComponent* BaseCapsuleComponent = GetDefault<AGCBaseCharacter>()->GetCapsuleComponent();
		if (bIsCrouched)
		{
			UnCrouch();
			MantlingParameters.InitialLocation = GetActorLocation() - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + BaseCapsuleComponent->GetScaledCapsuleHalfHeight();
		}
		else
		{
			MantlingParameters.InitialLocation = GetActorLocation();
		}

		MantlingParameters.InitialRotation = GetActorRotation();
		MantlingParameters.TargetLocation = LedgeDescription.Location;
		MantlingParameters.TargetRotation = LedgeDescription.Rotation;
		MantlingParameters.MantlingPrimitiveComponent = LedgeDescription.LedgePrimitiveComponent;
		
		const FVector TargetToLedgeDistance = MantlingParameters.TargetLocation - MantlingParameters.MantlingPrimitiveComponent->GetOwner()->GetActorLocation();
		MantlingParameters.TargetToLedgeDistance = TargetToLedgeDistance;

		float MantlingHeight = LedgeDescription.LedgeHeight;
		const FMantlingSettings& MantlingSettings = GetMantlingSettings(MantlingHeight);

		float MinRange;
		float MaxRange;
		MantlingSettings.MantlingCurve->GetTimeRange(MinRange, MaxRange);

		MantlingParameters.Duration = MaxRange - MinRange; 		

		MantlingParameters.MantlingCurve = MantlingSettings.MantlingCurve;

		FVector2D SourceRange(MantlingSettings.MinHeight, MantlingSettings.MaxHeight);
		FVector2D TargetRange(MantlingSettings.MinHeightStartTime, MantlingSettings.MaxHeightStartTime);
		MantlingParameters.Starttime = FMath::GetMappedRangeValueClamped(SourceRange, TargetRange, MantlingHeight);

		MantlingParameters.InitialAnimationLocation = MantlingParameters.TargetLocation - MantlingSettings.AnimationCorrectionZ * FVector::UpVector + MantlingSettings.AnimationCorrectionXY * LedgeDescription.LedgeNormal;	
		
		if (IsLocallyControlled() || GetLocalRole() == ROLE_Authority)
		{
			GetBaseCharacterMovementComponent()->StartMantle(MantlingParameters);
		}		
		
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(MantlingSettings.MantlingMontage, 1.0f, EMontagePlayReturnType::Duration, MantlingParameters.Starttime);
		OnMantle(MantlingSettings, MantlingParameters.Starttime);
	}
}

void AGCBaseCharacter::StartSlide()
{	
	if (!GetBaseCharacterMovementComponent()->IsSprinting() || !CanSlide())
	{
		return;
	}

	GetBaseCharacterMovementComponent()->StartSlide();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance) && IsValid(SlidingMontage))
	{
		AnimInstance->Montage_Play(SlidingMontage, 1.0f, EMontagePlayReturnType::Duration, 0.0f);		
	}	
}

void AGCBaseCharacter::StopSlide()
{
	if (!GetBaseCharacterMovementComponent()->IsSliding())
	{
		return;
	}

	GetBaseCharacterMovementComponent()->StopSlide();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();	
	if (IsValid(AnimInstance) && IsValid(SlidingMontage))
	{		
		if (GetBaseCharacterMovementComponent()->bWantToCrouchAfterSlide)
		{
			AnimInstance->Montage_Play(SlidingMontage, 1.0f, EMontagePlayReturnType::Duration, 0.0f, true);
			AnimInstance->Montage_JumpToSection(SlideToCrouchSection);
		}
		else
		{
			AnimInstance->StopAllMontages(SlidingToCrouchBlendOutTime);
		}
	}
}

void AGCBaseCharacter::OnLevelDeserialized_Implementation()
{
	
}

void AGCBaseCharacter::StartFire()
{
	if (!CanFire())
	{
		return;
	}
	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StartFire();
	}
}

bool AGCBaseCharacter::CanFire() const
{
	if (CharacterEquipmentComponent->IsEquipping())
	{
		return false;
	}
	if (CharacterEquipmentComponent->IsSelectingWeapon())
	{
		return false;
	}
	return true;
}

void AGCBaseCharacter::ChangeFireMode()
{
	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->ChangeFireMode();
	}
}

void AGCBaseCharacter::StopFire()
{
	ARangeWeaponItem* CurrentRangeWeapon = CharacterEquipmentComponent->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopFire();
	}
}

void AGCBaseCharacter::StartAiming()
{
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (!IsValid(CurrentRangeWeapon))
	{
		return;
	}
	bIsAiming = true;
	GCBaseCharacterMovementComponent->SetIsAiming(true);
	CurrentRangeWeapon->StartAiming();
	OnStartAiming();
}

void AGCBaseCharacter::StopAiming()
{
	if (!bIsAiming)
	{
		return;
	}
	
	ARangeWeaponItem* CurrentRangeWeapon = GetCharacterEquipmentComponent()->GetCurrentRangeWeapon();
	if (IsValid(CurrentRangeWeapon))
	{
		CurrentRangeWeapon->StopAiming();
	}	

	bIsAiming = false;
	GCBaseCharacterMovementComponent->SetIsAiming(false);
	OnStopAimingInternal();
}

FRotator AGCBaseCharacter::GetAimOffset()
{
	FVector AimDirectionWorld = GetBaseAimRotation().Vector();
	FVector AimDirectionLocal = GetTransform().InverseTransformVectorNoScale(AimDirectionWorld);
	FRotator Result = AimDirectionLocal.ToOrientationRotator();

	return Result;
}

void AGCBaseCharacter::Reload()
{
	if (IsValid(CharacterEquipmentComponent->GetCurrentRangeWeapon()))
	{
		CharacterEquipmentComponent->ReloadCurrentWeapon();
	}
}

void AGCBaseCharacter::OnStartAiming_Implementation()
{
	OnStartAimingInternal();
}

void AGCBaseCharacter::OnStopAiming_Implementation()
{
	OnStopAimingInternal();
}

float AGCBaseCharacter::GetAimingMovementSpeed() const
{
	return CurrentAimingMovementSpeed;
}

void AGCBaseCharacter::OnStartProne(float HeightAdjust, float ScaledHeightAdjust)
{
	const ACharacter* DefaultChar = GetDefault<ACharacter>(GetClass());
	if (GetMesh() && DefaultChar->GetMesh())
	{
		FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = DefaultChar->GetMesh()->GetRelativeLocation().Z + HeightAdjust;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}

	USpringArmComponent* SpringArmComponent = FindComponentByClass<USpringArmComponent>();
	if (IsValid(SpringArmComponent))
	{
		SpringArmComponent->TargetOffset.Z = 0.0f;
	}
}
	

void AGCBaseCharacter::OnEndProne(float HeightAdjust, float ScaledHeightAdjust)
{
	OnStartCrouch(HeightAdjust, ScaledHeightAdjust);
	
	USpringArmComponent* SpringArmComponent = FindComponentByClass<USpringArmComponent>();
	SpringArmComponent->TargetOffset.Z = HeightAdjust;	
}

void AGCBaseCharacter::OnStandAfterProne(float HeightAdjust, float ScaledHeightAdjust)
{
	OnEndCrouch(HeightAdjust, ScaledHeightAdjust);

	const ACharacter* DefaultCharacter = this->GetClass()->GetDefaultObject<ACharacter>();	
	USpringArmComponent* SpringArmComponent = FindComponentByClass<USpringArmComponent>();	
	USpringArmComponent* DefaultCharacterSpringArm = DefaultCharacter->FindComponentByClass<USpringArmComponent>();
	SpringArmComponent->TargetOffset.Z = DefaultCharacterSpringArm->TargetOffset.Z;
}

void AGCBaseCharacter::RegisterInteractiveActor(AInteractiveActor* InteractiveActor)
{
	AvailableInteractiveActors.AddUnique(InteractiveActor);
}

void AGCBaseCharacter::UnregisterInteractiveActor(AInteractiveActor* InteractiveActor)
{
	AvailableInteractiveActors.RemoveSingleSwap(InteractiveActor);
}

void AGCBaseCharacter::ClimbLadder(float Value)
{
	if (GetBaseCharacterMovementComponent()->IsOnLadder() && !FMath::IsNearlyZero(Value))
	{
		FVector LadderUpVector = GetBaseCharacterMovementComponent()->GetCurrentLadder()->GetActorUpVector();
		AddMovementInput(LadderUpVector, Value);
	}
}

void AGCBaseCharacter::InteractWithLadder()
{
	if (!CanInteractWithLadder())
	{
		return;
	}

	if(GetBaseCharacterMovementComponent()->IsOnLadder())
	{
		GetBaseCharacterMovementComponent()->DetachFromLadder(EDetachFromLadderMethod::JumpOff);
	}
	else
	{
		const ALadder* AvailableLadder = GetAvailableLadder();
		if (IsValid(AvailableLadder))
		{
			if (AvailableLadder->GetIsOnTop())
			{
				PlayAnimMontage(AvailableLadder->GetAttachFromTopAnimMontage());
			}
			GetBaseCharacterMovementComponent()->AttachToLadder(AvailableLadder);
		}
	}
}

const class ALadder* AGCBaseCharacter::GetAvailableLadder() const
{
	const ALadder* Result = nullptr;
	for(const AInteractiveActor* InteractiveActor : AvailableInteractiveActors)
	{
		if(InteractiveActor->IsA<ALadder>())
		{
			Result = StaticCast<const ALadder*>(InteractiveActor);
			break;
		}
	}
	return Result;
}

void AGCBaseCharacter::InteractWithZipline()
{	
	if (GetBaseCharacterMovementComponent()->IsOnZipline())
	{
		GetBaseCharacterMovementComponent()->DetachFromZipline();
	}
	else
	{
		const AZipline* AvailableZipline = GetAvailableZipline();
		if (IsValid(AvailableZipline))
		{
			GetBaseCharacterMovementComponent()->AttachToZipline(AvailableZipline);
		}
	}
}

const class AZipline* AGCBaseCharacter::GetAvailableZipline() const
{
	const AZipline* Result = nullptr;
	for (const AInteractiveActor* InteractiveActor : AvailableInteractiveActors)
	{
		if (InteractiveActor->IsA<AZipline>())
		{
			Result = StaticCast<const AZipline*>(InteractiveActor);
			break;
		}
	}
	return Result;
}

void AGCBaseCharacter::GetPlayerMovingInput(float& ForwardInput, float& RightInput) const
{
	ForwardInput = ForwardMovingInput;
	RightInput = RightMovingInput;
}

void AGCBaseCharacter::OnStartSliding(float HeightAdjust, float ScaledHeightAdjust)
{		
	if (GetMesh())
	{
		FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z += HeightAdjust;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}

	OnSprintEnd();
}

void AGCBaseCharacter::OnEndSliding(float HeightAdjust, float ScaledHeightAdjust)
{
	const ACharacter* DefaultCharacter = this->GetClass()->GetDefaultObject<ACharacter>();
	if (GetMesh() && DefaultCharacter->GetMesh())
	{
		FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
		MeshRelativeLocation.Z = DefaultCharacter->GetMesh()->GetRelativeLocation().Z;
		BaseTranslationOffset.Z = MeshRelativeLocation.Z;
	}
	else
	{
		BaseTranslationOffset.Z = DefaultCharacter->GetBaseTranslationOffset().Z;
	}
	
	USpringArmComponent* SpringArmComponent = FindComponentByClass<USpringArmComponent>();
	USpringArmComponent* DefaultCharacterSpringArm = DefaultCharacter->FindComponentByClass<USpringArmComponent>();
	SpringArmComponent->TargetOffset.Z = DefaultCharacterSpringArm->TargetOffset.Z;	
}

void AGCBaseCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (GCBaseCharacterMovementComponent->bWantToCrouchAfterSlide)
	{
		const ACharacter* DefaultCharacter = this->GetClass()->GetDefaultObject<ACharacter>();
		if (GetMesh() && DefaultCharacter->GetMesh())
		{
			FVector& MeshRelativeLocation = GetMesh()->GetRelativeLocation_DirectMutable();
			MeshRelativeLocation.Z = DefaultCharacter->GetMesh()->GetRelativeLocation().Z + GCBaseCharacterMovementComponent->GetCrouchAfterSlideHalfHeightAdjust();
			BaseTranslationOffset.Z = MeshRelativeLocation.Z;
		}
		else
		{
			BaseTranslationOffset.Z = DefaultCharacter->GetBaseTranslationOffset().Z + GCBaseCharacterMovementComponent->GetCrouchAfterSlideHalfHeightAdjust();
		}
		GCBaseCharacterMovementComponent->bWantToCrouchAfterSlide = false;		
	}
}

void AGCBaseCharacter::Falling()
{
	Super::Falling();
	GetBaseCharacterMovementComponent()->bNotifyApex = true;
}

void AGCBaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();	
	const float FallHeight = (CurrentFallApex - GetActorLocation()).Z;
	if (IsValid(FallDamageCurve))
	{
		float DamageAmount = FallDamageCurve->GetFloatValue(FallHeight * 0.01f);
		TakeDamage(DamageAmount, FDamageEvent(), GetController(), Hit.Actor.Get());
	}
	if (FallHeight >= HardLandingMinHeight && CharacterAttributesComponent->IsAlive() && IsValid(AnimInstance) && IsValid(HardLandingMontage))
	{					
		OnHardLanded();
		GCBaseCharacterMovementComponent->DisableMovement();		
		float HardLandingAnimationDuration = AnimInstance->Montage_Play(HardLandingMontage, 1.0f, EMontagePlayReturnType::Duration, 0.0f, true);	
		GetWorld()->GetTimerManager().SetTimer(HardLandingTimer, this, &AGCBaseCharacter::SetWalkingMovementMode, HardLandingAnimationDuration, false);
	}	
	CurrentFallApex = FVector::ZeroVector;
}

void AGCBaseCharacter::NotifyJumpApex()
{
	Super::NotifyJumpApex();
	CurrentFallApex = GetActorLocation();	
}

bool AGCBaseCharacter::IsSwimmingUnderWater() const
{
	if (!GetCharacterMovement()->IsSwimming())
	{
		return false;
	}	
	
	FVector HeadPosition = GetMesh()->GetSocketLocation(SocketHead);

	APhysicsVolume* Volume = GetCharacterMovement()->GetPhysicsVolume();
	float VolumeTopPlane = Volume->GetActorLocation().Z + Volume->GetBounds().BoxExtent.Z * Volume->GetActorScale3D().Z;	
	if (HeadPosition.Z > VolumeTopPlane)
	{
		return false;
	}
	return true;
}

const UCharacterEquipmentComponent* AGCBaseCharacter::GetCharacterEquipmentComponent() const
{
	return CharacterEquipmentComponent;
}

UCharacterEquipmentComponent* AGCBaseCharacter::GetCharacterEquipmentComponent_Mutable() const
{
	return CharacterEquipmentComponent;
}

bool AGCBaseCharacter::IsAiming() const
{
	return bIsAiming;
}

const UCharacterAttributeComponent* AGCBaseCharacter::GetCharacterAttributesComponent() const
{
	return CharacterAttributesComponent;
}

UCharacterAttributeComponent* AGCBaseCharacter::GetCharacterAttributesComponent_Mutable() const
{
	return CharacterAttributesComponent;
}

UCharacterInventoryComponent* AGCBaseCharacter::GetCharacterInventoryComponent_Mutable() const
{
	return CharacterInventoryComponent;
}

void AGCBaseCharacter::NextItem()
{
	CharacterEquipmentComponent->EquipNextItem();
}

void AGCBaseCharacter::PreviousItem()
{
	CharacterEquipmentComponent->EquipPreviousItem();
}

void AGCBaseCharacter::EquipPrimaryItem()
{
	CharacterEquipmentComponent->EquipItemInSlot(EEquipmentSlots::PrimaryItemSlot);
}

void AGCBaseCharacter::PrimaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = CharacterEquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackTypes::PrimaryAttack);
	}
}

void AGCBaseCharacter::SecondaryMeleeAttack()
{
	AMeleeWeaponItem* CurrentMeleeWeapon = CharacterEquipmentComponent->GetCurrentMeleeWeapon();
	if (IsValid(CurrentMeleeWeapon))
	{
		CurrentMeleeWeapon->StartAttack(EMeleeAttackTypes::SecondaryAttack);
	}
}

FGenericTeamId AGCBaseCharacter::GetGenericTeamId() const
{
	return FGenericTeamId((uint8)Team);
}

void AGCBaseCharacter::OnRep_IsMantling(bool bWasMantling)
{
	if (GetLocalRole() == ROLE_SimulatedProxy && !bWasMantling && bIsMantling)
	{		
		Mantle(true);	
	}
}

void AGCBaseCharacter::Interact()
{
	if (LineOfSightObject.GetInterface())
	{
		LineOfSightObject->Interact(this);
	}
}

void AGCBaseCharacter::InitializeHealthProgress()
{
	UGCAttributeProgressBar* Widget = Cast<UGCAttributeProgressBar>(HealthBarProgressComponent->GetUserWidgetObject());
	if (!IsValid(Widget))
	{
		HealthBarProgressComponent->SetVisibility(false);
		return;
	}

	if (IsPlayerControlled() && IsLocallyControlled())
	{
		HealthBarProgressComponent->SetVisibility(false);
	}

	CharacterAttributesComponent->OnHealthChangedEvent.AddUObject(Widget, &UGCAttributeProgressBar::SetProgressPercentage);
	CharacterAttributesComponent->OnDeathEvent.AddLambda([=]() { HealthBarProgressComponent->SetVisibility(false); });
	Widget->SetProgressPercentage(CharacterAttributesComponent->GetHealthPercent());
}

bool AGCBaseCharacter::PickupItem(TWeakObjectPtr<UInventoryItem> ItemToPickup)
{
	UInventoryAmmoItem* AmmoItem = Cast<UInventoryAmmoItem>(ItemToPickup);
	if (IsValid(AmmoItem))
	{
		UInventoryItem* ItemInSlot = CharacterInventoryComponent->FindAmmoItem(AmmoItem);
		if (IsValid(ItemInSlot))
		{
			UInventoryAmmoItem* AmmoItemInSlot = StaticCast<UInventoryAmmoItem*>(ItemInSlot);
			int32 NewItemCount = AmmoItemInSlot->GetAmmoAmount() + AmmoItem->GetAmmoAmount();
			CharacterInventoryComponent->ChangeItemCount(ItemInSlot, NewItemCount);			
		}
		if (GetLocalRole() == ROLE_Authority)
		{
			CharacterEquipmentComponent->AddAmmoToAmunition(AmmoItem->GetAmmoAmount(), AmmoItem->GetAmunitionType());
		}		
		return true;
	}
	if (CharacterInventoryComponent->HasFreeSlot())
	{
		int32 ItemCount = 1;		
		if (IsValid(AmmoItem))
		{
			ItemCount = AmmoItem->GetAmmoAmount();
		}

		CharacterInventoryComponent->AddItem(ItemToPickup, ItemCount);
		return true;
	}
	return false;
}

void AGCBaseCharacter::UseInventory(APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
	{
		return;
	}
	if (!CharacterInventoryComponent->IsViewVisible())
	{
		CharacterInventoryComponent->OpenViewInventory(PlayerController);
		CharacterEquipmentComponent->OpenViewEquipment(PlayerController);
		PlayerController->SetInputMode(FInputModeGameAndUI{});
		PlayerController->bShowMouseCursor = true;
	}
	else
	{
		CharacterInventoryComponent->CloseViewInventory();
		CharacterEquipmentComponent->CloseViewEquipment();
		PlayerController->SetInputMode(FInputModeGameOnly{});
		PlayerController->bShowMouseCursor = false;
	}
}

void AGCBaseCharacter::ConfirmWeaponSelection()
{
	if (CharacterEquipmentComponent->IsSelectingWeapon())
	{
		CharacterEquipmentComponent->ConfirmWeaponSelection();
	}
}

bool AGCBaseCharacter::IsMontagePlaying(UAnimMontage* Montage_In) const
{
	return GetMesh()->GetAnimInstance()->Montage_IsPlaying(Montage_In);
}

void AGCBaseCharacter::SetHandArrowVisibility(bool bIsVisible)
{
	ArrowMeshComponent->SetVisibility(bIsVisible);
}

void AGCBaseCharacter::SetCurrentAimingMovementSpeed(float Speed_In)
{
	CurrentAimingMovementSpeed = Speed_In;
}

bool AGCBaseCharacter::CanJumpInternal_Implementation() const
{
	return Super::CanJumpInternal_Implementation() && !GetBaseCharacterMovementComponent()->IsMantling() && !GetBaseCharacterMovementComponent()->IsSliding();
}

void AGCBaseCharacter::OnSprintStart_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("AGCBaseCharacter::OnSprintStart_Implementation"));
}

void AGCBaseCharacter::OnSprintEnd_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("AGCBaseCharacter::OnSprintEnd_Implementation"));
}

bool AGCBaseCharacter::CanSprint()
{
	if (GCBaseCharacterMovementComponent->IsProne() || GCBaseCharacterMovementComponent->IsOnLadder() || GCBaseCharacterMovementComponent->IsSliding())
	{
		bIsSprintRequested = false;
		return false;
	}
	else
	{
		return true;
	}	
}

bool AGCBaseCharacter::CanProne()
{
	return bIsCrouched && !GetBaseCharacterMovementComponent()->IsSliding();
}

bool AGCBaseCharacter::CanCrouch() const
{
	return Super::CanCrouch() && !GetBaseCharacterMovementComponent()->IsSliding();
}

bool AGCBaseCharacter::CanMantle() const
{
	return !GetBaseCharacterMovementComponent()->IsOnLadder() && !GetBaseCharacterMovementComponent()->IsMantling() && !GetBaseCharacterMovementComponent()->IsSliding() && !GetBaseCharacterMovementComponent()->IsProne();
}

bool AGCBaseCharacter::CanSlide() const
{
	return !GetBaseCharacterMovementComponent()->IsSwimming();
}

bool AGCBaseCharacter::CanInteractWithLadder() const
{
	return !GetBaseCharacterMovementComponent()->IsSliding();
}

void AGCBaseCharacter::OnMantle(const FMantlingSettings& MantlingSettings, float MantlingAnimationStartTime)
{

}

void AGCBaseCharacter::OnHardLanded()
{

}

void AGCBaseCharacter::OnDeath()
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	float Duration = PlayAnimMontage(OnDeathAnimMontage);
	if (Duration == 0.0f)
	{		
		EnableRagdoll();
	}	
}

void AGCBaseCharacter::OnStartAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(true);
	}
}

void AGCBaseCharacter::OnStopAimingInternal()
{
	if (OnAimingStateChanged.IsBound())
	{
		OnAimingStateChanged.Broadcast(false);
	}
}

void AGCBaseCharacter::TraceLineOfSight()
{
	if (!IsPlayerControlled())
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	APlayerController* PlayerController = GetController<APlayerController>();
	if (!IsValid(PlayerController))
	{
		return;
	}
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FVector ViewDirection = ViewRotation.Vector();
	FVector TraceEnd = ViewLocation + ViewDirection * LineOfSightDistance;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, TraceEnd, ECC_Visibility);
	
	if (LineOfSightObject.GetObject() != HitResult.Actor)
	{
		LineOfSightObject = HitResult.Actor.Get();

		FName ActionName;
		if (LineOfSightObject.GetInterface())
		{
			ActionName = LineOfSightObject->GetActionEventName();
		}
		else
		{
			ActionName = NAME_None;
		}
		OnInteractableObjectFound.ExecuteIfBound(ActionName);
	}	
}

void AGCBaseCharacter::TryChangeSprintState(float DeltaTime)
{
	if (GCBaseCharacterMovementComponent->IsOutOfStamina())
	{		
		return;		
	}	
	
	if (bIsSprintRequested && !GCBaseCharacterMovementComponent->IsSprinting() && CanSprint())
	{		
		GCBaseCharacterMovementComponent->StartSprint();
		OnSprintStart();
	}

	if (!bIsSprintRequested && GCBaseCharacterMovementComponent->IsSprinting())
	{
		GCBaseCharacterMovementComponent->StopSprint();
		OnSprintEnd();
	}
}

float AGCBaseCharacter::SignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint)
{
	if (ObjectInfo->GetTag() == SignificanceTagCharacter)
	{
		AGCBaseCharacter* Character = StaticCast<AGCBaseCharacter*>(ObjectInfo->GetObject());
		if (!IsValid(Character))
		{
			return SignificanceValueVeryHigh;
		}
		if (Character->IsPlayerControlled() && Character->IsLocallyControlled())
		{
			return SignificanceValueVeryHigh;
		}

		float DistToSquared = FVector::DistSquared(Character->GetActorLocation(), ViewPoint.GetLocation());
		if (DistToSquared <= FMath::Square(VeryHighSignificanceDistance))
		{
			return SignificanceValueVeryHigh;
		}
		else if (DistToSquared <= FMath::Square(HighSignificanceDistance))
		{
			return SignificanceValueHigh;
		}
		else if (DistToSquared <= FMath::Square(MediumSignificanceDistance))
		{
			return SignificanceValueMedium;
		}
		else if (DistToSquared <= FMath::Square(LowSignificanceDistance))
		{
			return SignificanceValueLow;
		}
		else
		{
			return SignificanceValueVeryLow;
		}
	}
	return  SignificanceValueVeryHigh;
}

void AGCBaseCharacter::PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
{
	if (OldSignificance == Significance)
	{
		return;
	}
	if (ObjectInfo->GetTag() != SignificanceTagCharacter)
	{
		return;
	}
	AGCBaseCharacter* Character = StaticCast<AGCBaseCharacter*>(ObjectInfo->GetObject());
	if (!IsValid(Character))
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement();
	AAIController* AIController = Character->GetController<AAIController>();
	UWidgetComponent* Widget = Character->HealthBarProgressComponent;


	if (Significance == SignificanceValueVeryHigh)
	{
		MovementComponent->SetComponentTickInterval(0.0f);
		Widget->SetVisibility(true);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.0f);
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.0f);
		}
	}
	else if (Significance == SignificanceValueHigh)
	{
		MovementComponent->SetComponentTickInterval(0.0f);
		Widget->SetVisibility(true);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.05f);
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.0f);
		}
	}
	else if (Significance == SignificanceValueMedium)
	{
		MovementComponent->SetComponentTickInterval(0.1f);
		Widget->SetVisibility(false);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(0.1f);
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(0.1f);
		}
	}
	else if (Significance == SignificanceValueLow)
	{
		MovementComponent->SetComponentTickInterval(1.0f);
		Widget->SetVisibility(false);
		Character->GetMesh()->SetComponentTickEnabled(true);
		Character->GetMesh()->SetComponentTickInterval(1.0f);
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(1.0f);
		}
	}
	else if (Significance == SignificanceValueVeryLow)
	{
		MovementComponent->SetComponentTickInterval(5.0f);
		Widget->SetVisibility(false);
		Character->GetMesh()->SetComponentTickEnabled(false);
		if (IsValid(AIController))
		{
			AIController->SetActorTickInterval(10.0f);
		}
	}
}

float AGCBaseCharacter::GetIKOffsetForASocket(const FName& SocketName)
{
	float Result = 0.0f;	
	
	FVector SocketLocation = GetMesh()->GetSocketLocation(SocketName);
	FVector TraceStart(SocketLocation.X, SocketLocation.Y, GetActorLocation().Z);
	FVector TraceEnd(SocketLocation.X, SocketLocation.Y, GetActorLocation().Z - GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - IKTraceDistance);		
	
	FHitResult HitResult;
	ETraceTypeQuery TraceType = UEngineTypes::ConvertToTraceType(ECC_Visibility);	

	if (UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEnd, TraceType, true, TArray<AActor*>(), EDrawDebugTrace::None, HitResult, true))
	{				
		Result = HitResult.Location.Z - GetMesh()->GetComponentLocation().Z;			
	}

	return Result;
}
