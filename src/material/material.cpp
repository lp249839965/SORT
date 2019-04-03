/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.
 
    Copyright (c) 2011-2019 by Cao Jiayin - All rights reserved.
 
    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 
    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#include "material.h"
#include "matmanager.h"
#include "core/log.h"
#include "osl_system.h"

bool Material::BuildShader(){
    m_shader = beginShaderGroup( m_name );
    for( const auto& shader : m_sources )
        build_shader( shader.source , shader.name , shader.name , m_name );
    for( const auto& connection : m_connections )
        connect_shader( connection.source_shader , connection.source_property , connection.target_shader , connection.target_property );

    bool ret = endShaderGroup();
    if( ret )
        slog( INFO , MATERIAL , "Build shader %s successfully." , m_name.c_str() );
    return ret;
}

void Material::Serialize(IStreamBase& stream){
    stream >> m_name;
    auto message = "Parsing Material '" + m_name + "'";
    SORT_PROFILE(message.c_str());

    auto shader_cnt = 0u, connection_cnt = 0u;
    stream >> shader_cnt;
    for (auto i = 0u; i < shader_cnt; ++i) {
        ShaderSource shader_source;
        stream >> shader_source.name >> shader_source.type;

        std::vector<std::string> paramDefaultValues;
        auto parameter_cnt = 0u;
        stream >> parameter_cnt;
        for (auto j = 0u; j < parameter_cnt; ++j) {
            std::string defaultValue;
            stream >> defaultValue;
            paramDefaultValues.push_back(defaultValue);
        }

        // construct the shader source code
        shader_source.source = MatManager::GetSingleton().ConstructShader(shader_source.name, shader_source.type, paramDefaultValues);

        m_sources.push_back( shader_source );
    }

    stream >> connection_cnt;
    for (auto i = 0u; i < connection_cnt; ++i) {
        ShaderConnection connection;
        stream >> connection.source_shader >> connection.source_property;
        stream >> connection.target_shader >> connection.target_property;
        m_connections.push_back( connection );
    }
}

Bsdf* Material::GetBsdf(const class Intersection* intersect) const {
    Bsdf* bsdf = SORT_MALLOC(Bsdf)(intersect);
    execute_shader( bsdf , intersect , m_shader.get() );
    return bsdf;
}