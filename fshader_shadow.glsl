#version 330 core
flat in int material;
void main(){
    // Decal clues and luminous lamp faces must not occlude their own source.
    if(material==5||material==4||material==27||(material>=32&&material<=34))discard;
}
