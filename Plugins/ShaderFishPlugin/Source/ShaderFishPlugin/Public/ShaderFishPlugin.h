// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

BEGIN_UNIFORM_BUFFER_STRUCT(FConstantParameters, )
UNIFORM_MEMBER(int, fishCount)
UNIFORM_MEMBER(float, radiusCohesion)
UNIFORM_MEMBER(float, radiusSeparation)
UNIFORM_MEMBER(float, radiusAlignment)
UNIFORM_MEMBER(float, mapRangeX)
UNIFORM_MEMBER(float, mapRangeY)
UNIFORM_MEMBER(float, mapRangeZ)
UNIFORM_MEMBER(float, kCohesion)
UNIFORM_MEMBER(float, kSeparation)
UNIFORM_MEMBER(float, kAlignment)
UNIFORM_MEMBER(float, maxAcceleration)
UNIFORM_MEMBER(float, maxVelocity)
UNIFORM_MEMBER(int, calculationsPerThread)
END_UNIFORM_BUFFER_STRUCT(FConstantParameters)

BEGIN_UNIFORM_BUFFER_STRUCT(FVariableParameters, )
UNIFORM_MEMBER(float, DeltaTime)
END_UNIFORM_BUFFER_STRUCT(FVariableParameters)

typedef TUniformBufferRef<FConstantParameters> FConstantParametersRef;
typedef TUniformBufferRef<FVariableParameters> FVariableParametersRef;

class FShaderFishPluginModule : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FShaderFishPluginModule, Global);
public:
	FShaderFishPluginModule() {}
	explicit FShaderFishPluginModule(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5); }
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	virtual bool Serialize(FArchive& Ar) override { bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar); Ar << m_shaderResource; return bShaderHasOutdatedParams; }

	void setShaderData(FRHICommandList& commandList, FUnorderedAccessViewRHIRef uav);
	void setUniformBuffers(FRHICommandList& commandList, FConstantParameters& constants, FVariableParameters& variables);
	void cleanupShaderData(FRHICommandList& commandList);

	//Required Function in UE4.20
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters);

private:
	FShaderResourceParameter m_shaderResource;
};