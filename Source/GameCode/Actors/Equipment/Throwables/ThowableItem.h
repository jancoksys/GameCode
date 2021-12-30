// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Equipment/EquipableItem.h"
#include "ThowableItem.generated.h"

class AGCProjectile;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemAmountChanged, int32);

UCLASS(Blueprintable)
class GAMECODE_API AThowableItem : public AEquipableItem
{
	GENERATED_BODY()
	
public:
	AThowableItem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

	void Throw();

	FOnItemAmountChanged OnItemAmountChanged;

	int32 GetItemAmount() const;
	void SetItemAmount(int32 NewItemAmount);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables")
	TSubclassOf<AGCProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throwables", meta = (UIMin = -90.0f, UIMax = 90.0f, ClampMin = -90.0f, ClampMax = 90.0f))
	float ThrowAngle = 0.0f;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables")
	EAmunitionType AmmoType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwables", meta = (UIMin = 1, ClampMin = 1))
	int32 ThrowablesPoolSize = 10;

private:
	int32 ItemAmount = 0;

	UPROPERTY(Replicated)
	TArray<AGCProjectile*> ThrowableItemsPool;

	const FVector ThrowablesPoolLocation = FVector(0.0f, 0.0f, -100.0f);

	UPROPERTY(Replicated)
	int32 CurrentThrowableIndex;

	UFUNCTION(Server, Reliable)
	void Server_LaunchProjectile(const FVector& Start, const FVector& Direction);

	void LaunchProjectile(const FVector& LaunchStart, const FVector& LaunchDirection);
};
