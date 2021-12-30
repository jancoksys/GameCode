// Fill out your copyright notice in the Description page of Project Settings.


#include "MeleeWeaponItem.h"
#include "Characters/GCBaseCharacter.h"
#include "Components/Weapon/MeleeHitRegistrator.h"

AMeleeWeaponItem::AMeleeWeaponItem()
{
	EquippedSocketName = SocketCharacterWeapon;
}

void AMeleeWeaponItem::StartAttack(EMeleeAttackTypes AttackType)
{
	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	if (!IsValid(CharacterOwner))
	{
		return;
	}

	HitActors.Empty();
	CurrentAttack = Attacks.Find(AttackType);
	if (CurrentAttack && IsValid(CurrentAttack->AttackMontage))
	{
		UAnimInstance* CharacterAnimInstance = CharacterOwner->GetMesh()->GetAnimInstance();
		if (IsValid(CharacterAnimInstance))
		{
			float Duration = CharacterAnimInstance->Montage_Play(CurrentAttack->AttackMontage, 1.0f, EMontagePlayReturnType::Duration);
			GetWorld()->GetTimerManager().SetTimer(AttackTimer, this, &AMeleeWeaponItem::OnAttackTimerElapsed, Duration, false);
		}
		else
		{
			OnAttackTimerElapsed();
		}
	}

}

void AMeleeWeaponItem::SetIsHitRegistrationEnabled(bool bIsRegistrationEnabled)
{
	HitActors.Empty();
	for (UMeleeHitRegistrator* HitRegistrator : HitRegistrators)
	{
		HitRegistrator->SetIsHitRegistrationEnabled(bIsRegistrationEnabled);
	}
}

void AMeleeWeaponItem::BeginPlay()
{
	Super::BeginPlay();
	GetComponents<UMeleeHitRegistrator>(HitRegistrators);
	for (UMeleeHitRegistrator* HitRegistrator : HitRegistrators)
	{
		HitRegistrator->OnMeleeHitRegistred.AddUFunction(this, FName("ProcessHit"));
	}
}

void AMeleeWeaponItem::OnAttackTimerElapsed()
{
	CurrentAttack = nullptr;
	SetIsHitRegistrationEnabled(false);
}

void AMeleeWeaponItem::ProcessHit(const FHitResult& HitResult, const FVector& HitDirection)
{	
	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor) || HitActors.Contains(HitActor) || CurrentAttack == nullptr)
	{
		return;
	}
	
	FPointDamageEvent DamageEvent;
	DamageEvent.HitInfo = HitResult;
	DamageEvent.ShotDirection = HitDirection;
	DamageEvent.DamageTypeClass = CurrentAttack->DamageTypeClass;

	AGCBaseCharacter* CharacterOwner = GetCharacterOwner();
	AController* Controller = IsValid(CharacterOwner) ? CharacterOwner->GetController<AController>() : nullptr;
	HitActor->TakeDamage(CurrentAttack->DamageAmount, DamageEvent, Controller, GetOwner());
	
	HitActors.Add(HitActor);
}
