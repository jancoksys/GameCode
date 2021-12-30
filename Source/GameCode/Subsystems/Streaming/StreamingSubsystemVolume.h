// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreamingSubsystemVolume.generated.h"

class UStreamingSubsystem;
class UBoxComponent;

UCLASS()
class GAMECODE_API AStreamingSubsystemVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	AStreamingSubsystemVolume();

	const TSet<FString>& GetLevelsToLoad() const;
	const TSet<FString>& GetLevelsToUnload() const;

	void HandleCharacterOverlapBegin(ACharacter* Character);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Collision")
	UBoxComponent* CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	TSet<FString> LevelsToLoad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	TSet<FString> LevelsToUnload;

private:
	TWeakObjectPtr<UStreamingSubsystem> StreamingSubsystem;
	TWeakObjectPtr<ACharacter> OverlappedCharacter;
};
