#pragma once

#include <rttr/type>

#include "Asset.h"

#include <EtCore/Helper/Hash.h>


//---------------------------------
// AssetDatabase
//
// Container for all assets and package descriptors
//
struct AssetDatabase final
{
public:
	// Definitions
	//---------------------
	typedef std::vector<I_Asset*> T_AssetList;

	class PackageDescriptor final
	{
	public:
		std::string const& GetName() const { return m_Name; }
		void SetName(std::string const& val);

		std::string const& GetPath() const { return m_Path; }
		void SetPath(std::string const& val) { m_Path = val; }

		T_Hash GetId() const { return m_Id; }
	private:
		// Data
		///////

		// reflected
		std::string m_Name;
		std::string m_Path;

		// derived
		T_Hash m_Id;

		RTTR_ENABLE()
	};

	struct AssetCache final
	{
		std::type_info const& GetType() const;

		T_AssetList cache;

		RTTR_ENABLE()
	};

	// Construct destruct
	//---------------------
	AssetDatabase(bool const ownsAssets = true) : m_OwnsAssets(ownsAssets) {}
	~AssetDatabase();

	// Accessors
	//---------------------
	T_AssetList GetAssetsInPackage(T_Hash const packageId);

	I_Asset* GetAsset(T_Hash const assetId, bool const reportErrors = true) const;
	I_Asset* GetAsset(T_Hash const assetId, std::type_info const& type, bool const reportErrors = true) const; // faster option

	// Functionality
	//---------------------
	void Flush();
	void Merge(AssetDatabase const& other);

	// Data
	////////
	std::vector<PackageDescriptor> packages;
	std::vector<AssetCache> caches;

	RTTR_ENABLE()

private:
	bool m_OwnsAssets = true;
};
