#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform vec3 u_CameraPos;

out vec3 v_Normal;
out vec3 v_FragPos;
out vec3 v_ViewDir;
out vec2 v_TexCoord;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    gl_Position = u_Projection * u_View * worldPos;
    
    v_Normal   = normalize(mat3(transpose(inverse(u_Model))) * a_Normal);
    v_FragPos  = worldPos.xyz;
    v_ViewDir  = normalize(u_CameraPos - worldPos.xyz);
    v_TexCoord = a_TexCoord;
}
