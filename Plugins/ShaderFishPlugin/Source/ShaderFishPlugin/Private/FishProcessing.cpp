// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FishProcessing.h"
#include "CoreUObject.h"
#include "Engine.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 128 

FishProcessing::FishProcessing(int32 fishCount, float radiusCohesion, float radiusSeparation, float radiusAlignment,
	float mapRangeX, float mapRangeY, float mapRangeZ, float kCohesion, float kSeparation, float kAlignment,
	float maxAcceleration, float maxVelocity, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	m_featureLevel = ShaderFeatureLevel;
	m_constantParameters.fishCount = fishCount;
	m_constantParameters.radiusCohesion = radiusCohesion;
	m_constantParameters.radiusSeparation = radiusSeparation;
	m_constantParameters.radiusAlignment = radiusAlignment;
	m_constantParameters.mapRangeX = mapRangeX;
	m_constantParameters.mapRangeY = mapRangeY;
	m_constantParameters.mapRangeZ = mapRangeZ;
	m_constantParameters.kCohesion = kCohesion;
	m_constantParameters.kSeparation = kSeparation;
	m_constantParameters.kAlignment = kAlignment;
	m_constantParameters.maxAcceleration = maxAcceleration;
	m_constantParameters.maxVelocity = maxVelocity;
	m_constantParameters.calculationsPerThread = 1;

	m_variableParameters = FVariableParameters();

	for (int i = 0; i < 2 * m_constantParameters.fishCount; i++) {
		m_states.Add(State());
	}

	m_threadNumGroupCount = (m_constantParameters.fishCount % (NUM_THREADS_PER_GROUP_DIMENSION * m_constantParameters.calculationsPerThread) == 0 ?
		m_constantParameters.fishCount / (NUM_THREADS_PER_GROUP_DIMENSION * m_constantParameters.calculationsPerThread) :
		m_constantParameters.fishCount / (NUM_THREADS_PER_GROUP_DIMENSION * m_constantParameters.calculationsPerThread) + 1);
	m_threadNumGroupCount = m_threadNumGroupCount == 0 ? 1 : m_threadNumGroupCount;
}

FishProcessing::~FishProcessing()
{
}

void FishProcessing::calculate(const TArray<State> &currentStates, float deltaTime)
{
	ExecuteComputeShader(currentStates, deltaTime);
	ReleaseResourcesFence.BeginFence();
	ReleaseResourcesFence.Wait();
}

void FishProcessing::ExecuteComputeShader(const TArray<State> &currentStates, float DeltaTime)
{
	m_variableParameters.DeltaTime = DeltaTime;
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER( FComputeShaderRunner,
		FishProcessing*, processing, this,
		TArray<State>&, result, m_states,
		const TArray<State>&, states, currentStates,
		{ 
			processing->ExecuteInRenderThread(states, result); 
		} 
	);
}

void FishProcessing::ExecuteInRenderThread(const TArray<State> &currentStates, TArray<State> &result)
{
	check(IsInRenderingThread());

	TResourceArray<State> data;
	for (int i = 0; i < m_constantParameters.fishCount; i++) {
		data.Add(currentStates[i]);
	}
	for (int i = 0; i < m_constantParameters.fishCount; i++) {
		data.Add(currentStates[i]);
	}

	FRHIResourceCreateInfo resource;
	resource.ResourceArray = &data;
	FStructuredBufferRHIRef buffer = RHICreateStructuredBuffer(sizeof(State), sizeof(State) * 2 * m_constantParameters.fishCount, BUF_UnorderedAccess | 0, resource);
	FUnorderedAccessViewRHIRef uav = RHICreateUnorderedAccessView(buffer, false, false);

	FRHICommandListImmediate& commandList = GRHICommandList.GetImmediateCommandList();
	TShaderMapRef<FShaderFishPluginModule> shader(GetGlobalShaderMap(m_featureLevel));
	commandList.SetComputeShader(shader->GetComputeShader());
	shader->setShaderData(commandList, uav);
	shader->setUniformBuffers(commandList, m_constantParameters, m_variableParameters);
	DispatchComputeShader(commandList, *shader, 1, m_threadNumGroupCount, 1);
	shader->cleanupShaderData(commandList);	

	char* shaderData = (char*)commandList.LockStructuredBuffer(buffer, 0, sizeof(State) * 2 * m_constantParameters.fishCount, EResourceLockMode::RLM_ReadOnly);
	State* p = (State*)shaderData;
	for (int32 Row = 0; Row < m_constantParameters.fishCount; ++Row) {
		result[Row] = *p;
		p++;
	}
	commandList.UnlockStructuredBuffer(buffer);
}