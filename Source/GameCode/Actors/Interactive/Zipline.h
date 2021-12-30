// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Interactive/InteractiveActor.h"
#include "Zipline.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class GAMECODE_API AZipline : public AInteractiveActor
{
	GENERATED_BODY()
	
public:
	AZipline();

	virtual void OnConstruction(const FTransform& Transform) override;

	const FVector GetZiplineCableDirection() const;

	const float GetZiplineLength() const;

	const UStaticMeshComponent* GetZiplineCable() const;

	virtual void BeginPlay() override;	

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* FirstPoleStaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* SecondPoleStaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* CableStaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline Parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float FirstPoleHeight = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline Parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float SecondPoleHeight = 300.0f;		

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline Parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float CableTopOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zipline Parameters", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float InteractionCapsuleRadius = 45.0f;

	class UCapsuleComponent* GetZiplineInteractionCapsule() const;

private:
	FVector ZiplineCableDirection = FVector::ZeroVector;

	FVector FirstPoleTop = FVector::ZeroVector;
	FVector SecondPoleTop = FVector::ZeroVector;
	float ZiplineLength = 0.0f;
};
