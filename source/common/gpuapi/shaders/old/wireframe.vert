
in vec3 vertColor;
in vec3 vertPosition;

uniform mat4 modelView;
uniform mat4 projection;

out vec3 color;

void main() {
  gl_Position = projection * modelView * vec4(vertPosition, 1.0);
  color = vertColor;
}
