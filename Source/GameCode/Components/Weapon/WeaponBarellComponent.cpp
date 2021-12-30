// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponBarellComponent.h"
#include "GameCodeTypes.h"
#include "DrawDebugHelpers.h"
#include "Subsystems/DebugSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Actors/Interactive/Pickables/PickableAmmo.h"
#include "Curves/CurveFloat.h"
#include "Components/DecalComponent.h"
#include "Actors/Projectiles/GCProjectile.h"
#include "Net/UnrealNetwork.h"
#include "Utils/GCDataTableUtils.h"


UWeaponBarellComponent::UWeaponBarellComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWeaponBarellComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams RepParams;
	RepParams.Condition = COND_SimulatedOnly;
	RepParams.RepNotifyCondition = REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS(UWeaponBarellComponent, LastShotsInfo, RepParams);
	DOREPLIFETIME(UWeaponBarellComponent, ProjectilePool);
	DOREPLIFETIME(UWeaponBarellComponent, CurrentProjectileIndex);
	DOREPLIFETIME(UWeaponBarellComponent, PickableProjectilePool);
	DOREPLIFETIME(UWeaponBarellComponent, CurrentPickableProjectileIndex);
}

void UWeaponBarellComponent::Shot(FVector ShotStart, FVector ShotDirection, float SpreadAngle, bool bCanProjectileHit)
{	
	TArray<FShotInfo> ShotsInfo;
	for (int i = 0; i < BulletsPerShot; i++)
	{
		ShotDirection += GetBulletSpreadOffset(FMath::RandRange(0.0f, SpreadAngle), ShotDirection.ToOrientationRotator());
		ShotDirection.GetSafeNormal();
		ShotsInfo.Emplace(ShotStart, ShotDirection);
	}

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_Shot(ShotsInfo);
	}
	ShotInternal(ShotsInfo, bCanProjectileHit);
}

void UWeaponBarellComponent::SetProjectile(TSubclassOf<AGCProjectile> Projectile_In)
{
	ProjectileClass = Projectile_In;
}

void UWeaponBarellComponent::SetHitRegistration(EHitRegistrationType HitRegistration_In)
{
	HitRegistration = HitRegistration_In;
}

void UWeaponBarellComponent::InitProjectilePool()
{
	if (GetOwnerRole() < ROLE_Authority || !IsValid(ProjectileClass))
	{
		return;
	}
	
	ProjectilePool.Reserve(ProjectilePoolSize);
	PickableProjectilePool.Reserve(ProjectilePoolSize);
	for (int32 i = 0; i < ProjectilePoolSize; ++i)
	{
		AGCProjectile* Projectile = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, ProjectilePoolLocation, FRotator::ZeroRotator);
		Projectile->SetOwner(GetOwningPawn());
		Projectile->SetProjectileActive(false);
		ProjectilePool.Add(Projectile);
		FAmmoTableRow* TableRow = GCDataTableUtils::FindAmmoTableData(Projectile->GetDataTableID());
		if (TableRow != nullptr)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = GetOwner();

			TSubclassOf<APickableItem> ClassToSpawn = TableRow->PickableActorClass;			
			APickableItem* PickableProjectile = GetWorld()->SpawnActor<APickableItem>(ClassToSpawn, ProjectilePoolLocation, FRotator::ZeroRotator, SpawnParameters);			
			PickableProjectile->SetActorLocation(ProjectilePoolLocation);
			PickableProjectile->SetActorRotation(FRotator::ZeroRotator);
			PickableProjectile->SetPickablePoolIndex(i);
			PickableProjectilePool.Add(PickableProjectile);
		}
	}
}

void UWeaponBarellComponent::SetDamageAmount(float NewDamageAmount)
{
	DamageAmount = NewDamageAmount;
}

void UWeaponBarellComponent::SetProjectileSpeed(float NewSpeed)
{
	for(AGCProjectile*& Projectile : ProjectilePool)
	{
		Projectile->SetInitialSpeed(NewSpeed);
	}
}

void UWeaponBarellComponent::BeginPlay()
{
	Super::BeginPlay();
	InitProjectilePool();
}

bool UWeaponBarellComponent::HitScan(FVector ShotStart, OUT FVector& ShotEnd, FVector ShotDirection)
{
	FHitResult ShotResult;
	bool bHasHit = GetWorld()->LineTraceSingleByChannel(ShotResult, ShotStart, ShotEnd, ECC_Bullet);
	if (bHasHit)
	{
		ShotEnd = ShotResult.ImpactPoint;
		ProcessHit(ShotResult, ShotDirection);		
	}		
	return bHasHit;
}

