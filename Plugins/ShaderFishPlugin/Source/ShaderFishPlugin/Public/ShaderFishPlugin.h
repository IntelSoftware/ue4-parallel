// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

BEGIN_UNIFORM_BUFFER_STRUCT(FConstantParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, fishCount)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, radiusCohesion)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, radiusSeparation)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, radiusAlignment)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, mapRangeX)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, mapRangeY)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, mapRangeZ)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, kCohesion)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, kSeparation)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, kAlignment)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, maxAcceleration)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, maxVelocity)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, calculationsPerThread)
END_UNIFORM_BUFFER_STRUCT(FConstantParameters)

BEGIN_UNIFORM_BUFFER_STRUCT(FVariableParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, DeltaTime)
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
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment);

	virtual bool Serialize(FArchive& Ar) override { bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar); Ar << m_shaderResource; return bShaderHasOutdatedParams; }

	void setShaderData(FRHICommandList& commandList, FUnorderedAccessViewRHIRef uav);
	void setUniformBuffers(FRHICommandList& commandList, FConstantParameters& constants, FVariableParameters& variables);
	void cleanupShaderData(FRHICommandList& commandList);

private:
	FShaderResourceParameter m_shaderResource;
};