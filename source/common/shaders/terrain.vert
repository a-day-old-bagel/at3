
in vec2 vertPosition;
uniform sampler2D terrain;

void main(void){
    vec2 normalizedPosition = vertPosition + 0.5;
    float height = texture(terrain, normalizedPosition).a;
    vec4 displaced = vec4(vertPosition, height, 1.0);
    gl_Position = displaced;
}