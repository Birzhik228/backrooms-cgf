#pragma once
// Included in world.cpp's geometry namespace. All garage assets are procedural.
// Materials 70..90 belong to Parking; each cell keeps a clear north/south aisle.
void parkingLetter(Chunk& c,float x,float y,float z,char letter,float size,float material=90) {
    const char* bits=letter=='A'?"01110100011000111111100011000110001":
                     letter=='B'?"11110100011000111110100011000111110":
                     letter=='C'?"01111100001000010000100001000001111":
                                 "11110100011000111110100001000010000"; // P
    for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(bits[row*5+col]=='1') {
        const float xx=x+col*size,yy=y+(6-row)*size;
        quad(c,{xx,yy,z},{xx+size*.88f,yy,z},{xx+size*.88f,yy+size*.88f,z},{xx,yy+size*.88f,z},{0,0,1},size,size,material);
    }
}
void parkingCar(Chunk& c,float x,float z,std::uint64_t h,bool distant) {
    const float paint=86+float(h%3);
    // A low sedan: wheels, bumpers, bonnet, dark glazing, pillars and roof.
    box(c,x-.90f,.33f,z-2.05f,x+.90f,.83f,z+2.05f,paint,true);
    box(c,x-.80f,.81f,z-.95f,x+.80f,1.37f,z+1.12f,74);
    box(c,x-.82f,1.35f,z-.85f,x+.82f,1.43f,z+1.02f,paint);
    for(float side:{-1.f,1.f}) {
        for(float end:{-1.f,1.f}) {
            // Wheel faces are an ellipsoid, avoiding square tyres in silhouette.
            funOrb(c,x+side*.86f,.33f,z+end*1.30f,.15f,.33f,.33f,75,distant?4:6,distant?6:10);
            if(!distant)funOrb(c,x+side*.985f,.33f,z+end*1.30f,.018f,.18f,.18f,7,4,8);
            box(c,x+side*.77f-.03f,.82f,z+end*.94f-.04f,x+side*.77f+.03f,1.38f,z+end*.94f+.04f,paint);
        }
        box(c,x+side*.8f-.025f,.84f,z+.08f,x+side*.8f+.025f,1.37f,z+.18f,paint);
        if(!distant)box(c,x+side*.94f-.09f,.98f,z-.76f,x+side*.94f+.09f,1.08f,z-.52f,paint);
    }
    for(float end:{-1.f,1.f}) {
        box(c,x-.89f,.32f,z+end*2.04f-.04f,x+.89f,.46f,z+end*2.04f+.04f,75);
        for(float side:{-.65f,.65f})box(c,x+side-.17f,.64f,z+end*2.055f-.012f,x+side+.17f,.77f,z+end*2.055f+.012f,end>0?77:90);
        if(!distant)box(c,x-.22f,.48f,z+end*2.065f-.008f,x+.22f,.59f,z+end*2.065f+.008f,90);
    }
}
Chunk parkingChunk(ChunkCoord coord,std::uint64_t seed,const std::array<bool,3>& powered,bool distant=false) {
    Chunk c;c.coord=coord;c.maxHeight=float(ROOM_HEIGHT);
    const auto exit=parkingExitCell(seed);
    for(int z=0;z<CHUNK_CELLS;++z)for(int x=0;x<CHUNK_CELLS;++x) {
        const auto gx=coord.x*CHUNK_CELLS+x,gz=coord.z*CHUNK_CELLS+z;
        const auto h=hashCell(gx,gz,seed^0x5041524b494e47ULL);
        const int sector=parkingSector(gx,gz);
        const float x0=float(x*CELL),z0=float(z*CELL),x1=x0+float(CELL),z1=z0+float(CELL);
        quad(c,{x0,0,z1},{x1,0,z1},{x1,0,z0},{x0,0,z0},{0,1,0},float(CELL),float(CELL),71);
        quad(c,{x0,3.2f,z0},{x1,3.2f,z0},{x1,3.2f,z1},{x0,3.2f,z1},{0,-1,0},float(CELL),float(CELL),70);
        c.overheads.push_back({{x0,z0,x1,z1},3.2f});
        box(c,x0,2.85f,z1-.62f,x1,3.19f,z1-.13f,70);
        // Column and caution-painted base, entirely within this cell.
        box(c,x1-1.02f,0,z1-1.02f,x1-.32f,3.2f,z1-.32f,70,true);
        box(c,x1-1.035f,.04f,z1-1.035f,x1-.305f,.95f,z1-.305f,72);
        c.sightBlockers.push_back({{x1-1.02f,z1-1.02f,x1-.32f,z1-.32f},0,3.2f});
        box(c,x1-1.04f,1.65f,z1-.30f,x1-.30f,2.48f,z1-.27f,89);
        parkingLetter(c,x1-.94f,1.79f,z1-.265f,char('A'+sector),.10f);
        // Painted bays, concrete wheel stops and a dashed centre line.
        for(float lane:{.25f,2.85f,4.65f,7.25f})
            box(c,x0+lane,.003f,z0+.22f,x0+lane+.055f,.007f,z0+5.18f,73);
        for(float lane:{.25f,4.65f}) {
            box(c,x0+lane,.003f,z0+5.13f,x0+lane+2.65f,.007f,z0+5.2f,73);
            box(c,x0+lane+.5f,.015f,z0+.32f,x0+lane+2.f,.13f,z0+.48f,70,true);
        }
        for(float at:{.55f,3.05f,5.55f})box(c,x0+3.72f,.003f,z0+at,x0+3.78f,.007f,z0+at+.65f,73);
        // Low service pipes and brackets; leave at least 2.7 m headroom.
        for(float offset:{.7f,1.02f})box(c,x0+offset,2.72f,z0,x0+offset+.10f,2.82f,z1,76);
        if(!distant)for(float at:{1.f,4.7f})box(c,x0+.57f,2.68f,z0+at,x0+1.24f,2.72f,z0+at+.08f,7);
        const bool exitHere=gx==exit.x && gz==exit.z;
        int panel=-1;
        for(int i=0;i<3;++i)if(ChunkCoord{gx,gz}==parkingBreakerCell(seed,i))panel=i;
        if(exitHere) {
            const auto begin=c.vertices.size();exitDoor(c,x0,z0);
            for(auto i=begin;i<c.vertices.size();++i)if(c.vertices[i].material==0 || c.vertices[i].material==3)c.vertices[i].material=70;
        } else if(panel>=0) {
            // A freestanding electrical cabinet in the east bay, facing the aisle.
            box(c,x0+5.15f,0,z0+.80f,x0+6.5f,2.45f,z0+1.15f,70,true);
            c.sightBlockers.push_back({{x0+5.15f,z0+.80f,x0+6.5f,z0+1.15f},0,2.45f});
            box(c,x0+5.35f,.75f,z0+1.15f,x0+6.3f,1.95f,z0+1.32f,7);
            box(c,x0+5.42f,1.58f,z0+1.325f,x0+6.22f,1.84f,z0+1.34f,75);
            box(c,x0+5.51f,1.64f,z0+1.345f,x0+5.68f,1.79f,z0+1.36f,powered[panel]?79:78);
            box(c,x0+5.80f,powered[panel]?1.24f:1.04f,z0+1.34f,x0+5.98f,powered[panel]?1.48f:1.28f,z0+1.47f,72);
            parkingLetter(c,x0+5.63f,2.02f,z0+1.155f,char('A'+panel),.045f);
            c.lamps.push_back({x0+5.8f,1.7f,z0+1.8f,.65f,powered[panel]?.20f:1.f,powered[panel]?1.f:.18f,.12f});
        }
        if(!exitHere && panel<0 && h%11==0 && !(gx==0&&gz==0))parkingCar(c,x0+1.55f,z0+2.8f,h,distant);
        // Emergency lights survive in every sector. Restoring a circuit makes
        // its remaining fittings steady and turns the dead ones back on.
        LampState state=powered[sector]?LampState::Working:(h%9==0?LampState::Dying:LampState::Off);
        if((gx==0&&gz==0)||panel>=0)state=LampState::Working;
        const bool amber=h%7==0;
        const float phase=float((h>>16)%90)*.01f;
        box(c,x0+2.65f,3.04f,z0+3.5f,x0+4.85f,3.18f,z0+3.97f,75);
        box(c,x0+2.75f,3.025f,z0+3.58f,x0+4.75f,3.04f,z0+3.89f,float(amber?83:80)+float(state)+phase);
        c.lamps.push_back({x0+3.75f,2.98f,z0+3.75f,state==LampState::Off?0.f:powered[sector]?2.2f:state==LampState::Working?1.3f:.25f,
            amber?1.f:.77f,amber?.64f:.88f,amber?.27f:1.f,state,phase});
    }
    if(!distant)c.distantVertices=parkingChunk(coord,seed,powered,true).vertices;
    return c;
}
