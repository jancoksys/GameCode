// Fill out your copyright notice in the Description page of Project Settings.


#include "RangeWeaponItem.h"
#include "Components/Weapon/WeaponBarellComponent.h"
#include "GameCodeTypes.h"
#include "Characters/GCBaseCharacter.h"
#include "Net/UnrealNetwork.h"

ARangeWeaponItem::ARangeWeaponItem()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	WeaponBarell = CreateDefaultSubobject<UWeaponBarellComponent>(TEXT("WeaponBarell"));
	WeaponBarell->SetupAttachment(WeaponMesh, SocketWeaponMuzzle);

	EquippedSocketName = SocketCharacterWeapon;

	ReticleType = EReticleType::Default;
}

void ARangeWeaponItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ProcessRecoil(DeltaSeconds);
	ProcessRecoilRollback(DeltaSeconds);
}

void ARangeWeaponItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARangeWeaponItem, FireModes);
}

void ARangeWeaponItem::StartFire()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(ShotTimer))
	{
		return;
	}

	bIsFiring = true;
	StopRecoilRollback();
	MakeShot();	
}

void ARangeWeaponItem::StopFire()
{
	bIsFiring = false;

	if (!GetCharacterOwner()->IsPlayerControlled())
	{
		return;
	}

	RecoilRollbackTime = AccumulatedShots * 60.0f / RecoilParameters.RecoilRollbackSpeed;
	if (AccumulatedRecoilPitch != 0.0f || AccumulatedRecoilYaw != 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(RecoilRollbackTimer, this, &ARangeWeaponItem::StopRecoilRollback, RecoilRollbackTime, false);
	}
}

void ARangeWeaponItem::ChangeFireMode()
{
	if (!bAllowAlternativeFireModes)
	{
		return;
	}
	
	if (CurrentFireMode + 1 >= FireModes.Num())
	{
		CurrentFireMode = 0;
	}
	else
	{
		++CurrentFireMode;
	}

	WeaponBarell->SetHitRegistration(FireModes[CurrentFireMode].HitRegistrationType);
	WeaponBarell->SetProjectile(FireModes[CurrentFireMode].ProjectileClass);

	if (OnAmmoChanged.IsBound())
	{
		OnAmmoChanged.Broadcast(FireModes[CurrentFireMode].Ammo);
	}
}

void ARangeWeaponItem::StartAiming()
{
	bIsAiming = true;
}

void ARangeWeaponItem::StopAiming()
{
	bIsAiming = false;
}

FTransform ARangeWeaponItem::GetFABRIKEffectorTransform() const
{
	return WeaponMesh->GetSocketTransform(SocketWeaponForeGrip);
}

float ARangeWeaponItem::GetAimFOV() const
{
	return AimFOV;
}

float ARangeWeaponItem::GetAimMovementMaxSpeed() const
{
	return AimMovementMaxSpeed;
}

float ARangeWeaponItem::GetAimTurnModifier() const
{
	return AimTurnModifier;
}

float ARangeWeaponItem::GetAimLookUpModifier() const
{
	return AimLookUpModifier;
}

int32 ARangeWeaponItem::GetAmmo() const
{
	return FireModes[CurrentFireMode].Ammo;	
}

int32 ARangeWeaponItem::GetMaxAmmo() const
{	
	return FireModes[CurrentFireMode].MaxAmmo;
}

void ARangeWeaponItem::SetAmmo(int32 NewAmmo)
{	
	FireModes[CurrentFireMode].Ammo = NewAmmo;
	if (OnAmmoChanged.IsBound())
	{
		OnAmmoChanged.Broadcast(NewAmmo);
	}
}

bool ARangeWeaponItem::CanShoot() const
{	
	return FireModes[CurrentFireMode].Ammo > 0;
}

EAmunitionType ARangeWeaponItem::GetAmmoType() const
{
	return FireModes[CurrentFireMode].AmmoType;
}

void ARangeWeaponItem::StartReload()
{
	if (GetCharacterOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_StartReload();
		StartReload_Internal();
	}
	else if (GetCharacterOwner()->GetLocalRole() == ROLE_Authority)
	{
		Server_StartReload();
	}
}

