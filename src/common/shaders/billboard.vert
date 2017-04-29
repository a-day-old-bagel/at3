
in vec3 vertPosition;
in vec2 vertTexCoord;

uniform mat4 modelView;
uniform mat4 projection;

void main() {
  gl_Position = projection * modelView * vec4(vertPosition, 1.0);
}
