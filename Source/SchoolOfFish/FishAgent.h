// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
//#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FishProcessing.h"
#include "FishAgent.generated.h"


struct FishState {
	int32 instanceId;
	FVector position;
	FVector velocity;
	FVector acceleration;
};

UCLASS()
class SCHOOLOFFISH_API AFishAgent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFishAgent();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	void cpuCalculate(FishState **&agents, float DeltaTime, bool isSingleThread);
	bool collisionDetected(const FVector &start, const FVector &end, FHitResult &hitResult);
	void swapFishStatesIndexes();

private:
	// ~
	// General settings to compute flocking behaviour
	int32 m_fishNum;            // total instances of fish
	FVector m_mapSize;          // size of area where fish can flock
	float m_oceanFloorZ;		// Z coordinate of ocean floor
	float m_spawningRange;		// spawning range in persents of map size
	float m_maxVelocity;        // maximum velocity of fish
	float m_maxAcceleration;    // maximum acceleration of fish
	float m_radiusCohesion;     // Cohesion radius. The radius inside which the fish will tend to inside the circle (approach) 
	float m_radiusSeparation;   // Separation radius. The radius within which the fish will tend to avoid collisions 
	float m_radiusAlignment;    // Alignment radius. The radius inside which the fish will tend to follow in one direction
	// Gain factors for the three types of fish behavior. By default  all three gain factors are equals 1.0f
	float m_kCohesion;          
	float m_kSeparation;
	float m_kAlignment;
	// ~

	// Array of fish states if flocking behaviour calculates on CPU
	FishState** m_fishStates;

	// index of fish states array where stored current states of fish
	int32 m_currentStatesIndex;

	// index of fish states array where stored previous states of fish
	int32 m_previousStatesIndex;

	// Array of fish states if flocking behaviour calculates on GPU
	TArray<State> m_gpuFishStates;

	// Single or multithreaded algorithm. CPU only.
	bool m_isCpuSingleThread;
	
	// This flag indicates where fish state should be calculated. On GPU or on CPU. CPU - by default
	bool m_isGpu;

	// time elapsed from last calculation
	float m_elapsedTime = 0.f;

	// Fish static mesh object
	UStaticMesh *m_staticMesh;

	// Fish instanced static mesh component. This component contains all of the fish instances on the scene
	//UInstancedStaticMeshComponent *m_instancedStaticMeshComponent;
	//Changed to HISM due to editor crashing when ISM is used
	UHierarchicalInstancedStaticMeshComponent *m_instancedStaticMeshComponent;

	// Pointer to the class FishProcessing which uses compute shader plugin to calculate flocking behaviour on GPU
	FishProcessing *m_gpuProcessing;
};
