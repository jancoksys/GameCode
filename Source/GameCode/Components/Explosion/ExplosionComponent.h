// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ExplosionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExplosion);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GAMECODE_API UExplosionComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	UPROPERTY(BlueprintAssignable)
	FOnExplosion OnExplosion;

	UFUNCTION(BlueprintCallable)
	void Explode(AController* Controller);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion | Damage")
	float MaxDamage = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion | Damage")
	float MinDamage = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion | Damage")
	float DamageFallof = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion | Damage")
	TSubclassOf<class UDamageType> DamageTypeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion | Radius")
	float InnerRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion | Radius")
	float OuterRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion | VFX")
	UParticleSystem* ExplosionVFX;

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Explode();
};
