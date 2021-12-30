// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameCodeTypes.h"
#include "WeaponBarellComponent.generated.h"

class APickableItem;
USTRUCT(BlueprintType)
struct FDecalInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	UMaterialInterface* DecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	FVector DecalSize = FVector(5.0f, 5.0f, 5.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	float DecalLifeTime = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Decal info")
	float DecalFadeoutTime = 5.0f;
};

USTRUCT(BlueprintType)
struct FShotInfo
{
	GENERATED_BODY()

	FShotInfo() : Location_Mul_10(FVector::ZeroVector), Direction(FVector::ZeroVector) {};

	FShotInfo(FVector Location, FVector Direction) : Location_Mul_10(Location * 10.0f), Direction(Direction) {};

	UPROPERTY()
	FVector_NetQuantize100 Location_Mul_10;
		
	UPROPERTY()
	FVector_NetQuantizeNormal Direction;

	FVector GetLocation() const { return Location_Mul_10 * 0.1f; }

	FVector GetDirection() const { return Direction; }
};

class UNiagaraSystem;
class AGCProjectile;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMECODE_API UWeaponBarellComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	UWeaponBarellComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Shot(FVector ShotStart, FVector ShotDirection, float SpreadAngle, bool bCanProjectileHit = true);

	void SetProjectile(TSubclassOf<AGCProjectile> Projectile_In);
	void SetHitRegistration(EHitRegistrationType HitRegistration_In);
	void InitProjectilePool();

	void SetDamageAmount(float NewDamageAmount);
	void SetProjectileSpeed(float NewSpeed);
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes")
	float FiringRange = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes", meta = (ClampMin = 1, UIMin = 1))
	int32 BulletsPerShot = 1;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Damage")
	float DamageAmount = 20.0f;

	// Damage depending from distance to target (in meters)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Damage")
	UCurveFloat* FalloffDiagram;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Damage")
	TSubclassOf<class UDamageType> DamageTypeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | VFX")
	UNiagaraSystem* MuzzleFlashFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | VFX")
	UNiagaraSystem* TraceFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Decals")
	FDecalInfo DefaultDecalInfo;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Hit registration")
	EHitRegistrationType HitRegistration = EHitRegistrationType::HitScan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Hit registration", meta = (UIMin = 1, ClampMin = 1))
	int32 ProjectilePoolSize = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Hit registration", meta = (EditCondition = "HitRegistration == EHitRegistrationType::Projectile"))
	TSubclassOf<AGCProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Hit registration", meta = (EditCondition = "HitRegistration == EHitRegistrationType::Projectile"))
	bool bReturnToPool = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Hit registration", meta = (EditCondition = "bReturnToPool == false"))
	float ProjectilePenetrationDepth = 10.0f;

	// Projectile length
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Hit registration", meta = (EditCondition = "bReturnToPool == false"))
	float ProjectileHitLocationOffset = -70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barell attributes | Hit registration", meta = (EditCondition = "bReturnToPool == false"))
	float ItemSpawnDistance = 500.0f;

private:
	void ShotInternal(const TArray<FShotInfo>& ShotsInfo, bool bCanProjectileHit = true);

	UFUNCTION(Server, Reliable)
	void Server_Shot(const TArray<FShotInfo>& ShotsInfo);

	UPROPERTY(ReplicatedUsing=OnRep_LastShotsInfo)
	TArray<FShotInfo> LastShotsInfo;

	UPROPERTY(Replicated)
	TArray<AGCProjectile*> ProjectilePool;

	UPROPERTY(Replicated)
	TArray<APickableItem*> PickableProjectilePool;

	UPROPERTY(Replicated)
	int32 CurrentPickableProjectileIndex;

	UPROPERTY(Replicated)
	int32 CurrentProjectileIndex;

	UFUNCTION(Server, Reliable)
	void Server_SpawnPickable(const FVector& Location, const FRotator& Rotation);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SpawnPickable(const FVector& Location, const FRotator& Rotation);

	void SpawnPickableInternal(const FVector& Location, const FRotator& Rotation);

	UFUNCTION()
	void OnPickableAmmoPicked(int32 Index);

	UFUNCTION(Server, Reliable)
	void Server_ReturnPickableToPool(int32 Index);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ReturnPickableToPool(int32 Index);

	void ReturnPickableToPool_Internal(int32 Index);

	UFUNCTION()
	void OnRep_LastShotsInfo();

	FVector GetBulletSpreadOffset(float Angle, FRotator ShotRotation) const;
	bool HitScan(FVector ShotStart, OUT FVector& ShotEnd, FVector ShotDirection);
	void LaunchProjectile(const FVector& LaunchStart, const FVector& LaunchDirection, bool bCanProjectileHit = true);

	APawn* GetOwningPawn() const;
	AController* GetController() const;

	UFUNCTION()
	void ProcessHit(const FHitResult& HitResult, const FVector& Direction);

	UFUNCTION()
	void ProcessProjectileHit(AGCProjectile* Projectile, const FHitResult& HitResult, const FVector& Direction);

	const FVector ProjectilePoolLocation = FVector(0.0f, 0.0f, -100.0f);

	FVector ShotStartLocation = FVector::ZeroVector;
	FVector ShotEndLocation = FVector::ZeroVector;
};
