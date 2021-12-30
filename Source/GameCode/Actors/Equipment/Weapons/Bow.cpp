// Fill out your copyright notice in the Description page of Project Settings.


#include "Bow.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/CharacterComponents/CharacterEquipmentComponent.h"
#include "Components/Weapon/WeaponBarellComponent.h"

void ABow::StartFire()
{
	if (!bIsAiming)
	{
		return;
	}

	if (ProcessFlags == EBowProcessFlags::FLAG_DrawStarted)
	{
		float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(ProcessTimer);
		float TimerRatio = ElapsedTime / MaxAimTime;			

		if (IsValid(ArrowSpeedCurveFloat))
		{
			CurrentArrowSpeed = ArrowSpeedCurveFloat->GetFloatValue(TimerRatio);
		}
		else
		{
			CurrentArrowSpeed = ArrowSpeedOnMinAim + TimerRatio * (ArrowSpeedOnMaxAim - ArrowSpeedOnMinAim);
		}

		if (IsValid(SpreadCurveFloat))
		{
			AimSpreadAngle = SpreadCurveFloat->GetFloatValue(TimerRatio);
		}
		else
		{
			AimSpreadAngle = ArrowSpreadAngleOnMaxAim + (1 - TimerRatio) * (ArrowSpreadAngleOnMinAim - ArrowSpreadAngleOnMaxAim);
		}			

		WeaponBarell->SetProjectileSpeed(CurrentArrowSpeed * 100.0f);
	}
	else if (ProcessFlags >= EBowProcessFlags::FLAG_HoldingStarted)
	{
		AimSpreadAngle = ArrowSpreadAngleOnMaxAim;
		WeaponBarell->SetProjectileSpeed(ArrowSpeedOnMaxAim * 100.0f);
	}

	if (ProcessFlags > EBowProcessFlags::FLAG_None)
	{
		ProcessFlags = EBowProcessFlags::FLAG_None;
		ClearTimers();
		bIsFiring = true;
		WeaponBarell->SetDamageAmount(CurrentDamage);
		MakeShot();
		SetArrowVisibility(false);
		
		if (GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.IsBound())
		{
			GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.Broadcast(0.0f);
		}
	}
}

void ABow::StopFire()
{
	Super::StopFire();
	SetArrowVisibility(false);
	ProcessFlags = EBowProcessFlags::FLAG_None;
	if (bIsAiming)
	{
		GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->ReloadCurrentWeapon();
	}
}

void ABow::StartAiming()
{
	Super::StartAiming();
	GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->ReloadCurrentWeapon();
}

void ABow::StopAiming()
{
	Super::StopAiming();
	SetArrowVisibility(false);
	ClearTimers();
	ProcessFlags = EBowProcessFlags::FLAG_None;
	if (GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.IsBound())
	{		
		GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.Broadcast(0.0f);
	}

	if (GetCharacterOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_StopAiming();
		GetCharacterOwner()->StopAnimMontage(CharacterReloadMontage);
		StopAnimMontage(BowAimMontage);
		Server_PlayBowMontage(true);
	}
	else if (GetCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		Server_StopAiming();
		Server_PlayBowMontage(true);
	}
}

FTransform ABow::GetFABRIKEffectorTransform() const
{
	return WeaponMesh->GetSocketTransform(SocketBowMidString);
}

void ABow::StartReload()
{
	if (!bIsAiming)
	{
		return;
	}
	Super::StartReload();
}

void ABow::EndReload(bool bIsSuccess)
{
	Super::EndReload(bIsSuccess);
	if (!bIsAiming)
	{
		SetArrowVisibility(false);
		ClearTimers();
		
		CurrentArrowSpeed = -1.0f;
		CurrentArrowSpreadAngle = -1.0f;
		CurrentDamage = -1.0f;

		ProcessFlags = EBowProcessFlags::FLAG_None;
		if (GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.IsBound())
		{		
			GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.Broadcast(0.0f);
		}
	}
	else
	{
		SetArrowVisibility(true);
		StartArrowDraw();
	}
}

void ABow::BeginPlay()
{
	Super::BeginPlay();
	SetAmmo(0);
	DamageIncreaseSpeed = (DamageOnEnd - DamageOnStartDraw) / (MaxAimTime + TimeAfterMaxAim);
	DamageOnEndDraw = DamageOnStartDraw + DamageIncreaseSpeed * MaxAimTime;
	DamageOnStartShaking = DamageOnStartDraw + DamageIncreaseSpeed * (MaxAimTime + TimeToStartShaking);
	TotalTime = MinAimTime + MaxAimTime + TimeAfterMaxAim;
	ShakeIncreasingSpeed = (ReticleShakingOffsetOnEnd - ReticleShakingOffset) / (TimeAfterMaxAim - TimeToStartShaking);
	CurrentReticleOffset = ReticleShakingOffset;
}

void ABow::StartArrowDraw()
{
	if (GetCharacterOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		PlayAnimMontage(BowAimMontage);
		Server_PlayBowMontage();
	}
	else if (GetCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		Server_PlayBowMontage();
	}
	
	GetWorld()->GetTimerManager().SetTimer(ProcessTimer, this, &ABow::EndMinArrowDraw, MinAimTime);
}

