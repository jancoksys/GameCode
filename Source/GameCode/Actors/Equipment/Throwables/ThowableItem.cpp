// Fill out your copyright notice in the Description page of Project Settings.


#include "ThowableItem.h"
#include "Characters/GCBaseCharacter.h"
#include "Actors/Projectiles/GCProjectile.h"
#include "GameCodeTypes.h"
#include "Components/Weapon/WeaponBarellComponent.h"
#include "Net/UnrealNetwork.h"

AThowableItem::AThowableItem()
{
	SetReplicates(true);
}

void AThowableItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AThowableItem, ThrowableItemsPool);
	DOREPLIFETIME(AThowableItem, CurrentThrowableIndex);
}

void AThowableItem::BeginPlay()
{
	Super::BeginPlay();	
	
	if (GetOwner()->GetLocalRole() < ROLE_Authority || !IsValid(ProjectileClass))
	{
		return;
	}

	ThrowableItemsPool.Reserve(ThrowablesPoolSize);

	for (int32 i = 0; i < ThrowablesPoolSize; ++i)
	{
		AGCProjectile* Projectile = GetWorld()->SpawnActor<AGCProjectile>(ProjectileClass, ThrowablesPoolLocation, FRotator::ZeroRotator);
		Projectile->SetOwner(GetOwner());
		Projectile->SetProjectileActive(false);
		ThrowableItemsPool.Add(Projectile);
	}
}

void AThowableItem::Throw()
{
	if (ItemAmount == 0)
	{
		return;
	}	
	
	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	AController* Controller = CharacterOwner->GetController<AController>();
	if (!IsValid(Controller))
	{
		return;
	}

	FVector PlayerViewPoint;
	FRotator PlayerViewRotation;	

	Controller->GetPlayerViewPoint(PlayerViewPoint, PlayerViewRotation);
	FTransform PlayerViewTransform(PlayerViewRotation, PlayerViewPoint);

	FVector ViewDirection = PlayerViewRotation.RotateVector(FVector::ForwardVector);

	FVector ViewUpVector = PlayerViewRotation.RotateVector(FVector::UpVector);
	FVector LaunchDirection = ViewDirection + FMath::Tan(FMath::DegreesToRadians(ThrowAngle)) * ViewUpVector;

	FVector ThrowableSocketLocation = CharacterOwner->GetMesh()->GetSocketLocation(SocketCharacterThrowable);
	FVector SocketInViewSpace = PlayerViewTransform.InverseTransformPosition(ThrowableSocketLocation);

	FVector StartLocation = PlayerViewPoint + ViewDirection * SocketInViewSpace.X;

	LaunchProjectile(StartLocation, LaunchDirection.GetSafeNormal());
}

int32 AThowableItem::GetItemAmount() const
{
	return ItemAmount;
}

void AThowableItem::SetItemAmount(int32 NewItemAmount)
{
	ItemAmount = NewItemAmount;
	if (OnItemAmountChanged.IsBound())
	{
		OnItemAmountChanged.Broadcast(ItemAmount);
	}
}

void AThowableItem::Server_LaunchProjectile_Implementation(const FVector& Start, const FVector& Direction)
{
	LaunchProjectile(Start, Direction);
}

void AThowableItem::LaunchProjectile(const FVector& LaunchStart, const FVector& LaunchDirection)
{
	AGCProjectile* Projectile = ThrowableItemsPool[CurrentThrowableIndex];	
	Projectile->SetActorLocation(LaunchStart);
	Projectile->SetActorRotation(LaunchDirection.ToOrientationRotator());
	Projectile->SetProjectileActive(true);
	Projectile->LaunchProjectile(LaunchDirection.GetSafeNormal());
	++CurrentThrowableIndex;
	if (CurrentThrowableIndex == ThrowableItemsPool.Num())
	{
		CurrentThrowableIndex = 0;
	}
}
