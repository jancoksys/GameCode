// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GCProjectile.generated.h"

class AGCProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectileHit, AGCProjectile*, Projectile, const FHitResult&, Hit, const FVector&, Direction);

UCLASS()
class GAMECODE_API AGCProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGCProjectile();

	UFUNCTION(BlueprintCallable)
	void LaunchProjectile(FVector Direction);

	UPROPERTY(BlueprintAssignable)
	FOnProjectileHit OnProjectileHit;
	
	UFUNCTION(BlueprintNativeEvent)
	void SetProjectileActive(bool bIsProjectileActive);

	void SetInitialSpeed(float NewSpeed);
	FName GetDataTableID() const;
protected:
	virtual void OnProjectileLaunched();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickable")
	FName DataTableID = NAME_None;

	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
