// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ShaderFishPlugin.h"
#include "CoreUObject.h"
#include "Engine.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FConstantParameters, TEXT("constants"))
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FVariableParameters, TEXT("variables"))

FShaderFishPluginModule::FShaderFishPluginModule(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
	: FGlobalShader(Initializer)
{
	m_shaderResource.Bind(Initializer.ParameterMap, TEXT("data"));
}

void FShaderFishPluginModule::ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

void FShaderFishPluginModule::setShaderData(FRHICommandList& commandList, FUnorderedAccessViewRHIRef uav)
{
	if (m_shaderResource.IsBound())
		commandList.SetUAVParameter(GetComputeShader(), m_shaderResource.GetBaseIndex(), uav);
}

void FShaderFishPluginModule::setUniformBuffers(FRHICommandList& commandList, FConstantParameters& constants, FVariableParameters& variables)
{
	SetUniformBufferParameter(commandList, GetComputeShader(), GetUniformBufferParameter<FConstantParameters>(),
		FConstantParametersRef::CreateUniformBufferImmediate(constants, UniformBuffer_SingleDraw));
	SetUniformBufferParameter(commandList, GetComputeShader(), GetUniformBufferParameter<FVariableParameters>(),
		FVariableParametersRef::CreateUniformBufferImmediate(variables, UniformBuffer_SingleDraw));
}

void FShaderFishPluginModule::cleanupShaderData(FRHICommandList& commandList)
{
	if (m_shaderResource.IsBound())
		commandList.SetUAVParameter(GetComputeShader(), m_shaderResource.GetBaseIndex(), FUnorderedAccessViewRHIRef());
}

IMPLEMENT_SHADER_TYPE(, FShaderFishPluginModule, TEXT("ComputeFishShader"),       TEXT("VS_test"),       SF_Compute);

IMPLEMENT_MODULE(FDefaultModuleImpl, ShaderFishPlugin)