void UWeaponBarellComponent::LaunchProjectile(const FVector& LaunchStart, const FVector& LaunchDirection, bool bCanProjectileHit)
{
	AGCProjectile* Projectile = ProjectilePool[CurrentProjectileIndex];	
	Projectile->SetActorLocation(LaunchStart);
	Projectile->SetActorRotation(LaunchDirection.ToOrientationRotator());
	Projectile->SetProjectileActive(true);
	Projectile->OnProjectileHit.AddDynamic(this, &UWeaponBarellComponent::ProcessProjectileHit);		
	Projectile->LaunchProjectile(LaunchDirection.GetSafeNormal());
	++CurrentProjectileIndex;
	if (CurrentProjectileIndex == ProjectilePool.Num())
	{
		CurrentProjectileIndex = 0;
	}
}

APawn* UWeaponBarellComponent::GetOwningPawn() const
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!IsValid(PawnOwner))
	{
		PawnOwner = Cast<APawn>(GetOwner()->GetOwner());
	}
	return PawnOwner;
}

AController* UWeaponBarellComponent::GetController() const
{
	APawn* PawnOwner = GetOwningPawn();
	return IsValid(PawnOwner) ? PawnOwner->GetController() : nullptr;
}

void UWeaponBarellComponent::ProcessHit(const FHitResult& HitResult, const FVector& Direction)
{
	AActor* HitActor = HitResult.GetActor();

	if (GetOwner()->HasAuthority() && IsValid(HitActor))
	{
		float CorrectedDamageAmount = DamageAmount;
		if (IsValid(FalloffDiagram))
		{
			float ShotDistance = (ShotEndLocation - ShotStartLocation).Size();
			CorrectedDamageAmount = FalloffDiagram->GetFloatValue(ShotDistance * 0.01f) * DamageAmount;
		}
		FPointDamageEvent DamageEvent;
		DamageEvent.HitInfo = HitResult;
		DamageEvent.ShotDirection = Direction;
		DamageEvent.DamageTypeClass = DamageTypeClass;
		HitActor->TakeDamage(CorrectedDamageAmount, DamageEvent, GetController(), GetOwner());
	}
	
	UDecalComponent* DecalComponent = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), DefaultDecalInfo.DecalMaterial, DefaultDecalInfo.DecalSize, HitResult.ImpactPoint, HitResult.ImpactNormal.ToOrientationRotator());
	if (IsValid(DecalComponent))
	{
		DecalComponent->SetFadeScreenSize(0.001f);
		DecalComponent->SetFadeOut(DefaultDecalInfo.DecalLifeTime, DefaultDecalInfo.DecalFadeoutTime);
	}
}

void UWeaponBarellComponent::ProcessProjectileHit(AGCProjectile* Projectile, const FHitResult& HitResult, const FVector& Direction)
{
	if (bReturnToPool)
	{
		Projectile->SetProjectileActive(false);
		Projectile->SetActorLocation(ProjectilePoolLocation);
		Projectile->SetActorRotation(FRotator::ZeroRotator);
		Projectile->OnProjectileHit.RemoveAll(this);
	}
	else
	{
		FAmmoTableRow* TableRow = GCDataTableUtils::FindAmmoTableData(Projectile->GetDataTableID());
		FVector SpawnLocation = Projectile->GetActorLocation() + Direction * ProjectilePenetrationDepth;
		if (TableRow != nullptr)
		{
			FRotator SpawnRotator = Projectile->GetActorRotation();
			SpawnRotator.Yaw -= 90.0f;
			SpawnLocation += Direction * ProjectileHitLocationOffset;

			if (GetOwner()->GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
			{
				Server_SpawnPickable(SpawnLocation, SpawnRotator);
				SpawnPickableInternal(SpawnLocation, SpawnRotator);
			}
			else if (GetOwner()->GetOwner()->GetLocalRole() == ROLE_Authority)
			{
				Server_SpawnPickable(SpawnLocation, SpawnRotator);
			}

			Projectile->SetProjectileActive(false);
			Projectile->SetActorLocation(ProjectilePoolLocation);
			Projectile->SetActorRotation(FRotator::ZeroRotator);
			Projectile->OnProjectileHit.RemoveAll(this);
		}
		else
		{
			Projectile->SetActorLocation(SpawnLocation);
		}		
	}	
	
	ProcessHit(HitResult, Direction);
}

void UWeaponBarellComponent::ShotInternal(const TArray<FShotInfo>& ShotsInfo, bool bCanProjectileHit)
{
	if (GetOwner()->HasAuthority())
	{
		LastShotsInfo = ShotsInfo;
	}

	FVector MuzzleLocation = GetComponentLocation();
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashFX, MuzzleLocation, GetComponentRotation());
	for (const FShotInfo& ShotInfo : ShotsInfo)
	{
		FVector ShotStart = ShotInfo.GetLocation();
		FVector ShotDirection = ShotInfo.GetDirection();

		FVector ShotEnd = ShotStart + FiringRange * ShotDirection;
		ShotStartLocation = ShotStart;
		ShotEndLocation = ShotEnd;

#if ENABLE_DRAW_DEBUG
		UDebugSubsystem* DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
		bool bIsDebugEnabled = DebugSubsystem->IsCategoryEnabled(DebugCategoryRangeWeapon);
#else 
		bool bIsDebugEnabled = false;
#endif

		if (HitRegistration == EHitRegistrationType::HitScan)
		{
			bool bHasHit = HitScan(ShotStart, ShotEnd, ShotDirection);
			if (bIsDebugEnabled && bHasHit)
			{
				DrawDebugSphere(GetWorld(), ShotEnd, 10.0f, 24, FColor::Red, false, 1.0f);
			}
		}
		else
		{
			LaunchProjectile(ShotStart, ShotDirection, bCanProjectileHit);
		}

		UNiagaraComponent* TraceFXComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TraceFX, MuzzleLocation, GetComponentRotation());
		if (IsValid(TraceFXComponent))
		{
			TraceFXComponent->SetVectorParameter(FXParamTraceEnd, ShotEnd);
		}		

		if (bIsDebugEnabled)
		{
			DrawDebugLine(GetWorld(), MuzzleLocation, ShotEnd, FColor::Red, false, 1.0f, 0, 3.0f);
		}
	}
}

