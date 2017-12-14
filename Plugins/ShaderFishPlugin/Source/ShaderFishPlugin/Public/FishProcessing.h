// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "ShaderFishPlugin.h"
#include "RenderCommandFence.h"

struct State {
	int32 instanceId = 0;
	float position[3] = { 0, 0, 0 };
	float velocity[3] = { 0, 0, 0 };
	float acceleration[3] = { 0, 0, 0 };
};

class SHADERFISHPLUGIN_API FishProcessing
{
public:
	FishProcessing(int32 fishCount, float radiusCohesion, float radiusSeparation, float radiusAlignment,
		float mapRangeX, float mapRangeY, float mapRangeZ, float kCohesion, float kSeparation, float kAlignment, 
		float maxAcceleration, float maxVelocity, ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FishProcessing();

	void calculate(const TArray<State> &currentStates, float deltaTime);
	TArray<State> getStates() { return m_states; }

protected:
	void ExecuteComputeShader(const TArray<State> &currentStates, float DeltaTime);
	void ExecuteInRenderThread(const TArray<State> &currentStates, TArray<State> &result);

private:
	FConstantParameters m_constantParameters;
	FVariableParameters m_variableParameters;
	ERHIFeatureLevel::Type m_featureLevel;
	TArray<State> m_states;
	int32 m_threadNumGroupCount;
	FRenderCommandFence ReleaseResourcesFence;
};