void ABow::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ProcessFlags > EBowProcessFlags::FLAG_None && GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.IsBound())
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(ProgressBarTimer))
		{
			float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(ProgressBarTimer);

			if (IsValid(DamageCurveFloat))
			{
				CurrentDamage = DamageCurveFloat->GetFloatValue(ElapsedTime / (TimeAfterMaxAim + MaxAimTime));
			}
			else
			{
				CurrentDamage = DamageOnStartDraw + (ElapsedTime / TotalTime) * (DamageOnEnd - DamageOnStartDraw);
			}			
			float Percent = CurrentDamage / DamageOnEnd;
			GetCharacterOwner()->GetCharacterEquipmentComponent_Mutable()->OnDamageChanged.Broadcast(Percent);			
		}		
	}
	ProcessShaking(DeltaSeconds);
}

void ABow::Server_StopAiming_Implementation()
{
	Multicast_StopAiming();
	GetCharacterOwner()->StopAnimMontage(CharacterReloadMontage);
}

void ABow::Multicast_StopAiming_Implementation()
{
	if (GetCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		GetCharacterOwner()->StopAnimMontage(CharacterReloadMontage);
	}	
}

void ABow::Server_SetArrowVisibility_Implementation(bool bIsVisible)
{
	GetCharacterOwner()->SetHandArrowVisibility(bIsVisible);
	Multicast_SetArrowVisibility(bIsVisible);
}

void ABow::Multicast_SetArrowVisibility_Implementation(bool bIsVisible)
{
	if (GetCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		GetCharacterOwner()->SetHandArrowVisibility(bIsVisible);
	}
}

void ABow::SetArrowVisibility(bool bIsVisible)
{
	if (GetCharacterOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		GetCharacterOwner()->SetHandArrowVisibility(bIsVisible);
		Server_SetArrowVisibility(bIsVisible);
	}
	else if (GetCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		Server_SetArrowVisibility(bIsVisible);
	}
}

void ABow::EndMinArrowDraw()
{
	SetArrowVisibility(true);
	ProcessFlags = EBowProcessFlags::FLAG_DrawStarted;
	GetWorld()->GetTimerManager().SetTimer(ProgressBarTimer, TimeAfterMaxAim + MaxAimTime, false);

	GetWorld()->GetTimerManager().ClearTimer(ProcessTimer);
	GetWorld()->GetTimerManager().SetTimer(ProcessTimer, this, &ABow::OnAimEnded, MaxAimTime);
	
	CurrentArrowSpeed = ArrowSpeedOnMinAim;
	CurrentArrowSpreadAngle = ArrowSpreadAngleOnMinAim;
	CurrentDamage = DamageOnStartDraw;
}

void ABow::OnStartShaking()
{
	ProcessFlags = EBowProcessFlags::FLAG_ShakingStarted;
	GetWorld()->GetTimerManager().ClearTimer(ProcessTimer);
	GetWorld()->GetTimerManager().SetTimer(ProcessTimer, this, &ABow::OnEndStringHolding, TimeAfterMaxAim - TimeToStartShaking);
}

void ABow::OnEndStringHolding()
{
	ClearTimers();
	StartFire();
	StopFire();
}

void ABow::OnAimEnded()
{
	ProcessFlags = EBowProcessFlags::FLAG_HoldingStarted;
	GetWorld()->GetTimerManager().ClearTimer(ProcessTimer);
	GetWorld()->GetTimerManager().SetTimer(ProcessTimer, this, &ABow::OnStartShaking, TimeToStartShaking);
	GetCharacterOwner()->StopAnimMontage(CharacterReloadMontage);
}

void ABow::Server_PlayBowMontage_Implementation(bool bStopMontage)
{
	if (!bStopMontage)
	{
		PlayAnimMontage(BowAimMontage);
	}
	else
	{
		StopAnimMontage(BowAimMontage);
	}
	Multicast_PlayBowMontage(bStopMontage);
}

void ABow::Multicast_PlayBowMontage_Implementation(bool bStopMontage)
{
	if (GetCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		if (!bStopMontage)
		{
			PlayAnimMontage(BowAimMontage);
		}
		else
		{
			StopAnimMontage(BowAimMontage);
		}
	}
}

void ABow::ClearTimers()
{
	GetWorld()->GetTimerManager().ClearTimer(ProcessTimer);
	GetWorld()->GetTimerManager().ClearTimer(ProgressBarTimer);
	GetWorld()->GetTimerManager().ClearTimer(ShakingTimer);
}

void ABow::ProcessShaking(float DeltaTime)
{
	if (ProcessFlags != EBowProcessFlags::FLAG_ShakingStarted)
	{
		return;
	}

	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	if (!CharacterOwner->IsPlayerControlled() || !CharacterOwner->IsLocallyControlled())
	{
		return;
	}

	if (IsValid(ReticleShakingCurveFloat))
	{
		float Percent = GetWorld()->GetTimerManager().GetTimerElapsed(ProcessTimer) / (TimeAfterMaxAim - TimeToStartShaking);
		CurrentReticleOffset = ReticleShakingCurveFloat->GetFloatValue(Percent);
	}
	else
	{
		CurrentReticleOffset += ShakeIncreasingSpeed * DeltaTime;
	}	

	FVector2D ReticleOffsetCircle = FMath::RandPointInCircle(CurrentReticleOffset);
	float ScalarToCircle = FMath::Sqrt(CurrentReticleOffset / (FMath::Square(ReticleOffsetCircle.X) + FMath::Square(ReticleOffsetCircle.Y)));

	float ReticlePitch = ReticleOffsetCircle.X * ScalarToCircle / ReticleShakingPeriod * DeltaTime;
	CharacterOwner->AddControllerPitchInput(ReticlePitch);
		
	float ReticleYaw = ReticleOffsetCircle.Y * ScalarToCircle / ReticleShakingPeriod * DeltaTime;
	CharacterOwner->AddControllerYawInput(ReticleYaw);
}