void UWeaponBarellComponent::Server_Shot_Implementation(const TArray<FShotInfo>& ShotsInfo)
{
	ShotInternal(ShotsInfo);
}

void UWeaponBarellComponent::Server_SpawnPickable_Implementation(const FVector& Location, const FRotator& Rotation)
{
	Multicast_SpawnPickable(Location, Rotation);
}

void UWeaponBarellComponent::Multicast_SpawnPickable_Implementation(const FVector& Location, const FRotator& Rotation)
{
	SpawnPickableInternal(Location, Rotation);
}

void UWeaponBarellComponent::SpawnPickableInternal(const FVector& Location, const FRotator& Rotation)
{
	APickableItem* PickableProjectile = PickableProjectilePool[CurrentPickableProjectileIndex];
	PickableProjectile->SetActorRotation(Rotation);
	PickableProjectile->SetActorLocation(Location);	
	PickableProjectile->OnPickedUpEvent.AddUFunction(this, FName("OnPickableAmmoPicked"));
	++CurrentPickableProjectileIndex;
	if (CurrentPickableProjectileIndex == PickableProjectilePool.Num())
	{
		CurrentPickableProjectileIndex = 0;
	}
}

void UWeaponBarellComponent::OnPickableAmmoPicked(int32 Index)
{
	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_ReturnPickableToPool(Index);
		ReturnPickableToPool_Internal(Index);
	}
	else if (GetOwner()->GetLocalRole() == ROLE_Authority)
	{		
		Server_ReturnPickableToPool(Index);
	}
}

void UWeaponBarellComponent::Multicast_ReturnPickableToPool_Implementation(int32 Index)
{
	ReturnPickableToPool_Internal(Index);	
}

void UWeaponBarellComponent::Server_ReturnPickableToPool_Implementation(int32 Index)
{
	Multicast_ReturnPickableToPool(Index);
	ReturnPickableToPool_Internal(Index);
}

void UWeaponBarellComponent::ReturnPickableToPool_Internal(int32 Index)
{
	PickableProjectilePool[Index]->SetActorLocation(ProjectilePoolLocation);
	PickableProjectilePool[Index]->SetActorRotation(FRotator::ZeroRotator);
	PickableProjectilePool[Index]->OnPickedUpEvent.RemoveAll(this);
}

void UWeaponBarellComponent::OnRep_LastShotsInfo()
{
	ShotInternal(LastShotsInfo);
}

FVector UWeaponBarellComponent::GetBulletSpreadOffset(float Angle, FRotator ShotRotation) const
{
	float SpreadSize = FMath::Tan(Angle);
	float RotationAngle = FMath::RandRange(0.0f, 2 * PI);

	float SpreadY = FMath::Cos(RotationAngle);
	float SpreadZ = FMath::Sin(RotationAngle);

	FVector Result = (ShotRotation.RotateVector(FVector::UpVector) * SpreadZ
		+ ShotRotation.RotateVector(FVector::RightVector) * SpreadY) * SpreadSize;

	return Result;
}