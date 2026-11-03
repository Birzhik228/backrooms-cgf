// Included by world.cpp beside its private mesh helpers. These are small,
// walk-over floor details; none changes the collision, lighting or room graph.
void level0Landmark(Chunk& chunk,float x,float z,int kind,bool observation) {
    if(kind==0) {
        // Fallen, unpowered fluorescent housing and two dull broken tubes.
        box(chunk,x+4.92f,.018f,z+5.10f,x+6.82f,.075f,z+5.52f,7);
        box(chunk,x+4.92f,.075f,z+5.10f,x+6.82f,.13f,z+5.15f,7);
        box(chunk,x+4.92f,.075f,z+5.47f,x+6.82f,.13f,z+5.52f,7);
        box(chunk,x+5.02f,.075f,z+5.22f,x+5.86f,.105f,z+5.27f,13);
        box(chunk,x+5.95f,.075f,z+5.22f,x+6.69f,.105f,z+5.27f,13);
        box(chunk,x+5.16f,.075f,z+5.36f,x+6.47f,.105f,z+5.41f,13);
        box(chunk,x+5.01f,.076f,z+5.18f,x+5.08f,.12f,z+5.45f,3);
        box(chunk,x+6.63f,.076f,z+5.18f,x+6.70f,.12f,z+5.45f,3);
    } else if(kind==1) {
        // Displaced acoustic tiles with exposed dark backing and a bent frame.
        box(chunk,x+5.08f,.012f,z+5.03f,x+6.29f,.052f,z+5.73f,3);
        quad(chunk,{x+5.08f,.054f,z+5.73f},{x+6.29f,.054f,z+5.73f},
             {x+6.29f,.054f,z+5.03f},{x+5.08f,.054f,z+5.03f},{0,1,0},1.21f,.7f,2);
        quad(chunk,{x+6.36f,.016f,z+5.15f},{x+6.30f,.016f,z+5.58f},
             {x+6.65f,.016f,z+5.88f},{x+6.83f,.016f,z+5.42f},{0,1,0},.5f,.5f,2);
        box(chunk,x+5.02f,.012f,z+5.0f,x+5.06f,.045f,z+5.88f,7);
        box(chunk,x+5.02f,.012f,z+5.84f,x+6.77f,.045f,z+5.88f,7);
    } else {
        // A dark fan-shaped water mark and a few strips of stripped wallpaper.
        // Geometry is sparse rather than an additional repeating wall texture.
        const std::array<P,8> edge={{{x+5.04f,.004f,z+5.48f},{x+5.30f,.004f,z+5.81f},
            {x+5.80f,.004f,z+5.95f},{x+6.42f,.004f,z+5.77f},
            {x+6.73f,.004f,z+5.41f},{x+6.40f,.004f,z+5.10f},
            {x+5.88f,.004f,z+5.06f},{x+5.26f,.004f,z+5.25f}}};
        const auto stainFirst=chunk.vertices.size();
        for(int side=0;side<8;side+=2)
            quad(chunk,{x+5.85f,.004f,z+5.50f},edge[side],edge[side+1],edge[(side+2)%8],
                 {0,1,0},1,1,18);
        // Continuous carpet fibres across the irregular damp patch.
        for(std::size_t i=stainFirst;i<chunk.vertices.size();++i) {
            auto& v=chunk.vertices[i];v.u=v.x;v.v=-v.z;
        }
        for(int strip=0;strip<3;++strip) {
            const float sx=x+5.18f+strip*.43f,sz=z+5.20f+strip*.12f;
            quad(chunk,{sx,.009f,sz},{sx+.06f,.009f,sz+.45f},
                 {sx+.31f,.009f,sz+.51f},{sx+.19f,.009f,sz+.03f},{0,1,0},.25f,.5f,0);
        }
    }
    if(!observation)return;
    // The paper is legible as an object; its prose appears on inspection.
    // A dark backing, pale sheet, clip and ink strokes read without new textures.
    box(chunk,x+5.53f,.02f,z+5.90f,x+6.17f,.073f,z+6.43f,3);
    quad(chunk,{x+5.58f,.09f,z+6.38f},{x+6.12f,.09f,z+6.38f},
         {x+6.12f,.09f,z+5.95f},{x+5.58f,.09f,z+5.95f},{0,1,0},.54f,.43f,46);
    box(chunk,x+5.77f,.091f,z+5.94f,x+5.95f,.105f,z+5.99f,7);
    for(int line=0;line<4;++line) {
        const float zz=z+6.06f+line*.067f;
        const float end=x+6.04f-(line==3?.13f:0.f);
        quad(chunk,{x+5.64f,.092f,zz+.009f},{end,.092f,zz+.009f},
             {end,.092f,zz},{x+5.64f,.092f,zz},{0,1,0},.4f,.009f,25);
    }
}
