in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexCoord;

uniform mat4 model;
uniform mat4 modelView;
uniform mat4 projection;

out vec2 texCoord;
out vec3 normal;

void main() {
  gl_Position = projection * modelView * vec4(vertPosition, 1.0);
  normal = (model * vec4(vertNormal, 0.f)).xyz;
  texCoord = vertTexCoord;
}
