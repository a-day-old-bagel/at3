
in vec2 vertPosition;
uniform sampler2D terrain;

void main(void){
    vec2 texcoord = vertPosition;
    float height = texture(terrain, vertPosition).a;
    vec4 displaced = vec4(vertPosition, height, 1.0);
    gl_Position = displaced;
}