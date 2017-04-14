
uniform samplerCube texture0;
in vec3 eyeDirection;
out vec4 fragColor;

void main() {
    fragColor = texture(texture0, eyeDirection);
}