#include "directionalLight.h"

#include "glm/gtx/string_cast.hpp"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "view/view.h"
#include "shaders/directionalLight_glsl.h"

namespace Tangram {

std::string DirectionalLight::s_classBlock;
std::string DirectionalLight::s_typeName = "DirectionalLight";

DirectionalLight::DirectionalLight(const std::string& _name, bool _dynamic) :
    Light(_name, _dynamic),
    m_direction(1.0,0.0,0.0) {

    m_type = LightType::directional;
}

DirectionalLight::~DirectionalLight() {}

void DirectionalLight::setDirection(const glm::vec3 &_dir) {
    m_direction = glm::normalize(_dir);
}

std::unique_ptr<LightUniforms> DirectionalLight::injectOnProgram(ShaderProgram& _shader) {
    injectSourceBlocks(_shader);

    if (!m_dynamic) { return nullptr; }

    return std::make_unique<Uniforms>(_shader, getUniformName());
}

void DirectionalLight::setupProgram(const View& _view, LightUniforms& _uniforms) {

    glm::vec3 direction = m_direction;
    if (m_origin == LightOrigin::world) {
        direction = _view.getNormalMatrix() * direction;
    }

    Light::setupProgram(_view, _uniforms);

    auto& u = static_cast<DirectionalLight::Uniforms&>(_uniforms);
    u.shader.setUniformf(u.direction, direction);
}

std::string DirectionalLight::getClassBlock() {
    if (s_classBlock.empty()) {
        s_classBlock = std::string(reinterpret_cast<const char*>(directionalLight_glsl_data)) + "\n";
    }
    return s_classBlock;
}

std::string DirectionalLight::getInstanceDefinesBlock() {
    //	Directional lights don't have defines.... yet.
    return "\n";
}

std::string DirectionalLight::getInstanceAssignBlock() {
    std::string block = Light::getInstanceAssignBlock();
    if (!m_dynamic) {
        block += ", " + glm::to_string(m_direction) + ")";
    }
    return block;
}

const std::string& DirectionalLight::getTypeName() {

    return s_typeName;

}

}
