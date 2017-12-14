// Fill out your copyright notice in the Description page of Project Settings.

#include "FishAgent.h"
#include "FlockingGameMode.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ConstructorHelpers.h"
#include "ParallelFor.h"
#include "Event.h"
#include "ScopeLock.h"
#include "Engine/StaticMesh.h"

FVector checkMapRange(const FVector &map, const FVector &currentPosition, const FVector &currentVelocity)
{
	FVector newVelocity = currentVelocity;
	if (currentPosition.X > map.X || currentPosition.X < -map.X) {
		newVelocity.X *= -1.f;
	}

	if (currentPosition.Y > map.Y || currentPosition.Y < -map.Y) {
		newVelocity.Y *= -1.f;
	}

	if (currentPosition.Z > map.Z || currentPosition.Z < -3000.f) {
		newVelocity.Z *= -1.f;
	}
	return newVelocity;
}

int32 getCommandLineArgIntValue(FString key, int32 defaultValue)
{
	int32 value;
	if (FParse::Value(FCommandLine::Get(), *key, value)) {
		return value;
	}
	return defaultValue;
}

float getCommandLineArgFloatValue(FString key, float defaultValue)
{
	float value;
	if (FParse::Value(FCommandLine::Get(), *key, value)) {
		return value;
	}
	return defaultValue;
}

FString getCommandLineArgStringValue(FString key, FString defaultValue)
{
	FString value;
	if (FParse::Value(FCommandLine::Get(), *key, value)) {
		return value.Replace(TEXT("="), TEXT("")).Replace(TEXT("\""), TEXT(""));
	}
	return defaultValue;
}

AFishAgent::AFishAgent()
{
	// Available calcModes:
	// CPU_SINGLE - CPU single-threaded calculation of flocking behaviour
	// CPU_MULTI - CPU multi-threaded calculation of flocking behaviour
	// GPU_MULTI - GPU multi-threaded calculation of flocking behaviour

	FString mode = getCommandLineArgStringValue("calcMode", "CPU_SINGLE");
	if ("CPU_SINGLE" == mode) {
		m_isCpuSingleThread = true;
		m_isGpu = false;
	}
	else if ("CPU_MULTI" == mode) {
		m_isCpuSingleThread = false;
		m_isGpu = false;
	}
	else if ("GPU_MULTI" == mode) {
		m_isGpu = true;
	}
	else {
		m_isCpuSingleThread = true;
		m_isGpu = false;
	}

	m_fishNum = getCommandLineArgIntValue("agentCount", 1000);
	m_maxVelocity = getCommandLineArgFloatValue("maxVelocity", 2500.f);
	m_maxAcceleration = getCommandLineArgFloatValue("maxAcceleration", 1500.f);
	m_radiusCohesion = getCommandLineArgFloatValue("radiusCohesion", 1000.f);
	m_radiusSeparation = getCommandLineArgFloatValue("radiusSeparation", 120.f);
	m_radiusAlignment = getCommandLineArgFloatValue("radiusAlignment", 240.f);
	m_kCohesion = getCommandLineArgFloatValue("kCohesion", 100.f);
	m_kSeparation = getCommandLineArgFloatValue("kSeparation", 1.f);
	m_kAlignment = getCommandLineArgFloatValue("kAlignment", 20.f);
	m_mapSize.X = getCommandLineArgFloatValue("mapSizeX", 20000.f);
	m_mapSize.Y = getCommandLineArgFloatValue("mapSizeY", 20000.f);
	m_mapSize.Z = getCommandLineArgFloatValue("mapSizeZ", 0.f);
	m_oceanFloorZ = getCommandLineArgFloatValue("oceanFloorZ", -3000.f);
	m_spawningRange = getCommandLineArgFloatValue("spawningRange", 0.9f);

	m_currentStatesIndex = 0;
	m_previousStatesIndex = 1;

	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshOb(TEXT("StaticMesh'/Game/Fish/SM_fish'"));
	m_staticMesh = StaticMeshOb.Object;
}

