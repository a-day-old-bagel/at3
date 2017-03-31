attribute vec3 vertPosition;

uniform mat4 modelView;
uniform mat4 projection;

varying vec3 eyeDirection;

void main() {
    vec4 unprojected = (projection * vec4(vertPosition, 1.0));
    eyeDirection = (modelView * unprojected).xyz;
    gl_Position = vec4(vertPosition, 1.0);
}