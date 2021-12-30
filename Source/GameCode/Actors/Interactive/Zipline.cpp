// Fill out your copyright notice in the Description page of Project Settings.


#include "Zipline.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameCodeTypes.h"

AZipline::AZipline()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ZiplineRoot"));
	
	FirstPoleStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPole"));
	FirstPoleStaticMeshComponent->SetupAttachment(RootComponent);

	SecondPoleStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SecondPole"));
	SecondPoleStaticMeshComponent->SetupAttachment(RootComponent);

	CableStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cable"));
	CableStaticMeshComponent->SetupAttachment(RootComponent);

	InteractionVolume = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetCollisionProfileName(CollisionProfilePawnInteractionVolume);
	InteractionVolume->SetGenerateOverlapEvents(true);
}

void AZipline::OnConstruction(const FTransform& Transform)
{	
	FVector	ZiplinePolesDistance = FirstPoleStaticMeshComponent->GetRelativeLocation() - SecondPoleStaticMeshComponent->GetRelativeLocation();

	FVector FirstPoleTopPoint(FirstPoleStaticMeshComponent->GetRelativeLocation().X, FirstPoleStaticMeshComponent->GetRelativeLocation().Y, FirstPoleStaticMeshComponent->GetRelativeLocation().Z + (FirstPoleHeight / 2.0f));
	FVector SecondPoleTopPoint(SecondPoleStaticMeshComponent->GetRelativeLocation().X, SecondPoleStaticMeshComponent->GetRelativeLocation().Y, SecondPoleStaticMeshComponent->GetRelativeLocation().Z + (SecondPoleHeight / 2.0f));

	float PolesTopPointsDistance = FVector::Distance(FirstPoleTopPoint, SecondPoleTopPoint);
	float ZiplineTopPointsHeightDifference = FirstPoleTopPoint.Z - SecondPoleTopPoint.Z;

	FVector ZiplineMiddlePoint = (FirstPoleTopPoint + SecondPoleTopPoint) * 0.5f;

	UStaticMesh* FirstPoleMesh = FirstPoleStaticMeshComponent->GetStaticMesh();
	if (IsValid(FirstPoleMesh))
	{
		float MeshHeight = FirstPoleMesh->GetBoundingBox().GetSize().Z;
		if (!FMath::IsNearlyZero(MeshHeight))
		{
			FirstPoleStaticMeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, FirstPoleHeight / MeshHeight));
		}
	}

	UStaticMesh* SecondPoleMesh = SecondPoleStaticMeshComponent->GetStaticMesh();
	if (IsValid(SecondPoleMesh))
	{
		float MeshHeight = SecondPoleMesh->GetBoundingBox().GetSize().Z;
		if (!FMath::IsNearlyZero(MeshHeight))
		{
			SecondPoleStaticMeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, SecondPoleHeight / MeshHeight));
		}
	}

	UStaticMesh* CableMesh = CableStaticMeshComponent->GetStaticMesh();
	if (IsValid(CableMesh))
	{
		float MeshLength = CableMesh->GetBoundingBox().GetSize().X;
		if (!FMath::IsNearlyZero(MeshLength))
		{				
			CableStaticMeshComponent->SetRelativeScale3D(FVector(PolesTopPointsDistance / MeshLength, 1.0f, 1.0f));
			
			FRotator CableRotation(FMath::RadiansToDegrees(FMath::Atan2(ZiplineTopPointsHeightDifference, ZiplinePolesDistance.X)), 90.0f - FMath::RadiansToDegrees(FMath::Atan2(ZiplinePolesDistance.X, ZiplinePolesDistance.Y)), 0.0f);
			CableStaticMeshComponent->SetRelativeRotation(CableRotation);

			CableStaticMeshComponent->SetRelativeLocation(FVector(ZiplineMiddlePoint.X, ZiplineMiddlePoint.Y, ZiplineMiddlePoint.Z - CableTopOffset));
		}
	}	
	
	GetZiplineInteractionCapsule()->SetRelativeLocation(FVector(ZiplineMiddlePoint.X, ZiplineMiddlePoint.Y, ZiplineMiddlePoint.Z - CableTopOffset));
	GetZiplineInteractionCapsule()->SetRelativeRotation(FRotator(90.0f + FMath::RadiansToDegrees(FMath::Atan2(ZiplineTopPointsHeightDifference, ZiplinePolesDistance.X)), 90.0f - FMath::RadiansToDegrees(FMath::Atan2(ZiplinePolesDistance.X, ZiplinePolesDistance.Y)), 0.0f));
	GetZiplineInteractionCapsule()->SetCapsuleSize(InteractionCapsuleRadius, PolesTopPointsDistance * 0.5f + InteractionCapsuleRadius);
}

const FVector AZipline::GetZiplineCableDirection() const
{
	if (FirstPoleTop.Z >= SecondPoleTop.Z)
	{
		return SecondPoleTop - FirstPoleTop;
	}
	else
	{
		return FirstPoleTop - SecondPoleTop;
	}
}

const float AZipline::GetZiplineLength() const
{
	return ZiplineLength;
}

const UStaticMeshComponent* AZipline::GetZiplineCable() const
{
	return CableStaticMeshComponent;
}

void AZipline::BeginPlay()
{
	Super::BeginPlay();
	FirstPoleTop = FVector(FirstPoleStaticMeshComponent->GetRelativeLocation().X, FirstPoleStaticMeshComponent->GetRelativeLocation().Y, FirstPoleStaticMeshComponent->GetRelativeLocation().Z + (FirstPoleHeight / 2.0f));
	SecondPoleTop = FVector(SecondPoleStaticMeshComponent->GetRelativeLocation().X, SecondPoleStaticMeshComponent->GetRelativeLocation().Y, SecondPoleStaticMeshComponent->GetRelativeLocation().Z + (SecondPoleHeight / 2.0f));	 
	ZiplineLength = FVector::Distance(FirstPoleTop, SecondPoleTop);
}

UCapsuleComponent* AZipline::GetZiplineInteractionCapsule() const
{
	return StaticCast<UCapsuleComponent*>(InteractionVolume);
}