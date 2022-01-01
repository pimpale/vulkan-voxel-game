#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texAtlasSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {
  // const float ambientStrength = 0.5;
  // vec3 lightColor = normalize(vec3(1.0, 1.0, 1.0));
  // vec3 objectColor = texture(texAtlasSampler, fragTexCoord).rgb;
  // vec3 ambient = ambientStrength * lightColor;
  // vec3 lightDir = normalize(vec3(1.0, -1.0, 1.0));
  // float diff = max(dot(fragNormal, lightDir), 0.0);
  // vec3 diffuse = diff * lightColor;
  // vec3 result = (ambient + diffuse) * objectColor;
  // outColor = vec4(result, 1.0);

  outColor = texture(texAtlasSampler, fragTexCoord);

}
