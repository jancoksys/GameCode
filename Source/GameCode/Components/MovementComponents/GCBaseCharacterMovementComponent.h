// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/GCBaseCharacter.h"
#include "GCBaseCharacterMovementComponent.generated.h"

struct FMantlingMovementParameters
{
	FVector InitialLocation = FVector::ZeroVector;
	FRotator InitialRotation = FRotator::ZeroRotator;

	FVector TargetLocation = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;

	FVector InitialAnimationLocation = FVector::ZeroVector;

	float Duration = 1.0f;
	float Starttime = 0.0f;

	UCurveVector* MantlingCurve;

	UPrimitiveComponent* MantlingPrimitiveComponent;

	FVector TargetToLedgeDistance;
};

UENUM(BlueprintType)
enum class ECustomMovementMode : uint8
{
	CMOVE_None = 0 UMETA(DisplayName = "None"),
	CMOVE_Mantling UMETA(DisplayName = "Mantling"),
	CMOVE_Ladder UMETA(DisplayName = "Ladder"),
	CMOVE_Zipline UMETA(DisplayName = "Zipline"),
	CMOVE_WallRun UMETA(DisplayName = "WallRun"),
	CMOVE_Sliding UMETA(DisplayName = "Sliding"),
	CMOVE_Max UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EDetachFromLadderMethod : uint8
{
	Fall = 0,
	ReachingTheTop,
	ReachingTheBottom,
	JumpOff
};

USTRUCT(BlueprintType)
struct FContextMovementSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bOrientRotationToMovement = false;
};

UCLASS()
class GAMECODE_API UGCBaseCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend class FSavedMove_GC;

public:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual float GetMaxSpeed() const override;

	bool IsSprinting() { return bIsSprintng; }
	void StartSprint();
	void StopSprint();
	void SetIsAiming(bool bIsAiming_In);
	bool IsOutOfStamina() const { return bIsOutOfStamina; }
	void SetIsOutOfStamina(bool bIsOutOfStamina_In);

	virtual void Crouch(bool bClientSimulation /* = false */) override;
	virtual void UnCrouch(bool bClientSimulation /* = false */) override;

	bool IsProne() const { return bIsProne; }		
	void SetWantsToProne(bool bWantsToProne_In);
	bool GetWantsToProne() const { return bWantsToProne; }
	void Prone();
	void UnProne(bool bWantsToStand);	

	void StartMantle(const FMantlingMovementParameters& MantlingParameters);
	void EndMantle();
	bool IsMantling() const;

	void AttachToLadder(const class ALadder* Ladder);
	float GetActorToCurrentLadderProjection(const FVector& Location) const;
	void DetachFromLadder(EDetachFromLadderMethod DetachFromLadderMethod = EDetachFromLadderMethod::Fall);
	bool IsOnLadder() const;
	const class ALadder* GetCurrentLadder();	
	float GetLadderSpeedRatio() const;

	virtual void PhysicsRotation(float DeltaTime) override;

	void AttachToZipline(const class AZipline* Zipline);
	bool IsOnZipline() const;
	void DetachFromZipline();

	void StartWallRun(const EWallRunSide& WallRunSide, const FVector& Direction);
	void EndWallRun();
	bool IsWallRunning() const;	
	const EWallRunSide GetCurrentWallRunSide() const;
	void GetWallRunSideAndDirection(const FVector& HitNormal, EWallRunSide& OutSide, FVector& Direction) const;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	
	bool IsSliding() const;
	void StartSlide();
	void StopSlide();	
	bool bWantToCrouchAfterSlide = false;
	const float GetCrouchAfterSlideHalfHeightAdjust() const;	

	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: sprint", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SprintSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: sprint")
	FContextMovementSettings SprintMovementSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: stamina", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float OutOfStaminaSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: stamina")
	FContextMovementSettings OutOfStaminaMovementSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: crouch")
	FContextMovementSettings CrouchMovementSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: prone", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ProneCapsuleRadius = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: prone")
	FContextMovementSettings ProneMovementSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: prone", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ProneCapsuleHalfHeight = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character movement: prone", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float MaxProneSpeed = 100.0f;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Swimming", meta = (ClampMin = "0", UIMin = "0"))
	float SwimmingCapsuleRadius = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Swimming", meta = (ClampMin = "0", UIMin = "0"))
	float SwimmingCapsuleHalfHeight = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Movement: Swimming")
	FContextMovementSettings SwimmingMovementSettings;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	void PhysMantling(float deltaTime, int32 Iterations);
	void PhysLadder(float deltaTime, int32 Iterations);
	void PhysZipline(float DeltaTime, int32 Iterations);
	void PhysWallRun(float deltaTime, int32 Iterations);
	void PhysSliding(float DeltaTime, int32 Iterations);
	

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ClimbingLadderMaxSpeed = 200.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float ClimbingLadderBrakingDeceleration = 2048.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float LadderToCharacterOffset = 60.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MaxLadderTopOffset = 90.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float MinLadderBottomOffset = 90.0f;

	UPROPERTY(Category = "Character Movement: Ladder", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float JumpOffFromLadderSpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement : Zipline", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ZiplineSpeed = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Movement : Zipline", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float ZiplineZAdjustment = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character: wall run", meta = (ClampMin = "0", UIMin = "0"))
	float MaxWallRunTime = 2.0f;

	FVector ZiplineDirection = FVector::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character: wall run", meta = (ClampMin = "0", UIMin = "0"))
	float MaxWallRunSpeed = 1000.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character: wall run", meta = (ClampMin = "0", UIMin = "0"))
	float WallRunJumpZVelocity = 700.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character: wall run", meta = (ClampMin = "0", UIMin = "0"))
	float WallRunLineTraceLength = 200.0f;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideSpeed = 1000.0f;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideCaspsuleHalfHeight = 60.0f;

	UPROPERTY(Category = "Character Movement: Slide", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
	float SlideMaxTime = 2.0f;

	AGCBaseCharacter* GetBaseCharacterOwner() const;

private:
	bool bIsSprintng;
	bool bIsProne = false;
	bool bWantsToProne = false;	
	bool bIsOutOfStamina = false;
	bool bIsAiming = false;

	FMantlingMovementParameters CurrentMantlingParameters;	
	FTimerHandle MantlingTimer;

	FTimerHandle ZiplineTimer;
	float ZiplineMovingTime = 0.0f;
	FVector ZiplineMovingStartLocation = FVector::ZeroVector;	

	const ALadder* CurrentLadder = nullptr;	

	FRotator ForcedTargetRotation = FRotator::ZeroRotator;
	bool bForceRotation = false;

	EWallRunSide CurrentWallRunSide = EWallRunSide::None;
	FVector CurrentWallRunDirection = FVector::ZeroVector;
	FTimerHandle WallRunTimer;
	FVector WallRunStartLocation;
	bool bIsWallRunning = false;
	bool AreWallRunRequiredKeysDown(EWallRunSide Side) const;

	float StandartJumpZVelocity;		

	FVector SlidingStartLocation = FVector::ZeroVector;
	FVector SlidingDirection = FVector::ZeroVector;
	FTimerHandle SlidingTimer;
	bool bIsSliding = false;
	float CrouchAfterSlideHalfHeightAdjust;
	FVector SlidingVerticalVelocityComponent = FVector::ZeroVector;
	FVector SlidingHorizontalVelocityComponent = FVector::ZeroVector;
	float SlidingGroundCheckingDistance;

	bool CanIncreaseCapsuleHalfHeight(float NewHalfHeight, float CurrentHalfHeight, float& OutHalfHeightAdjust) const;

	void ApplyContextMovementSettings(const FContextMovementSettings& MovementSettings);
	void RemoveContextMovementSettings(const FContextMovementSettings& MovementSettings);

	TArray<const FContextMovementSettings*> CurrentContextMovementSettings;
};

class FSavedMove_GC : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;
public:
	virtual void Clear() override;

	virtual uint8 GetCompressedFlags() const override;

	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* InCharacter, float MaxDelta) const override;

	virtual void SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;

	virtual void PrepMoveFor(ACharacter* Character) override;

private:
	uint8 bSavedIsSprinting : 1;
	uint8 bSavedIsMantling : 1;
	uint8 bSavedIsAiming : 1;
};

class FNetworkPredictionData_Client_Character_GC : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:
	FNetworkPredictionData_Client_Character_GC(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};
