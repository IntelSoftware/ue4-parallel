// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "FlockingGameMode.generated.h"


UCLASS()
class SCHOOLOFFISH_API AFlockingGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void InitGameState() override;

	UPROPERTY(EditAnywhere, Category = FlockingConfig)
		TSubclassOf<class AFishAgent> agent_BP;
};