void AFishAgent::OnConstruction(const FTransform& Transform)
{
	m_instancedStaticMeshComponent = NewObject<UInstancedStaticMeshComponent>(this);
	m_instancedStaticMeshComponent->RegisterComponent();
	m_instancedStaticMeshComponent->SetStaticMesh(m_staticMesh);
	m_instancedStaticMeshComponent->SetFlags(RF_Transactional);
	this->AddInstanceComponent(m_instancedStaticMeshComponent);

	if (!m_isGpu) {
		m_fishStates = new FishState*[m_fishNum];
	}

	for (int i = 0; i < m_fishNum; i++) {
		FVector randomPos(FMath::RandRange(-m_spawningRange * m_mapSize.X, m_mapSize.X * m_spawningRange),
			FMath::RandRange(-m_spawningRange * m_mapSize.Y, m_mapSize.Y * m_spawningRange),
			FMath::RandRange(m_spawningRange * m_oceanFloorZ, m_mapSize.Z * m_spawningRange));
		FRotator randRotator(0, FMath::RandRange(0, 360), 0);

		FTransform t;
		t.SetLocation(randomPos);
		t.SetRotation(randRotator.Quaternion());
		t.SetScale3D(FVector(10, 10, 10));

		int32 instanceId = m_instancedStaticMeshComponent->AddInstance(t);

		if (m_isGpu) {
			State gpustate;
			gpustate.instanceId = instanceId;
			gpustate.position[0] = randomPos.X; gpustate.position[1] = randomPos.Y; gpustate.position[2] = randomPos.Z;
			gpustate.velocity[0] = randRotator.Vector().X * 10000; gpustate.velocity[1] = randRotator.Vector().Y * 10000; gpustate.velocity[2] = randRotator.Vector().Z * 10000;
			gpustate.acceleration[0] = 0.f; gpustate.acceleration[1] = 0.f; gpustate.acceleration[2] = 0.f;
			m_gpuFishStates.Add(gpustate);
		} else {
			FishState state;
			FishState stateCopy;
			m_fishStates[i] = new FishState[2];
			stateCopy.instanceId = state.instanceId = instanceId;;
			stateCopy.position = state.position = randomPos;
			stateCopy.velocity = state.velocity = randRotator.Vector() * 10000;
			stateCopy.acceleration = state.acceleration = FVector::ZeroVector;
			m_fishStates[i][m_currentStatesIndex] = state;
			m_fishStates[i][m_previousStatesIndex] = stateCopy;
		}
	}
}

void AFishAgent::BeginPlay()
{
	Super::BeginPlay();

	if (m_isGpu) {
		m_gpuProcessing = new FishProcessing(m_fishNum,
			m_radiusCohesion, m_radiusSeparation, m_radiusAlignment,
			m_mapSize.X, m_mapSize.Y, m_mapSize.Z,
			m_kCohesion, m_kSeparation, m_kAlignment,
			m_maxAcceleration, m_maxVelocity,
			GetWorld()->Scene->GetFeatureLevel()
		);
	}
}

void AFishAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (m_isGpu) {
		if (m_elapsedTime > 0.016f) {
			m_gpuProcessing->calculate(m_gpuFishStates, DeltaTime);
			TArray<State> previousGpuFishStates = m_gpuFishStates;
			m_gpuFishStates = m_gpuProcessing->getStates();

			if (previousGpuFishStates.Num() != 0 && previousGpuFishStates.Num() == m_gpuFishStates.Num()) {
				for (int i = 0; i < previousGpuFishStates.Num(); i++) {
					FHitResult hit(ForceInit);
					if (collisionDetected(FVector(previousGpuFishStates[i].position[0], previousGpuFishStates[i].position[1], previousGpuFishStates[i].position[2]),
						FVector(m_gpuFishStates[i].position[0], m_gpuFishStates[i].position[1], m_gpuFishStates[i].position[2]), hit)) {
						m_gpuFishStates[i].position[0] -= m_gpuFishStates[i].velocity[0] * DeltaTime;
						m_gpuFishStates[i].position[1] -= m_gpuFishStates[i].velocity[1] * DeltaTime;
						m_gpuFishStates[i].position[2] -= m_gpuFishStates[i].velocity[2] * DeltaTime;
						m_gpuFishStates[i].velocity[0] *= -1.0;
						m_gpuFishStates[i].velocity[1] *= -1.0;
						m_gpuFishStates[i].velocity[2] *= -1.0;
						m_gpuFishStates[i].position[0] += m_gpuFishStates[i].velocity[0] * DeltaTime;
						m_gpuFishStates[i].position[1] += m_gpuFishStates[i].velocity[1] * DeltaTime;
						m_gpuFishStates[i].position[2] += m_gpuFishStates[i].velocity[2] * DeltaTime;
					}
				}
			}

			for (int i = 0; i < m_fishNum; i++) {
				FTransform transform;
				m_instancedStaticMeshComponent->GetInstanceTransform(m_gpuFishStates[i].instanceId, transform);
				transform.SetLocation(FVector(m_gpuFishStates[i].position[0], m_gpuFishStates[i].position[1], m_gpuFishStates[i].position[2]));
				transform.SetRotation(FRotationMatrix::MakeFromX(FVector(m_gpuFishStates[i].velocity[0], m_gpuFishStates[i].velocity[1], m_gpuFishStates[i].velocity[2])).Rotator().Add(0.f, -90.f, 0.f).Quaternion());
				m_instancedStaticMeshComponent->UpdateInstanceTransform(m_gpuFishStates[i].instanceId, transform, true, false);
			}
			m_instancedStaticMeshComponent->ReleasePerInstanceRenderData();
			m_instancedStaticMeshComponent->MarkRenderStateDirty();
			m_elapsedTime = 0.f;
		} else {
			m_elapsedTime += DeltaTime;
		}
	} else {
		if (m_elapsedTime > 0.016f) {
			cpuCalculate(m_fishStates, DeltaTime, m_isCpuSingleThread);
			m_elapsedTime = 0.f;
		} else {
			m_elapsedTime += DeltaTime;
		}
	}
}

