
in vec3 vertPosition;
in vec2 vertTexCoord;

out vec2 texCoord;

void main() {
    texCoord = vertTexCoord;
    gl_Position = vec4(vertPosition, 1.0);
}
