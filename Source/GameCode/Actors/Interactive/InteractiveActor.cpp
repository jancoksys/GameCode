// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveActor.h"
#include "../../Characters/GCBaseCharacter.h"
#include "Components/CapsuleComponent.h"


void AInteractiveActor::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(InteractionVolume))
	{
		InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AInteractiveActor::OnInteractionVolumeOverlapBegin);
		InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AInteractiveActor::OnInteractionVolumeOverlapEnd);
	}
}

void AInteractiveActor::OnInteractionVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsOverlappingCharacterCapsule(OtherActor, OtherComp))
	{
		return;
	}	
	
	AGCBaseCharacter* BaseCharacter = StaticCast<AGCBaseCharacter*>(OtherActor);
	BaseCharacter->RegisterInteractiveActor(this);
}

bool AInteractiveActor::IsOverlappingCharacterCapsule(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	AGCBaseCharacter* BaseCharacter = Cast<AGCBaseCharacter>(OtherActor);
	if (!IsValid(BaseCharacter))
	{
		return false;
	}
	
	if (Cast<UCapsuleComponent>(OtherComp) != BaseCharacter->GetCapsuleComponent())
	{
		return false;
	}
	return true;
}

void AInteractiveActor::OnInteractionVolumeOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsOverlappingCharacterCapsule(OtherActor, OtherComp))
	{
		return;
	}

	AGCBaseCharacter* BaseCharacter = StaticCast<AGCBaseCharacter*>(OtherActor);		
	BaseCharacter->UnregisterInteractiveActor(this);
}
