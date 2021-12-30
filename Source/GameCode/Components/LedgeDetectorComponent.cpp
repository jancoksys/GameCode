#include "LedgeDetectorComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "../GameCodeTypes.h"
#include <DrawDebugHelpers.h>
#include "../Utils/GCTraceUtils.h"
#include "../Characters/GCBaseCharacter.h"
#include "../GCGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "../Subsystems/DebugSubsystem.h"

void ULedgeDetectorComponent::BeginPlay()
{
	Super::BeginPlay();
	checkf(GetOwner()->IsA<ACharacter>(), TEXT("ULedgeDetectorComponent::BeginPlay() only a character can use ULedgeDetectorComponent"));
	CachedCharacterOwner = StaticCast<ACharacter*>(GetOwner());

}

bool ULedgeDetectorComponent::DetectLedge(OUT FLedgeDescription& LedgeDescription)
{
	if (!CachedCharacterOwner.IsValid())
	{
		return false;
	}
	UCapsuleComponent* CapsuleComponent = GetDefault<AGCBaseCharacter>()->GetCapsuleComponent();	
	
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	QueryParams.AddIgnoredActor(GetOwner());	

#if ENABLE_DRAW_DEBUG
	UDebugSubsystem* DebugSubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDebugSubsystem>();
	bool bIsDebugEnabled = DebugSubsystem->IsCategoryEnabled(DebugCategoryLedgeDetection);
#else 
	bool bIsDebugEnabled = false;
#endif

	float DrawTime = 2.0f;

	float BottomZOffset = 2.0f;
	FVector CharacterBottom = CachedCharacterOwner->GetActorLocation() - (CachedCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - BottomZOffset) * FVector::UpVector;
	
	//1. forward check
	float ForwardCheckCapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	float ForwardCheckCapsuleHalfHeight = (MaximumLedgeHeight - MinimumLedgeHeight) * 0.5f;
	
	FHitResult ForwardCheckHitResult;	
	FVector ForwardStartLocation = CharacterBottom + (MinimumLedgeHeight + ForwardCheckCapsuleHalfHeight) * FVector::UpVector;
	FVector ForwardEndLocation = ForwardStartLocation + CachedCharacterOwner->GetActorForwardVector() * ForwardCheckDistance;
		

	if (!GCTraceUtils::SweepCapsuleSingleByChannel(GetWorld(), ForwardCheckHitResult, ForwardStartLocation, ForwardEndLocation, ForwardCheckCapsuleRadius, ForwardCheckCapsuleHalfHeight, FQuat::Identity, ECC_Climbing, QueryParams, FCollisionResponseParams::DefaultResponseParam, bIsDebugEnabled, DrawTime))
	{
		return false;
	}	

	//2. Downward check
	FHitResult DownwardCheckHitResult;
	
	float DownwardSphereCheckRadius = CapsuleComponent->GetScaledCapsuleRadius();	
	
	float DownwardCheckDepthOffset = 10.0f;
	FVector DownwardStartLocation = ForwardCheckHitResult.ImpactPoint - ForwardCheckHitResult.ImpactNormal * DownwardCheckDepthOffset;
	DownwardStartLocation.Z = CharacterBottom.Z + MaximumLedgeHeight + DownwardSphereCheckRadius;
	FVector DownwardEndLocation(DownwardStartLocation.X, DownwardStartLocation.Y, CharacterBottom.Z);

	if (!GCTraceUtils::SweepSphereSingleByChannel(GetWorld(), DownwardCheckHitResult, DownwardStartLocation, DownwardEndLocation, DownwardSphereCheckRadius, ECC_Climbing, QueryParams, FCollisionResponseParams::DefaultResponseParam, bIsDebugEnabled, DrawTime))
	{
		return false;
	}	
	
	//3. overlap check
	float OverlapCapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
	float OverlapCapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();	
	float OverlapCapsuleFloorOffset = 2.0f;
	FVector OverlapLocation = DownwardCheckHitResult.ImpactPoint + (OverlapCapsuleHalfHeight + OverlapCapsuleFloorOffset) * FVector::UpVector;
	
	if (GCTraceUtils::OverlapCapsuleBlockingByProfile(GetWorld(), OverlapLocation, OverlapCapsuleRadius, OverlapCapsuleHalfHeight, FQuat::Identity, CollisionProfilePawn, QueryParams, bIsDebugEnabled, DrawTime))
	{
		return false;
	}	
	
	LedgeDescription.Location = OverlapLocation;
	LedgeDescription.Rotation = (ForwardCheckHitResult.ImpactNormal * FVector(-1.0f, -1.0f, 0.0f)).ToOrientationRotator();
	LedgeDescription.LedgeNormal = ForwardCheckHitResult.ImpactNormal;
	LedgeDescription.LedgeHeight = DownwardCheckHitResult.Location.Z - CharacterBottom.Z;
	LedgeDescription.LedgePrimitiveComponent = DownwardCheckHitResult.GetComponent();
	
	return true;
}

