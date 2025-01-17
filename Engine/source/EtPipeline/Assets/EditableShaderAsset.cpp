#include "stdafx.h"
#include "EditableShaderAsset.h"

#include <EtCore/Content/AssetPointer.h>
#include <EtCore/Content/AssetStub.h>
#include <EtCore/FileSystem/FileUtil.h>

#include <EtRendering/GlobalRenderingSystems/GlobalRenderingSystems.h>


namespace et {
namespace pl {


//=======================
// Editable Shader Asset
//=======================


// reflection
RTTR_REGISTRATION
{
	BEGIN_REGISTER_CLASS(EditableShaderAsset, "editable shader asset")
		.property("use geometry", &EditableShaderAsset::m_UseGeometry)
		.property("use fragment", &EditableShaderAsset::m_UseFragment)
	END_REGISTER_CLASS_POLYMORPHIC(EditableShaderAsset, EditorAssetBase);
}
DEFINE_FORCED_LINKING(EditableShaderAsset) // force the asset class to be linked as it is only used in reflection


//-------------------------------------
// EditableShaderAsset::LoadFromMemory
//
bool EditableShaderAsset::LoadFromMemory(std::vector<uint8> const& data)
{
	// Extract the shader text from binary data
	//------------------------
	std::string shaderContent = core::FileUtil::AsText(data);
	if (shaderContent.size() == 0)
	{
		LOG("EditableShaderAsset::LoadFromMemory > Empty shader file!", core::LogLevel::Warning);
		return false;
	}

	// Precompile
	//--------------------
	std::string vertSource;
	std::string geoSource;
	std::string fragSource;
	if (!Precompile(shaderContent, vertSource, geoSource, fragSource))
	{
		return false;
	}

	// Compile
	//------------------
	render::I_GraphicsContextApi* const api = render::ContextHolder::GetRenderContext();
	render::T_ShaderLoc const shaderProgram = render::ShaderAsset::LinkShader(vertSource, geoSource, fragSource, api);

	// Create shader data
	SetData(new render::ShaderData(shaderProgram));
	render::ShaderData* const shaderData = GetData();

	// Extract uniform info
	//------------------
	api->SetShader(shaderData);
	render::ShaderAsset::InitUniforms(shaderData);
	render::ShaderAsset::GetAttributes(shaderProgram, shaderData->m_Attributes);

	// all done
	return true;
}

//-------------------------------------------------
// EditableShaderAsset::SetupRuntimeAssetsInternal
//
void EditableShaderAsset::SetupRuntimeAssetsInternal()
{
	render::ShaderAsset* const mainAsset = new render::ShaderAsset(*static_cast<render::ShaderAsset*>(m_Asset));
	ET_ASSERT(core::FileUtil::ExtractExtension(mainAsset->GetName()) == render::ShaderAsset::s_MainExtension);

	m_RuntimeAssets.emplace_back(mainAsset, true);

	std::vector<core::HashString> references;

	if (m_UseFragment || m_UseGeometry)
	{
		std::string const baseName = core::FileUtil::RemoveExtension(mainAsset->GetName());
		std::string const& path = mainAsset->GetPath();
		core::HashString const package = mainAsset->GetPackageId();

		if (m_UseGeometry)
		{
			core::StubAsset* const geoAsset = new core::StubAsset();
			geoAsset->SetName(baseName + "." + render::ShaderAsset::s_GeoExtension);
			geoAsset->SetPath(path);
			geoAsset->SetPackageId(package);

			m_RuntimeAssets.emplace_back(geoAsset, true);
			references.push_back(geoAsset->GetId());
		}

		if (m_UseFragment)
		{
			core::StubAsset* const fragAsset = new core::StubAsset();
			fragAsset->SetName(baseName + "." + render::ShaderAsset::s_FragExtension);
			fragAsset->SetPath(path);
			fragAsset->SetPackageId(package);

			m_RuntimeAssets.emplace_back(fragAsset, true);
			references.push_back(fragAsset->GetId());
		}
	}

	mainAsset->SetReferenceIds(references);
}

//-------------------------------------
// EditableShaderAsset::LoadFromMemory
//
bool EditableShaderAsset::GenerateInternal(BuildConfiguration const& buildConfig, std::string const& dbPath)
{
	ET_UNUSED(buildConfig);
	ET_UNUSED(dbPath);

	// Extract the shader text from binary data
	//------------------------
	std::string shaderContent = core::FileUtil::AsText(m_Asset->GetLoadData());
	if (shaderContent.size() == 0)
	{
		LOG("EditableShaderAsset::GenerateInternal > Empty shader file!", core::LogLevel::Warning);
		return false;
	}

	// Precompile
	//--------------------
	std::string vertSource;
	std::string geoSource;
	std::string fragSource;
	if (!Precompile(shaderContent, vertSource, geoSource, fragSource))
	{
		return false;
	}

	// Write Data
	//--------------------
	for (RuntimeAssetData& data : m_RuntimeAssets)
	{
		data.m_HasGeneratedData = true;

		std::string const ext = core::FileUtil::ExtractExtension(data.m_Asset->GetName());
		if (ext == render::ShaderAsset::s_MainExtension)
		{
			data.m_GeneratedData = core::FileUtil::FromText(vertSource);
		}
		else if (ext == render::ShaderAsset::s_GeoExtension)
		{
			data.m_GeneratedData = core::FileUtil::FromText(geoSource);
		}
		else if (ext == render::ShaderAsset::s_FragExtension)
		{
			data.m_GeneratedData = core::FileUtil::FromText(fragSource);
		}
		else 
		{
			ET_ASSERT(false, "unexpected file extension");
			data.m_HasGeneratedData = false;
		}
	}

	return true;
}

//---------------------------------
// EditableShaderAsset::Precompile
//
// Precompile a .glsl file into multiple shader strings that can be compiled on their own
//
bool EditableShaderAsset::Precompile(std::string &shaderContent, std::string &vertSource, std::string &geoSource, std::string &fragSource)
{
	enum ParseState {
		INIT,
		VERT,
		GEO,
		FRAG
	} state = ParseState::INIT;

	std::string extractedLine;
	while (core::FileUtil::ParseLine(shaderContent, extractedLine))
	{
		//Includes
		if (extractedLine.find("#include") != std::string::npos)
		{
			if (!(ReplaceInclude(extractedLine)))
			{
				LOG(std::string("ShaderAsset::Precompile > Replacing include at '") + extractedLine + "' failed!", core::LogLevel::Warning);
				return false;
			}
		}

		//Precompile types
		switch (state)
		{
		case INIT:
			if (extractedLine.find("<VERTEX>") != std::string::npos)
			{
				state = ParseState::VERT;
			}
			if (extractedLine.find("<GEOMETRY>") != std::string::npos)
			{
				ET_ASSERT(m_UseGeometry);
				state = ParseState::GEO;
			}
			if (extractedLine.find("<FRAGMENT>") != std::string::npos)
			{
				state = ParseState::FRAG;
			}
			break;
		case VERT:
			if (extractedLine.find("</VERTEX>") != std::string::npos)
			{
				state = ParseState::INIT;
				break;
			}
			vertSource += extractedLine;
			vertSource += "\n";
			break;
		case GEO:
			if (extractedLine.find("</GEOMETRY>") != std::string::npos)
			{
				state = ParseState::INIT;
				break;
			}
			geoSource += extractedLine;
			geoSource += "\n";
			break;
		case FRAG:
			if (extractedLine.find("</FRAGMENT>") != std::string::npos)
			{
				state = ParseState::INIT;
				break;
			}
			else if (extractedLine.find("#disable") != std::string::npos)
			{
				ET_ASSERT(!m_UseFragment);
				state = ParseState::INIT;
				break;
			}
			fragSource += extractedLine;
			fragSource += "\n";
			break;
		}
	}

	return true;
}

//---------------------------------
// EditableShaderAsset::ReplaceInclude
//
// Include another shader asset inline
//
bool EditableShaderAsset::ReplaceInclude(std::string &line)
{
	// Get the asset ID
	uint32 firstQ = (uint32)line.find("\"");
	uint32 lastQ = (uint32)line.rfind("\"");
	if ((firstQ == std::string::npos) ||
		(lastQ == std::string::npos) ||
		lastQ <= firstQ)
	{
		LOG(std::string("ShaderAsset::ReplaceInclude > Replacing include line '") + line + "' failed", core::LogLevel::Warning);
		return false;
	}
	firstQ++;
	std::string path = line.substr(firstQ, lastQ - firstQ);
	core::HashString const assetId(path.c_str());

	// Get the stub asset data
	std::vector<core::I_Asset::Reference> const& refs = m_Asset->GetReferences();
	auto const foundRefIt = std::find_if(refs.cbegin(), refs.cend(), [assetId](core::I_Asset::Reference const& reference)
		{
			ET_ASSERT(reference.GetAsset() != nullptr);
			return reference.GetAsset()->GetAsset()->GetId() == assetId;
		});

	if (foundRefIt == refs.cend())
	{
		ET_ASSERT(false, "Asset at path '%s' not found in references!", path.c_str());
		return false;
	}

	I_AssetPtr const* const rawAssetPtr = foundRefIt->GetAsset();
	ET_ASSERT(rawAssetPtr->GetType() == rttr::type::get<core::StubData>(), "Asset reference found at path %s is not of type StubData", path);
	AssetPtr<core::StubData> stubPtr = *static_cast<AssetPtr<core::StubData> const*>(rawAssetPtr);

	// extract the shader string
	std::string shaderContent(stubPtr->GetText(), stubPtr->GetLength());
	if (shaderContent.size() == 0)
	{
		LOG(std::string("ShaderAsset::ReplaceInclude > Shader string extracted from stub data at'") + path + "' was empty!", core::LogLevel::Warning);
		return false;
	}

	// replace the original line with the included shader
	line = "";
	std::string extractedLine;
	while (core::FileUtil::ParseLine(shaderContent, extractedLine))
	{
		//Includes
		if (extractedLine.find("#include") != std::string::npos)
		{
			if (!(ReplaceInclude(extractedLine)))
			{
				LOG(std::string("ShaderAsset::ReplaceInclude > Replacing include at '") + extractedLine + "' failed!", core::LogLevel::Warning);
				return false;
			}
		}

		line += extractedLine + "\n";
	}

	// we're done
	return true;
}


} // namespace pl
} // namespace et
