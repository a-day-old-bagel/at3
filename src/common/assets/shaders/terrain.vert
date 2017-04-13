
in vec3 vertPosition;
in vec2 vertPosition;

uniform mat4 modelView;
uniform mat4 projection;
uniform sampler2D terrain;

out vec2 texCoord;

void main(void){
    vec2 texcoord = vertPosition.xy;
    float height = texture(terrain, vertPosition).a;
    vec4 displaced = vec4(vertPosition.x, vertPosition.y, height, 1.0);
    vec4 transformed = projection * modelView * displaced;
    gl_Position = displaced;
}