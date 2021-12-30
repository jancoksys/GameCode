// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/Weapons/RangeWeaponItem.h"
#include "Bow.generated.h"

UENUM(BlueprintType)
enum class EBowProcessFlags : uint8
{
	FLAG_None,
	FLAG_DrawStarted,
	FLAG_HoldingStarted,
	FLAG_ShakingStarted
};

UCLASS()
class GAMECODE_API ABow : public ARangeWeaponItem
{
	GENERATED_BODY()

public:
	virtual void StartFire() override;
	virtual void StopFire() override;
	virtual void StartAiming() override;
	virtual void StopAiming() override;
	virtual FTransform GetFABRIKEffectorTransform() const override;
	virtual void StartReload() override;
	virtual void EndReload(bool bIsSuccess) override;
	void StartArrowDraw();
	virtual void Tick(float DeltaSeconds) override;
	void SetArrowVisibility(bool bIsVisible);
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming")
	UAnimMontage* BowAimMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float MinAimTime = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float MaxAimTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Spread", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ArrowSpreadAngleOnMinAim = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Spread", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ArrowSpreadAngleOnMaxAim = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Spread")
	UCurveFloat* SpreadCurveFloat;

	// Speed is in meters per second
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Speed", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ArrowSpeedOnMinAim = 10.0f;

	// Speed is in meters per second
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Speed", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ArrowSpeedOnMaxAim = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Speed")
	UCurveFloat* ArrowSpeedCurveFloat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float TimeAfterMaxAim = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Shaking", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float TimeToStartShaking = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Shaking", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ReticleShakingOffset = 0.1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Shaking", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ReticleShakingPeriod = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Shaking", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ReticleShakingOffsetOnEnd = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Aiming | Shaking")
	UCurveFloat* ReticleShakingCurveFloat;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Damage", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float DamageOnStartDraw = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Damage", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float DamageOnEnd = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bow | Damage")
	UCurveFloat* DamageCurveFloat;

	virtual void BeginPlay() override;

private:
	FTimerHandle ProgressBarTimer;
	FTimerHandle ProcessTimer;
	FTimerHandle ShakingTimer;
	EBowProcessFlags ProcessFlags = EBowProcessFlags::FLAG_None;

	float TotalTime = -1.0f;

	UFUNCTION(Server, Reliable)
	void Server_SetArrowVisibility(bool bIsVisible);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_SetArrowVisibility(bool bIsVisible);	

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopAiming();

	UFUNCTION(Server, Reliable)
	void Server_StopAiming();

	UFUNCTION()
	void EndMinArrowDraw();

	UFUNCTION()
	void OnStartShaking();

	UFUNCTION()
	void OnEndStringHolding();

	UFUNCTION()
	void OnAimEnded();

	UFUNCTION(Server, Reliable)
	void Server_PlayBowMontage(bool bStopMontage = false);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayBowMontage(bool bStopMontage = false);

	void ClearTimers();

	void ProcessShaking(float DeltaTime);

	float CurrentArrowSpreadAngle = -1.0f;
	float CurrentArrowSpeed = -1.0f;
	float CurrentDamage = -1.0f;
	float CurrentReticleOffset = -1.0f;

	float DamageIncreaseSpeed = -1.0f;
	float DamageOnEndDraw = -1.0f;
	float DamageOnStartShaking = -1.0f;

	float ShakeIncreasingSpeed = -1.0f;
};
