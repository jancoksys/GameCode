// Fill out your copyright notice in the Description page of Project Settings.


#include "HostSessionWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GCGameInstance.h"

void UHostSessionWidget::CreateSession()
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld());
	check(GameInstance->IsA<UGCGameInstance>());
	UGCGameInstance* GCGameInstance = StaticCast<UGCGameInstance*>(GameInstance);

	GCGameInstance->LaunchLobby(4, ServerName, bIsLan);
}
