// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameCodeTypes.h"
#include "GenericTeamAgentInterface.h"
#include "SignificanceManager.h"
#include "Subsystems/SaveSubsystem/SaveSubsystemInterface.h"
#include "UObject/ScriptInterface.h"
#include "GCBaseCharacter.generated.h"


class UCurveVector;
class UGCBaseCharacterMovementComponent;
class AInteractiveActor;
class UAnimMontage;
class UCharacterEquipmentComponent;
class UCharacterAttributeComponent;
class IInteractable;
class UWidgetComponent;
class UInventoryItem;
class UCharacterInventoryComponent;

typedef TArray<AInteractiveActor*, TInlineAllocator<10>> TInteractiveActorsArray;

UENUM(BlueprintType)
enum class EWallRunSide : uint8
{
	None = 0,
	Left,
	Right
};

USTRUCT(BlueprintType)
struct FMantlingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UAnimMontage* MantlingMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UAnimMontage* FPMantlingMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UCurveVector* MantlingCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AnimationCorrectionXY = 65.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AnimationCorrectionZ = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHeight = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MinHeight = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxHeightStartTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MinHeightStartTime = 0.5f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAimingStateChanged, bool);
DECLARE_DELEGATE_OneParam(FOnInteractableObjectFound, FName);

UCLASS(Abstract, NotBlueprintable)
class GAMECODE_API AGCBaseCharacter : public ACharacter, public IGenericTeamAgentInterface, public ISaveSubsystemInterface
{
	GENERATED_BODY()

public:
	AGCBaseCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void MoveForward(float Value);
	virtual void MoveRight(float Value);
	virtual void Jump() override;
	virtual void Turn(float Value) {};
	virtual void LookUp(float Value) {};
	virtual void TurnAtRate(float Value) {};
	virtual void LookUpAtRate(float Value) {};
	virtual void ChangeCrouchState();
	virtual void ChangeProneState();
	virtual void Prone();
	virtual void UnProne();
	virtual void StartSprint();
	virtual void StopSprint();
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void Mantle(bool bForce = false);
	virtual void SwimForward(float Value) {};
	virtual void SwimRight(float Value) {};
	virtual void SwimUp(float Value) {};
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void StartSlide();
	virtual void StopSlide();

	//@ ISaveSubsystemInterface
	virtual void OnLevelDeserialized_Implementation() override;
	//~ISaveSubsystemInterface

	void StartFire();

	bool CanFire() const;

	void ChangeFireMode();
	void StopFire();
	void StartAiming();
	void StopAiming();
	FRotator GetAimOffset();

	void Reload();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
	void OnStartAiming();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Character")
	void OnStopAiming();

	float GetAimingMovementSpeed() const;

