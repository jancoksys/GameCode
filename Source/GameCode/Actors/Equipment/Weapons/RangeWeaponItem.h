// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"
#include "Subsystems/SaveSubsystem/SaveSubsystemInterface.h"
#include "RangeWeaponItem.generated.h"

/**
 * 
 */

DECLARE_MULTICAST_DELEGATE(FOnReloadComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32);

UENUM(BlueprintType)
enum class EWeaponFireMode : uint8
{
	Single,
	FullAuto
};

UENUM(BlueprintType)
enum class EReloadType : uint8
{
	FullClip,
	ByBullet
};

USTRUCT(BlueprintType)
struct FRecoilParameters
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = -2.0f, UIMin = -2.0f, ClampMax = 2.0f, UIMax = 2.0f))
	float RecoilYaw = -0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = -2.0f, UIMin = -2.0f, ClampMax = 2.0f, UIMax = 2.0f))
	float RecoilPitch = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float RecoilTime = 0.05f;

	// Recoil rollback in shots per minute
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float RecoilRollbackSpeed = 1200.0f;
};

USTRUCT(BlueprintType)
struct FFireModeSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", SaveGame)
	EWeaponFireMode FireMode = EWeaponFireMode::Single;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", SaveGame)
	EAmunitionType AmmoType = EAmunitionType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", meta = (ClampMin = 1, UIMin = 1), SaveGame)
	int32 MaxAmmo = 30;

	//FullClip reload type adds ammo only when the whole reload animation is successfully played
	//ByBullet reload type requires section "ReloadEnd" in character reload animation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", SaveGame)
	EReloadType ReloadType = EReloadType::FullClip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", meta = (ClampMin = 1.0f, UIMin = 1.0f), SaveGame)
	float RateOfFire = 600.0f;

	UPROPERTY(SaveGame)
	int32 Ammo = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", SaveGame)
	EHitRegistrationType HitRegistrationType = EHitRegistrationType::HitScan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", meta = (EditCondition = "HitRegistrationType == EHitRegistrationType::Projectile"), SaveGame)
	TSubclassOf<class AGCProjectile> ProjectileClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire mode settings", meta = (EditCondition = "HitRegistrationType == EHitRegistrationType::Projectile"), SaveGame)
	bool bCanProjectileHit = false;
};

class UAnimMontage;
class UCameraShake;
UCLASS(Blueprintable)
class GAMECODE_API ARangeWeaponItem : public AEquipableItem, public ISaveSubsystemInterface
{
	GENERATED_BODY()
	
public:
	ARangeWeaponItem();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void StartFire();
	virtual void StopFire();
	void ChangeFireMode();
	virtual void StartAiming();
	virtual void StopAiming();
	virtual FTransform GetFABRIKEffectorTransform() const;
	float GetAimFOV() const;
	float GetAimMovementMaxSpeed() const;
	float GetAimTurnModifier() const;
	float GetAimLookUpModifier() const;
	int32 GetAmmo() const;
	int32 GetMaxAmmo() const;
	void SetAmmo(int32 NewAmmo);
	bool CanShoot() const;

	FOnAmmoChanged OnAmmoChanged;
	FOnReloadComplete OnReloadComplete;

	EAmunitionType GetAmmoType() const;
	virtual void StartReload();
	virtual void EndReload(bool bIsSuccess);
	virtual EReticleType GetReticleType() const override;

	bool IsFiring() const;
	bool IsReloading() const;
	virtual void OnLevelDeserialized_Implementation() override;

	void StopRecoilRollback();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWeaponBarellComponent* WeaponBarell;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	UAnimMontage* WeaponFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Character")
	UAnimMontage* CharacterFireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Weapon")
	UAnimMontage* WeaponReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations | Character")
	UAnimMontage* CharacterReloadMontage;

	//If unchecked, only Default FireModeSetting will be used.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Fire modes")
	bool bAllowAlternativeFireModes = false;	

	//Fire modes for current weapon. First mode always used as a default.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Fire modes", SaveGame, Replicated)
	TArray<FFireModeSettings> FireModes;

	//Bullet spread half angle in degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2.0f, UIMax = 2.0f))
	float SpreadAngle = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2.0f, UIMax = 2.0f))
	float AimSpreadAngle = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AimMovementMaxSpeed = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 120.0f, UIMax = 120.0f))
	float AimFOV = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AimTurnModifier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Aiming", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float AimLookUpModifier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Ammo")
	bool bAutoReload = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Recoil")
	FRecoilParameters RecoilParameters;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon | Parameters | Recoil")
	TSubclassOf<UCameraShake> ShotCameraShakeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reticle")
	EReticleType AimReticleType = EReticleType::Default;	

	bool bIsAiming;
	bool bIsFiring = false;
	float PlayAnimMontage(UAnimMontage* AnimMontage);
	void StopAnimMontage(UAnimMontage* AnimMontage, float BlendOutTime = 0.0f);
	void MakeShot();
private:
	bool bIsReloading = false;	

	void ProcessRecoil(float DeltaTime);
	float GetRecoilTimeInterval() const;
	FTimerHandle RecoilTimer;

	void OnShotTimerElapsed();	

	FTimerHandle ShotTimer;
	FTimerHandle ReloadTimer;
	float GetShotTimerInterval() const;
			
	float AccumulatedRecoilPitch = 0.0f;
	float AccumulatedRecoilYaw = 0.0f;
	int32 AccumulatedShots = 0;
	float RecoilRollbackTime = 0.0f;
	FTimerHandle RecoilRollbackTimer;
	void ProcessRecoilRollback(float DeltaTime);

	float GetCurrentBulletSpreadAngle() const;

	//Index of a FireModeSettings array
	UPROPERTY(SaveGame)
	int32 CurrentFireMode = 0;

	UFUNCTION(Server, Reliable)
	void Server_StartReload();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StartReload();

	void StartReload_Internal();
};
