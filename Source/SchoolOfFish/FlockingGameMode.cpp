// Fill out your copyright notice in the Description page of Project Settings.

#include "FlockingGameMode.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "FishAgent.h"

void AFlockingGameMode::InitGameState()
{
	Super::InitGameState();
	GetWorld()->SpawnActor(agent_BP);
}