	UGCBaseCharacterMovementComponent* GetBaseCharacterMovementComponent() const { return GCBaseCharacterMovementComponent; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetIKLeftFootOffset() const { return IKLeftFootOffset; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetIKRightFootOffset() const { return IKRightFootOffset; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetIKBodyOffset() const { return IKBodyOffset; }

	virtual void OnStartProne(float HeightAdjust, float ScaledHeightAdjust);
	virtual void OnEndProne(float HeightAdjust, float ScaledHeightAdjust);
	virtual void OnStandAfterProne(float HeightAdjust, float ScaledHeightAdjust);

	virtual bool CanJumpInternal_Implementation() const override;
	
	void RegisterInteractiveActor(AInteractiveActor* InteractiveActor);
	void UnregisterInteractiveActor(AInteractiveActor* InteractiveActor);

	void ClimbLadder(float Value);
	void InteractWithLadder();
	const class ALadder* GetAvailableLadder() const;
	void InteractWithZipline();
	const class AZipline* GetAvailableZipline() const;

	void GetPlayerMovingInput(float& ForwardInput, float& RightInput) const;
	void OnStartSliding(float HeightAdjust, float ScaledHeightAdjust);
	void OnEndSliding(float HeightAdjust, float ScaledHeightAdjust);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void Falling() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void NotifyJumpApex() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Character | Movement")
	void OnSprintStart();
	virtual void OnSprintStart_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Character | Movement")
	void OnSprintEnd();
	virtual void OnSprintEnd_Implementation();

	bool IsSwimmingUnderWater() const;

	const UCharacterEquipmentComponent* GetCharacterEquipmentComponent() const;
	UCharacterEquipmentComponent* GetCharacterEquipmentComponent_Mutable() const;
	bool IsAiming() const;
	FOnAimingStateChanged OnAimingStateChanged;

	const UCharacterAttributeComponent* GetCharacterAttributesComponent() const;
	UCharacterAttributeComponent* GetCharacterAttributesComponent_Mutable() const;

	UCharacterInventoryComponent* GetCharacterInventoryComponent_Mutable() const;
	void NextItem();
	void PreviousItem();
	void EquipPrimaryItem();

	void PrimaryMeleeAttack();
	void SecondaryMeleeAttack();

	/** IGenericTeamAgentInterface */
	virtual FGenericTeamId GetGenericTeamId() const override;
	/** ~IGenericTeamAgentInterface */

	UPROPERTY(ReplicatedUsing=OnRep_IsMantling)
	bool bIsMantling;

	UFUNCTION()
	void OnRep_IsMantling(bool bWasMantling);

	void Interact();

	FOnInteractableObjectFound OnInteractableObjectFound;

	UPROPERTY(VisibleDefaultsOnly, Category = "Character | Components")
	UWidgetComponent* HealthBarProgressComponent;

	void InitializeHealthProgress();

	bool PickupItem(TWeakObjectPtr<UInventoryItem> ItemToPickup);
	void UseInventory(APlayerController* PlayerController);
	void ConfirmWeaponSelection();
	bool IsMontagePlaying(UAnimMontage* Montage_In) const;
	void SetHandArrowVisibility(bool bIsVisible);
	void SetCurrentAimingMovementSpeed(float Speed_In);
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK settings")
	FName RightFootToeSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK settings")
	FName LeftFootToeSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK settings")
	FName RightFootHeelSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | IK settings")
	FName LeftFootHeelSocketName;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Controls")
	float BaseTurnRate = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Controls")
	float BaseLookUpRate = 45.0f;		

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Character | IK settings", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float IKInterpSpeed = 20.0f;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSettings HighMantleSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling")
	FMantlingSettings LowMantleSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Mantling", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float LowMantleMaxHeight = 125.0f;

	virtual bool CanSprint();

	virtual bool CanProne();
	virtual bool CanCrouch() const override;

	bool CanMantle() const;
	bool CanSlide() const;
	bool CanInteractWithLadder() const;

	virtual void OnMantle(const FMantlingSettings& MantlingSettings, float MantlingAnimationStartTime);

	UGCBaseCharacterMovementComponent* GCBaseCharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Movement | Ledge Detection")
	class ULedgeDetectorComponent* LedgeDetectorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Character | Spring arm component")
	float DefaultSpringArmLength = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Sliding")
	UAnimMontage* SlidingMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Sliding", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SlidingToCrouchBlendOutTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character | Movement | Landing", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float HardLandingMinHeight = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Movement | Landing")
	UAnimMontage* HardLandingMontage;

	virtual void OnHardLanded();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterAttributeComponent* CharacterAttributesComponent;

	virtual void OnDeath();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Animations")
	UAnimMontage* OnDeathAnimMontage;

	// Damage depending from fall height (in meters)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character | Attributes")
	class UCurveFloat* FallDamageCurve;

	float ForwardMovingInput = 0.0f;
	float RightMovingInput = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterEquipmentComponent* CharacterEquipmentComponent;

	bool bIsAiming = false;

	virtual void OnStartAimingInternal();
	virtual void OnStopAimingInternal();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Team")
	ETeams Team = ETeams::Enemy;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	float LineOfSightDistance = 500.0f;

	void TraceLineOfSight();

	UPROPERTY()
	TScriptInterface<IInteractable> LineOfSightObject;	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character | Components")
	UCharacterInventoryComponent* CharacterInventoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	bool bIsSignificanceEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float VeryHighSignificanceDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float HighSignificanceDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float MediumSignificanceDistance = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character | Significance")
	float LowSignificanceDistance = 6000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Arrows")
	UStaticMeshComponent* ArrowMeshComponent;

private:
	float SignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& ViewPoint);
	void PostSignificanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal);

	float GetIKOffsetForASocket(const FName& SocketName);
	
	void TryChangeSprintState(float DeltaTime);	
	void ChangeStaminaCondition(bool IsOutOfStamina_In);
	void UpdateFootIK(float DeltaTime);		

	bool bIsSprintRequested = false;

	float IKRightFootHeelOffset = 0.0f;	
	float IKLeftFootHeelOffset = 0.0f;
	float IKRightFootToeOffset = 0.0f;
	float IKLeftFootToeOffset = 0.0f;

	float IKRightFootOffset = 0.0f;
	float IKLeftFootOffset = 0.0f;

	float IKBodyOffset = 0.0f;	

	float IKTraceDistance = 0.0f;
	float IKScale = 1.0f;		

	const FMantlingSettings& GetMantlingSettings(float LedgeHeight) const;

	TInteractiveActorsArray AvailableInteractiveActors;	

	UFUNCTION()
	void OnPlayerCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool IsSurfaceWallRunnable(AActor* SurfaceActor) const;		

	FVector CurrentWallRunDirection = FVector::ZeroVector;
	AActor* CurrentWallRunActor = nullptr;
	
	FTimerHandle HardLandingTimer;
	void SetWalkingMovementMode();
	
	void EnableRagdoll();

	FVector CurrentFallApex;

	float CurrentAimingMovementSpeed;
};