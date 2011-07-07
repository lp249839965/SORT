/*
 * filename :	matmanager.cpp
 *
 * programmer :	Cao Jiayin
 */

// include the header
#include "sort.h"
#include "matmanager.h"
#include "material/material.h"
#include "utility/path.h"
#include "thirdparty/tinyxml/tinyxml.h"
#include "logmanager.h"
#include "material/matte.h"

// instance the singleton with tex manager
DEFINE_SINGLETON(MatManager);

// default constructor
MatManager::MatManager()
{
	// register materials
	_registerMaterials();
}
// destructor
MatManager::~MatManager()
{
	_clearMatPool();
	_unregisterMaterials();
}

// clear the material pool
void MatManager::_clearMatPool()
{
	map< string , Material* >::iterator it = m_matPool.begin();
	while( it != m_matPool.end() )
	{
		delete it->second;
		it++;
	}
	m_matPool.clear();
}

// find specific material
Material* MatManager::FindMaterial( const string& mat_name ) const
{
	map< string , Material* >::const_iterator it = m_matPool.find( mat_name );
	if( it != m_matPool.end() )
		return it->second;
	return 0;
}

// parse material file and add the materials into the manager
unsigned MatManager::ParseMatFile( const string& str )
{
	// load the xml file
	TiXmlDocument doc( GetFullPath(str).c_str() );
	doc.LoadFile();

	// if there is error , return false
	if( doc.Error() )
	{
		LOG_WARNING<<doc.ErrorDesc()<<ENDL;
		LOG_WARNING<<"Material file load failed."<<ENDL;
		return false;
	}

	// get the root of xml
	TiXmlNode*	root = doc.RootElement();

	// parse materials
	TiXmlElement* material = root->FirstChildElement( "Material" );
	while( material )
	{
		// parse the material
		string name = material->Attribute( "name" );
		string type = material->Attribute( "type" );

		// check if there is a material with the specific name, crash if there is
		if( FindMaterial( name ) != 0 )
			LOG_ERROR<<"A material named "<<name<<" already exists in material system."<<CRASH;

		// create specific material
		Material* mat = _createMaterial( type );

		if( mat )
		{
			// push the material
			m_matPool.insert( make_pair( name , mat ) );
		}

		// parse the next material
		material = material->NextSiblingElement( "Material" );
	}

	return 0;
}

// get material number
unsigned MatManager::GetMatCount() const
{
	return m_matPool.size();
}

// register all of the materials
void MatManager::_registerMaterials()
{
	// register all of the materials
	m_matType.insert( make_pair( "Matte" , new Matte() ) );
}

// clear registered types
void MatManager::_unregisterMaterials()
{
	map< string , Material* >::iterator it = m_matType.begin();
	if( it != m_matType.end() )
		delete it->second;
	m_matType.clear();
}

// create material
Material* MatManager::_createMaterial( const string& str )
{
	map< string , Material* >::const_iterator it = m_matType.find( str );
	if( it != m_matType.end() )
		return it->second->CreateInstance();

	// crash
	LOG_WARNING<<"There is no material named "<<str<<"."<<ENDL;

	return 0;
}