void ARangeWeaponItem::EndReload(bool bIsSuccess)
{
	if (!bIsReloading)
	{
		return;
	}
	
	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();	

	if (!bIsSuccess)
	{
		if (!IsValid(CharacterOwner))
		{
			return;
		}
		CharacterOwner->StopAnimMontage(CharacterReloadMontage);
		StopAnimMontage(WeaponReloadMontage);
	}

	if (FireModes[CurrentFireMode].ReloadType == EReloadType::ByBullet)
	{
		UAnimInstance* CharacterAnimInstance = IsValid(CharacterOwner) ? CharacterOwner->GetMesh()->GetAnimInstance() : nullptr;
		if (IsValid(CharacterAnimInstance))
		{
			CharacterAnimInstance->Montage_JumpToSection(SectionMontageReloadEnd, CharacterReloadMontage);
		}
		UAnimInstance* WeaponAnimInstance = WeaponMesh->GetAnimInstance();
		if (IsValid(WeaponAnimInstance))
		{
			WeaponAnimInstance->Montage_JumpToSection(SectionMontageReloadEnd, WeaponReloadMontage);
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	bIsReloading = false;
	if (bIsSuccess && OnReloadComplete.IsBound())
	{
		OnReloadComplete.Broadcast();
	}
}

EReticleType ARangeWeaponItem::GetReticleType() const
{
	return bIsAiming ? AimReticleType : ReticleType;
}

bool ARangeWeaponItem::IsFiring() const
{
	return bIsFiring;
}

bool ARangeWeaponItem::IsReloading() const
{
	return bIsReloading;
}

void ARangeWeaponItem::OnLevelDeserialized_Implementation()
{
	SetActorRelativeTransform(FTransform(FRotator::ZeroRotator, FVector::ZeroVector));
	if (OnAmmoChanged.IsBound())
	{
		OnAmmoChanged.Broadcast(FireModes[CurrentFireMode].Ammo);
	}
}

void ARangeWeaponItem::StopRecoilRollback()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(RecoilRollbackTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(RecoilRollbackTimer);
	}
	AccumulatedShots = 0;
	AccumulatedRecoilYaw = 0.0f;
	AccumulatedRecoilPitch = 0.0f;
}

void ARangeWeaponItem::BeginPlay()
{
	Super::BeginPlay();	
	if (FireModes.Num() == 0)
	{
		FireModes.Add(FFireModeSettings{});
	}

	for (FFireModeSettings& FireMode : FireModes)
	{
		FireMode.Ammo = FireMode.MaxAmmo;
		if (FireMode.HitRegistrationType == EHitRegistrationType::Projectile)
		{			
			WeaponBarell->SetProjectile(FireMode.ProjectileClass);
			WeaponBarell->InitProjectilePool();
		}
	}	
}

void ARangeWeaponItem::OnShotTimerElapsed()
{
	if (!bIsFiring)
	{
		return;
	}
	
	switch (FireModes[CurrentFireMode].FireMode)
	{
	case EWeaponFireMode::Single:
	{
		StopFire();
		break;
	}
	case EWeaponFireMode::FullAuto:
	{
		MakeShot();
		break;
	}
	}
}

float ARangeWeaponItem::PlayAnimMontage(UAnimMontage* AnimMontage)
{
	UAnimInstance* WeaponAnimInstance = WeaponMesh->GetAnimInstance();
	float Result = 0.0f;
	if (IsValid(WeaponAnimInstance))
	{
		Result = WeaponAnimInstance->Montage_Play(AnimMontage);
	}
	return Result;
}

void ARangeWeaponItem::StopAnimMontage(UAnimMontage* AnimMontage, float BlendOutTime /*= 0.0f*/)
{
	UAnimInstance* WeaponAnimInstance = WeaponMesh->GetAnimInstance();
	if (IsValid(WeaponAnimInstance))
	{
		WeaponAnimInstance->Montage_Stop(BlendOutTime, AnimMontage);
	}
}

float ARangeWeaponItem::GetShotTimerInterval() const
{
	return 60.0f / FireModes[CurrentFireMode].RateOfFire;
}

void ARangeWeaponItem::MakeShot()
{
	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	if (!CanShoot())
	{
		StopFire();
		if (FireModes[CurrentFireMode].Ammo == 0 && bAutoReload)
		{
			CharacterOwner->Reload();
		}
		return;
	}
	EndReload(false);

	CharacterOwner->PlayAnimMontage(CharacterFireMontage);
	PlayAnimMontage(WeaponFireMontage);

	FVector ShotLocation;
	FRotator ShotRotation;
	if (CharacterOwner->IsPlayerControlled())
	{
		APlayerController* Controller = CharacterOwner->GetController<APlayerController>();
		Controller->GetPlayerViewPoint(ShotLocation, ShotRotation);
	}
	else
	{
		ShotLocation = WeaponBarell->GetComponentLocation();
		ShotRotation = CharacterOwner->GetBaseAimRotation();
	}	
	
	FVector ShotDirection = ShotRotation.RotateVector(FVector::ForwardVector);
	
	SetAmmo(FireModes[CurrentFireMode].Ammo - 1);		

	WeaponBarell->Shot(ShotLocation, ShotDirection, GetCurrentBulletSpreadAngle(), FireModes[CurrentFireMode].bCanProjectileHit);

	GetWorld()->GetTimerManager().SetTimer(ShotTimer, this, &ARangeWeaponItem::OnShotTimerElapsed, GetShotTimerInterval(), false);
	GetWorld()->GetTimerManager().SetTimer(RecoilTimer, GetRecoilTimeInterval(), false);

	if (IsValid(ShotCameraShakeClass) && GetCharacterOwner()->IsPlayerControlled())
	{
		APlayerController* Controller = CharacterOwner->GetController<APlayerController>();
		Controller->PlayerCameraManager->PlayCameraShake(ShotCameraShakeClass);
	}

	++AccumulatedShots;
}

void ARangeWeaponItem::ProcessRecoil(float DeltaTime)
{
	if (!GetWorld()->GetTimerManager().IsTimerActive(RecoilTimer))
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

	float RecoilPitch = -RecoilParameters.RecoilPitch / GetRecoilTimeInterval() * DeltaTime;
	CharacterOwner->AddControllerPitchInput(RecoilPitch);
	AccumulatedRecoilPitch += RecoilPitch;
		
	float RecoilYaw = RecoilParameters.RecoilYaw / GetRecoilTimeInterval() * DeltaTime;
	CharacterOwner->AddControllerYawInput(RecoilYaw);
	AccumulatedRecoilYaw += RecoilYaw;
}

float ARangeWeaponItem::GetRecoilTimeInterval() const
{
	return FMath::Min(GetShotTimerInterval(), RecoilParameters.RecoilTime);
}

void ARangeWeaponItem::ProcessRecoilRollback(float DeltaTime)
{
	if (!GetWorld()->GetTimerManager().IsTimerActive(RecoilRollbackTimer))
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

	float RollbackPitch = -AccumulatedRecoilPitch / RecoilRollbackTime * DeltaTime;
	CharacterOwner->AddControllerPitchInput(RollbackPitch);

	float RollbackYaw = -AccumulatedRecoilYaw / RecoilRollbackTime * DeltaTime;
	CharacterOwner->AddControllerYawInput(RollbackYaw);
}

float ARangeWeaponItem::GetCurrentBulletSpreadAngle() const
{
	float AngleInDegrees = bIsAiming ? AimSpreadAngle : SpreadAngle;
	return FMath::DegreesToRadians(AngleInDegrees);
}

void ARangeWeaponItem::StartReload_Internal()
{
	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	bIsReloading = true;
	if (IsValid(CharacterReloadMontage))
	{
		float MontageDuration = CharacterOwner->PlayAnimMontage(CharacterReloadMontage);
		PlayAnimMontage(WeaponReloadMontage);
		if (FireModes[CurrentFireMode].ReloadType == EReloadType::FullClip)
		{
			GetWorld()->GetTimerManager().SetTimer(ReloadTimer, [this]() { EndReload(true); }, MontageDuration, false);
		}
	}
	else
	{
		EndReload(true);
	}
}

void ARangeWeaponItem::Multicast_StartReload_Implementation()
{
	if (GetCharacterOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		StartReload_Internal();
	}	
}

void ARangeWeaponItem::Server_StartReload_Implementation()
{
	Multicast_StartReload();
	StartReload_Internal();
}
