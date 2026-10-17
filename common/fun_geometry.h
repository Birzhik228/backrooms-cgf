#pragma once
// Included in world.cpp's geometry namespace, sharing its mesh/collision tools.
// Materials 40..55 are reserved for the party rooms.
void funOrb(Chunk& c,float x,float y,float z,float rx,float ry,float rz,float mat,int rings=6,int sides=10) {
    constexpr float pi=3.14159265359f;
    auto point=[&](float a,float b) {return P{x+rx*std::cos(a)*std::cos(b),y+ry*std::sin(a),z+rz*std::cos(a)*std::sin(b)};};
    auto vertex=[&](P p,float u,float v) {
        const float nx=(p.x-x)/(rx*rx),ny=(p.y-y)/(ry*ry),nz=(p.z-z)/(rz*rz);
        const float len=std::sqrt(nx*nx+ny*ny+nz*nz);
        return Vertex{p.x,p.y,p.z,nx/len,ny/len,nz/len,u,v,mat};
    };
    for(int ring=0;ring<rings;++ring) for(int side=0;side<sides;++side) {
        float a=-pi/2+ring*pi/rings,b=a+pi/rings,u=side*2*pi/sides,v=u+2*pi/sides;
        const Vertex q[]={vertex(point(a,u),u,a),vertex(point(b,u),u,b),vertex(point(b,v),v,b),vertex(point(a,v),v,a)};
        // Omit degenerate pole triangles.
        if(ring!=rings-1) for(int k:{0,1,2}) c.vertices.push_back(q[k]);
        if(ring!=0) for(int k:{0,2,3}) c.vertices.push_back(q[k]);
    }
}
void funCylinder(Chunk& c,float x,float y,float z,float r,float height,float material,bool distant=false) {
    const int sides=distant?6:10;
    for(int i=0;i<sides;++i) {
        const float a=i*6.2831853f/sides,b=(i+1)*6.2831853f/sides;
        P p{x+std::cos(a)*r,y,z+std::sin(a)*r},q{x+std::cos(b)*r,y,z+std::sin(b)*r};
        P n{std::cos((a+b)*.5f),0,std::sin((a+b)*.5f)};
        quad(c,p,{p.x,y+height,p.z},{q.x,y+height,q.z},q,n,.12f,height,material);
        // Upward-facing triangle fan; no invisible bottom faces.
        for(P t:{P{x,y+height,z},P{q.x,y+height,q.z},P{p.x,y+height,p.z}})
            c.vertices.push_back({t.x,t.y,t.z,0,1,0,t.x-x,t.z-z,material});
    }
}
void funChair(Chunk& c,float x,float z,bool south,bool distant=false) {
    constexpr float half=.24f;
    box(c,x-half,.44f,z-half,x+half,.51f,z+half,43);
    const float back=z+(south?.23f:-.23f);
    for(float side:{-.20f,.20f}) {
        box(c,x+side-.028f,0,z-.21f,x+side+.028f,.45f,z-.155f,43);
        box(c,x+side-.028f,0,z+.155f,x+side+.028f,.45f,z+.21f,43);
        box(c,x+side-.028f,.48f,back-.025f,x+side+.028f,1.04f,back+.025f,43);
    }
    box(c,x-.22f,.92f,back-.03f,x+.22f,1.04f,back+.03f,43);
    if(!distant) for(float slat:{-.10f,0.f,.10f}) box(c,x+slat-.016f,.53f,back-.018f,x+slat+.016f,.94f,back+.018f,43);
    c.obstacles.push_back({x-half,z-half,x+half,z+half});
}
void funTable(Chunk& c,float x,float z,std::uint64_t h,bool distant=false) {
    constexpr float hw=1.27f,hd=.60f,top=.84f;
    for(float sx:{-1.f,1.f}) for(float sz:{-1.f,1.f})
        box(c,x+sx*1.08f-.045f,0,z+sz*.44f-.045f,x+sx*1.08f+.045f,.80f,z+sz*.44f+.045f,17);
    box(c,x-hw,.77f,z-hd,x+hw,top,z+hd,42);
    // Cloth hangs down on all sides; collider prevents walking through the table.
    box(c,x-hw,.57f,z-hd,x+hw,.79f,z-hd+.018f,42);
    box(c,x-hw,.57f,z+hd-.018f,x+hw,.79f,z+hd,42);
    box(c,x-hw,.57f,z-hd,x-hw+.018f,.79f,z+hd,42);
    box(c,x+hw-.018f,.57f,z-hd,x+hw,.79f,z+hd,42);
    c.obstacles.push_back({x-hw,z-hd,x+hw,z+hd});
    int setting=0;
    for(float sx:{-.70f,.70f}) for(float sz:{-1.f,1.f}) {
        funChair(c,x+sx,z+sz*1.02f,sz>0,distant);
        const auto place=hashCell(setting++,29,h);
        if(place%4==0)continue; // Some seats have no dishes at all.
        const float px=x+sx,pz=z+sz*.33f;
        funCylinder(c,px,top,pz,.18f,.018f,45,distant);
        funCylinder(c,px,top+.018f,pz,.14f,.009f,46,distant);
        // Only some plates have a slice; both LODs use the same decision.
        if((place>>4)%3==0) {
        box(c,px-.075f,top+.027f,pz-.060f,px+.075f,top+.10f,pz+.06f,44);
        box(c,px-.077f,top+.062f,pz-.062f,px+.077f,top+.074f,pz+.062f,46);
        box(c,px-.078f,top+.10f,pz-.064f,px+.078f,top+.12f,pz+.064f,46);
        if(!distant) funOrb(c,px,top+.137f,pz,.025f,.025f,.025f,48,4,6);
        }
        if((place>>8)%2==0) funCylinder(c,px+.27f,top,pz,.048f,.13f,48+float(h%6),distant);
    }
    // A centrepiece appears on about one table in three.
    if((h>>20)%3!=0)return;
    funCylinder(c,x,top,z,.27f,.02f,45,distant);
    funCylinder(c,x,top+.02f,z,.22f,.16f,44,distant);
    funCylinder(c,x,top+.09f,z,.224f,.025f,46,distant);
    funCylinder(c,x,top+.18f,z,.225f,.03f,46,distant);
    if(!distant) for(float dx:{-.1f,0.f,.1f}) funCylinder(c,x+dx,top+.21f,z,.012f,.10f,49+float(h%4),distant);
}
void funBalloons(Chunk& c,float x,float z,std::uint64_t h,bool distant=false) {
    box(c,x-.12f,0,z-.12f,x+.12f,.10f,z+.12f,49);
    for(int i=0;i<3;++i) {
        const float bx=x+(i-1)*.31f,bz=z+(i==1?-.12f:.12f),y=2.08f+(i==1?.38f:0.f);
        funOrb(c,bx,y,bz,.23f,.31f,.23f,48+float((h+i)%6),distant?4:6,distant?6:10);
        funOrb(c,bx,y-.31f,bz,.035f,.035f,.035f,48+float((h+i)%6),distant?4:6,distant?6:10);
        // Thin ribbon follows the balloon to its floor weight.
        for(int j=0;j<8;++j) {
            const float t=j/8.f,u=(j+1)/8.f;
            const float ax=x+(bx-x)*t,az=z+(bz-z)*t,ay=.10f+(y-.42f)*t;
            const float ex=x+(bx-x)*u,ez=z+(bz-z)*u,ey=.10f+(y-.42f)*u;
            quad(c,{ax-.006f,ay,az},{ex-.006f,ey,ez},{ex+.006f,ey,ez},{ax+.006f,ay,az},{0,0,-1},.012f,ey-ay,45);
            quad(c,{ax+.006f,ay,az},{ex+.006f,ey,ez},{ex-.006f,ey,ez},{ax-.006f,ay,az},{0,0,1},.012f,ey-ay,45);
        }
    }
}
void funBunting(Chunk& c,float x0,float z0,std::uint64_t h) {
    for(int i=0;i<12;++i) {
        const float x=x0+.25f+i*.58f,z=z0+3.0f;
        const float y=3.10f-.20f*std::sin((i+.5f)/12*3.14159f);
        const float mat=48+float((h+i)%6);
        box(c,x,y,z-.008f,x+.58f,y+.012f,z+.008f,45);
        const P a{x+.055f,y,z},b{x+.49f,y,z},tip{x+.27f,y-.34f,z};
        for(float side:{-1.f,1.f}) {
            const P q[]={a,side>0?tip:b,side>0?b:tip};
            for(P p:q) c.vertices.push_back({p.x,p.y,p.z,0,0,side,p.x-x,y-p.y,mat});
        }
    }
}
Chunk funChunk(ChunkCoord coord,std::uint64_t seed,std::uint64_t variant,bool distant=false) {
    Chunk c;c.coord=coord;c.maxHeight=float(ROOM_HEIGHT);
    for(int z=0;z<CHUNK_CELLS;++z) for(int x=0;x<CHUNK_CELLS;++x) {
        const auto gx=coord.x*CHUNK_CELLS+x,gz=coord.z*CHUNK_CELLS+z;
        const float x0=float(x*CELL),z0=float(z*CELL),x1=x0+float(CELL),z1=z0+float(CELL);
        const auto h=hashCell(gx,gz,seed^0x46554e);
        quad(c,{x0,0,z1},{x1,0,z1},{x1,0,z0},{x0,0,z0},{0,1,0},float(CELL),float(CELL),41);
        quad(c,{x0,float(ROOM_HEIGHT),z0},{x1,float(ROOM_HEIGHT),z0},{x1,float(ROOM_HEIGHT),z1},{x0,float(ROOM_HEIGHT),z1},{0,-1,0},float(CELL),float(CELL),2);
        c.overheads.push_back({{x0,z0,x1,z1},float(ROOM_HEIGHT)});
        for(bool east:{true,false}) {
            const auto bx=gx+(east?1:0),bz=gz+(east?0:1);
            if(sharedHall(gx,gz,bx,bz,seed)) continue;
            const auto start=c.vertices.size();
            wall(c,east,east?x1:z1,east?z0:x0,edgeOpen(gx,gz,bx,bz,seed,variant),true);
            for(std::size_t i=start;i<c.vertices.size();++i) if(c.vertices[i].material==0)c.vertices[i].material=40;
        }
        // Preserve a clear cross between every cell centre and its doorways.
        const float jitter=float((h>>8)%201)*.001f-.1f;
        if(h%2==0 || (gx==0&&gz==0)) funTable(c,x0+1.65f+jitter,z0+1.65f,h,distant);
        if((h>>12)%5==0) funTable(c,x0+5.85f-jitter,z0+5.85f,h>>4,distant);
        if((h>>16)%3!=0 || (gx==0&&gz==0)) funBalloons(c,x0+6.1f,z0+1.45f,h,distant);
        if((h>>24)%3!=0) funBunting(c,x0,z0,h);
        ceilingLight(c,x0+float(CELL/2),z0+float(CELL/2),1.65f,LampState::Working,0);
        // A few gift boxes occupy unused corners, never the connecting routes.
        if((h>>28)%4==0) {
            box(c,x0+.5f,0,z0+5.6f,x0+1.1f,.65f,z0+6.2f,48+float(h%6),true);
            box(c,x0+.75f,.65f,z0+5.58f,x0+.82f,.665f,z0+6.22f,45);
            box(c,x0+.48f,.65f,z0+5.85f,x0+1.12f,.665f,z0+5.92f,45);
        }
    }
    if(!distant) c.distantVertices=funChunk(coord,seed,variant,true).vertices;
    return c;
}