void AFishAgent::cpuCalculate(FishState **&agents, float DeltaTime, bool isSingleThread)
{
	float kCoh = m_kCohesion, kSep = m_kSeparation, kAlign = m_kAlignment;
	float rCohesion = m_radiusCohesion, rSeparation = m_radiusSeparation, rAlignment = m_radiusAlignment;
	float maxAccel = m_maxAcceleration;
	float maxVel = m_maxVelocity;
	FVector mapSz = m_mapSize;
	static const int32 cnt = m_fishNum;

	int currentStatesIndex = m_currentStatesIndex;
	int previousStatesIndex = m_previousStatesIndex;
	
	ParallelFor(cnt, [&agents, currentStatesIndex, previousStatesIndex, kCoh, kSep, kAlign, rCohesion, rSeparation, rAlignment, maxAccel, maxVel, mapSz, DeltaTime, isSingleThread](int32 fishNum) {
		FVector cohesion(FVector::ZeroVector), separation(FVector::ZeroVector), alignment(FVector::ZeroVector);
		int32 cohesionCnt = 0, separationCnt = 0, alignmentCnt = 0;
		for (int i = 0; i < cnt; i++) {
			if (i != fishNum) {
				float distance = FVector::Distance(agents[i][previousStatesIndex].position, agents[fishNum][previousStatesIndex].position);
				if (distance > 0) {
					if (distance < rCohesion) {
						cohesion += agents[i][previousStatesIndex].position;
						cohesionCnt++;
					}
					if (distance < rSeparation) {
						separation += agents[i][previousStatesIndex].position - agents[fishNum][previousStatesIndex].position;
						separationCnt++;
					}
					if (distance < rAlignment) {
						alignment += agents[i][previousStatesIndex].velocity;
						alignmentCnt++;
					}
				}
			}
		}

		if (cohesionCnt != 0) {
			cohesion /= cohesionCnt;
			cohesion -= agents[fishNum][previousStatesIndex].position;
			cohesion.Normalize();
		}

		if (separationCnt != 0) {
			separation /= separationCnt;
			separation *= -1.f;
			separation.Normalize();
		}

		if (alignmentCnt != 0) {
			alignment /= alignmentCnt;
			alignment.Normalize();
		}

		agents[fishNum][currentStatesIndex].acceleration = (cohesion * kCoh + separation * kSep + alignment * kAlign).GetClampedToMaxSize(maxAccel);
		agents[fishNum][currentStatesIndex].acceleration.Z = 0;

		agents[fishNum][currentStatesIndex].velocity += agents[fishNum][currentStatesIndex].acceleration * DeltaTime;
		agents[fishNum][currentStatesIndex].velocity  = agents[fishNum][currentStatesIndex].velocity.GetClampedToMaxSize(maxVel);
		agents[fishNum][currentStatesIndex].position += agents[fishNum][currentStatesIndex].velocity * DeltaTime;
		agents[fishNum][currentStatesIndex].velocity = checkMapRange(mapSz, agents[fishNum][currentStatesIndex].position, agents[fishNum][currentStatesIndex].velocity);
	}, isSingleThread);

	for (int i = 0; i < cnt; i++) {
		FHitResult hit(ForceInit);
		if (collisionDetected(agents[i][previousStatesIndex].position, agents[i][currentStatesIndex].position, hit)) {
			agents[i][currentStatesIndex].position -= agents[i][currentStatesIndex].velocity * DeltaTime;
			agents[i][currentStatesIndex].velocity *= -1.0;
			agents[i][currentStatesIndex].position += agents[i][currentStatesIndex].velocity * DeltaTime;
		}
	}

	for (int i = 0; i < cnt; i++) {
		FTransform transform;
		m_instancedStaticMeshComponent->GetInstanceTransform(agents[i][0].instanceId, transform);
		transform.SetLocation(agents[i][0].position);
		FVector direction = agents[i][0].velocity;
		direction.Normalize();
		transform.SetRotation(FRotationMatrix::MakeFromX(direction).Rotator().Add(0.f, -90.f, 0.f).Quaternion());
		m_instancedStaticMeshComponent->UpdateInstanceTransform(agents[i][0].instanceId, transform, false, false);
	}

	m_instancedStaticMeshComponent->ReleasePerInstanceRenderData();
	m_instancedStaticMeshComponent->MarkRenderStateDirty();

	swapFishStatesIndexes();
}

bool AFishAgent::collisionDetected(const FVector &start, const FVector &end, FHitResult &hitResult)
{
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bTraceAsyncScene = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	return GetWorld()->SweepSingleByChannel(hitResult, start, end, FQuat(), ECC_WorldStatic, FCollisionShape::MakeSphere(200), RV_TraceParams);
}

void AFishAgent::swapFishStatesIndexes()
{
	int32 tmp = m_currentStatesIndex;
	m_currentStatesIndex = m_previousStatesIndex;
	m_previousStatesIndex = tmp;
}