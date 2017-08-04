
uniform samplerCube texture0;
in vec3 eyeDirection;
out vec4 fragColor;

void main() {
    fragColor = texture(texture0, eyeDirection);
    float interp = clamp(pow(normalize(eyeDirection).y + 1.0, 3.0), 0.0, 1.0);
    fragColor = mix(fragColor, vec4(0.8, 0.82, 0.9, 1.0), interp);
}