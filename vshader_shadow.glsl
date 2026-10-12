#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=3) in float aMaterial;
uniform mat4 uModel,uLightVP;
flat out int material;
void main(){material=int(floor(aMaterial));gl_Position=uLightVP*uModel*vec4(aPosition,1);}
