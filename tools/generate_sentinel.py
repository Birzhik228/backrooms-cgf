"""Generate the original Threshold Sentinel mesh, textures and seven BRARAN3 clips.

No imported model, image, animation, rig or reference mesh is read. The design
is a plated quadruped assembled from mathematical surfaces and authored points.
Requires Python and NumPy; Blender is not needed. Legacy aranea runtime paths
are kept only for compatibility with the existing cache loader.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import struct
import numpy as np

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'assets/models/aranea'
PI = math.pi
parts = []
triangles = []
masks = []
uvs = []
landmarks = []
vertex_count = 0

def vec(x): return np.asarray(x, dtype=float)
def normalize(x): return x / max(1e-12, np.linalg.norm(x))
def smooth(x):
    x = np.clip(x, 0, 1)
    return x*x*(3-2*x)
def rot(axis, angle):
    x, y, z = normalize(vec(axis)); c, s = math.cos(angle), math.sin(angle); t = 1-c
    return np.array([[t*x*x+c,t*x*y-s*z,t*x*z+s*y],
                     [t*x*y+s*z,t*y*y+c,t*y*z-s*x],
                     [t*x*z-s*y,t*y*z+s*x,t*z*z+c]])
def align(a, b):
    a, b = normalize(a), normalize(b)
    cross = np.cross(a,b); dot = np.clip(np.dot(a,b),-1,1)
    if np.linalg.norm(cross)<1e-10:
        return np.eye(3) if dot>0 else rot((1,0,0),PI)
    return rot(cross, math.acos(dot))

def add_part(name, positions, faces, material=20, joint='body', uv=None, tip=None):
    global vertex_count
    p = np.asarray(positions,dtype=float)
    n = np.zeros_like(p)
    valid = []
    for face in faces:
        a,b,c=face
        normal=np.cross(p[b]-p[a],p[c]-p[a])
        if np.linalg.norm(normal)<1e-9: continue
        n[a]+=normal;n[b]+=normal;n[c]+=normal;valid.append(face)
    lengths=np.linalg.norm(n,axis=1)
    assert np.all(lengths>1e-10),name
    n/=lengths[:,None]
    if uv is None:
        uv=[(float(q[0]*.37+.5),float(q[1]*.37+q[2]*.13)) for q in p]
    feet_index = int(joint[-1]) if joint.startswith('lower') else None
    pm=[]
    for k in range(len(p)):
        feet=[0]*4
        if feet_index is not None and tip is not None and k in tip:feet[feet_index]=255
        pm.append([255 if joint in ('head','jaw') else 0,*feet])
    parts.append(dict(name=name,p=p,n=n,joint=joint,offset=vertex_count))
    triangles.extend((a+vertex_count,b+vertex_count,c+vertex_count,material) for a,b,c in valid)
    masks.extend(pm);uvs.extend(uv);vertex_count+=len(p)
    return parts[-1]

def ellipsoid(name,center,radii,material=20,joint='body',segments=20,rings=10):
    p=[];faces=[];uv=[]
    p.append(vec(center)+vec((0,radii[1],0)));uv.append((.5,1))
    for j in range(1,rings):
        a=PI*j/rings
        for k in range(segments):
            t=2*PI*k/segments
            p.append(vec(center)+vec((radii[0]*math.sin(a)*math.cos(t),radii[1]*math.cos(a),radii[2]*math.sin(a)*math.sin(t))))
            uv.append((k/segments,j/rings))
    bottom=len(p);p.append(vec(center)-vec((0,radii[1],0)));uv.append((.5,0))
    for k in range(segments):
        kn=(k+1)%segments
        faces.append((0,1+kn,1+k))
        for j in range(rings-2):
            a=1+j*segments+k;b=1+j*segments+kn;c=a+segments;d=b+segments
            faces.extend(((a,b,c),(b,d,c)))
        faces.append((bottom,1+(rings-2)*segments+k,1+(rings-2)*segments+kn))
    return add_part(name,p,faces,material,joint,uv)

def plate(name,outline,zfront,zback,material=21,joint='body'):
    # Extruded polygon, specified clockwise in the visible front (negative Z).
    p=[(x,y,zfront) for x,y in outline]+[(x*.93,y,zback) for x,y in outline]
    m=len(outline);faces=[]
    for i in range(1,m-1):faces.extend(((0,i,i+1),(m,m+i+1,m+i)))
    for i in range(m):
        j=(i+1)%m;faces.extend(((i,m+i,j),(j,m+i,m+j)))
    return add_part(name,p,faces,material,joint)

def tube(name,a,b,radius_a,radius_b,material=20,joint='body',segments=12,rings=4,lower=False):
    a,b=vec(a),vec(b);axis=normalize(b-a)
    right=normalize(np.cross(axis,(0,0,1)))
    if abs(np.dot(axis,(0,0,1)))>.98:right=normalize(np.cross(axis,(0,1,0)))
    if lower and right[0]<0:right=-right
    side=normalize(np.cross(axis,right))
    p=[];uv=[];faces=[]
    for j in range(rings):
        t=j/(rings-1);center=a+(b-a)*t;r=radius_a*(1-t)+radius_b*t
        for i in range(segments):
            angle=2*PI*i/segments
            radial=right*math.cos(angle)+side*math.sin(angle)
            if lower:
                blend=smooth((t-.70)/.30)
                radial=radial*(1-blend)+vec((math.cos(angle),0,math.sin(angle)))*blend
            p.append(center+radial*r);uv.append((i/segments,t))
    for j in range(rings-1):
        for i in range(segments):
            k=j*segments+i;kn=j*segments+(i+1)%segments
            # Consistent outward winding along the axis.
            faces.extend(((k,kn,k+segments),(kn,kn+segments,k+segments)))
    start=len(p);p.append(a);uv.append((.5,0))
    end=len(p);p.append(b);uv.append((.5,1))
    for i in range(segments):
        j=(i+1)%segments
        faces.extend(((start,j,i),(end,(rings-1)*segments+i,(rings-1)*segments+j)))
    # Correct winding against the radial vector; tube orientation can vary.
    for k,face in enumerate(faces):
        i,j,l=face;mid=(p[i]+p[j]+p[l])/3;center=a+axis*np.clip(np.dot(mid-a,axis),0,np.linalg.norm(b-a))
        desired=-axis if start in face else axis if end in face else mid-center
        if np.linalg.norm(desired)>1e-7 and np.dot(np.cross(p[j]-p[i],p[l]-p[i]),desired)<0:faces[k]=(i,l,j)
    tip=set(range((rings-1)*segments,rings*segments))|{end} if lower else None
    return add_part(name,p,faces,material,joint,uv,tip)

def eye_ring(side,x):
    p=[];faces=[];segments=24
    # Three coaxial rings form a circular ceramic sensor bezel, not a skull.
    for radius,z in ((.115,-1.045),(.083,-1.075),(.060,-1.046)):
        for i in range(segments):
            a=2*PI*i/segments;p.append((x+radius*math.cos(a),2.015+radius*math.sin(a),z))
    for j in range(2):
        for i in range(segments):
            n=(i+1)%segments;a=j*segments+i;b=j*segments+n;c=a+segments;d=b+segments
            faces.extend(((a,c,b),(b,c,d)))
    part=add_part(side+' sensor bezel',p,faces,21,'head')
    landmarks.append(dict(side=side,first=part['offset'],count=len(p)))
    ellipsoid(side+' dark sensor',(x,2.015,-1.034),(.076,.076,.028),22,'head',16,8)
    ellipsoid(side+' sensor aperture',(x,2.015,-1.067),(.018,.042,.008),21,'head',12,6)

# Original sculptural layout: segmented bronze torso, porcelain angular mask,
# four exposed struts, layered joint caps and a ribbed dorsal crest.
ellipsoid('armored thorax',(0,1.52,.10),(.36,.31,.72),20,segments=28,rings=14)
ellipsoid('underside',(0,1.39,.08),(.255,.23,.62),22)
for j in range(7):
    z=-.38+j*.16
    ellipsoid('overlapping carapace '+str(j),(0,1.69,z),(.37,.145,.145),21,segments=20,rings=8)
    plate('dorsal crest '+str(j),[(-.085,1.74),(-.055,1.98+.10*math.sin(j/6*PI)),(.055,1.98+.10*math.sin(j/6*PI)),(.085,1.74)],z-.025,z+.025,20)
ellipsoid('neck collar',(0,1.73,-.49),(.24,.20,.26),22)
plate('faceted ceramic mask',[(-.32,2.08),(-.23,2.29),(.23,2.29),(.32,2.08),(.24,1.84),(0,1.72),(-.24,1.84)],-.99,-.64,21,'head')
plate('forehead shield',[(-.22,2.18),(0,2.38),(.22,2.18),(0,2.10)],-.985,-.78,20,'head')
eye_ring('left',-.152);eye_ring('right',.152)
plate('central bridge',[(-.026,2.15),(.026,2.15),(.05,1.90),(0,1.82),(-.05,1.90)],-1.086,-1.01,20,'head')
for x in (-.205,-.125,.125,.205):
    plate('cheek vent '+str(x),[(x-.018,1.91),(x+.018,1.91),(x+.012,1.81),(x-.012,1.81)],-1.017,-.99,22,'head')
plate('hinged lower shield',[(-.235,1.85),(.235,1.85),(.18,1.64),(0,1.53),(-.18,1.64)],-.96,-.68,20,'jaw')
for j in range(4):
    x=(j-1.5)*.085
    plate('lower grille '+str(j),[(x-.022,1.79),(x+.022,1.79),(x+.014,1.64),(x-.014,1.64)],-.975,-.95,21,'jaw')

legs=[]
for i,(sign,front) in enumerate(((-1,True),(1,True),(-1,False),(1,False))):
    hip=vec((sign*.28,1.49,-.43 if front else .52))
    knee=vec((sign*1.08,1.10,-.72 if front else .85))
    foot=vec((sign*.96,.012,-1.15 if front else 1.23))
    legs.append(dict(hip=hip,knee=knee,tip=foot,a=np.linalg.norm(knee-hip),b=np.linalg.norm(foot-knee)))
    tube('upper strut '+str(i),hip,knee,.10,.065,20,'upper'+str(i),16,5)
    ellipsoid('hip socket '+str(i),hip,(.14,.14,.14),22,'upper'+str(i),16,8)
    ellipsoid('knee shell '+str(i),knee,(.13,.16,.13),21,'upper'+str(i),16,8)
    tube('lower strut '+str(i),knee,foot,.070,.105,21,'lower'+str(i),16,7,True)
    # Offset brace adds layered manufactured detail to the upper limb.
    offset=vec((0,.07,.035))
    tube('exposed tendon '+str(i),hip+offset,knee+offset,.026,.023,22,'upper'+str(i),8,4)

EYE=vec((0,2.015,(-1.045-1.075-1.046)/3))
NECK=vec((0,1.74,-.59));JAW=vec((0,1.84,-.68));BODY=vec((0,1.52,.10))
def pose(phase,kind,duck):
    moving=kind in ('walk','run');running=kind=='run';scare=kind=='scare';a=phase*2*PI
    body_offset=vec((.025*math.sin(a) if moving else 0,(.025 if running else .012)*math.cos(a*2) if moving else .006*math.sin(a),0))
    body_offset[1]-=.38*duck
    body_rot=rot((0,0,1),(.022 if running else .012)*math.sin(a) if moving else .005*math.sin(a))
    head_rot=rot((1,0,0),.018*math.sin(a*2) if moving else .025*math.sin(a))
    jaw_rot=rot((1,0,0),-.035*(.5+.5*math.sin(a)))
    if scare:
        attack=smooth(phase/.42)
        body_offset=vec((0,-.08*attack,-.18*attack))
        body_rot=rot((1,0,0),-.12*attack)
        head_rot=rot((1,0,0),.12*attack+.045*math.sin(phase*30)*attack)
        jaw_rot=rot((1,0,0),-1.45*attack)
    def body(p):return (p-BODY)@body_rot.T+BODY+body_offset
    pivot=body(NECK);jaw=body(JAW)
    target=[];flags=0
    for i,l in enumerate(legs):
        u=(phase+([0,.5,.5,0] if running else [0,.5,.75,.25])[i])%1
        support=.58 if running else .78;stride=2 if running else 1.30
        foot=l['tip'].copy()
        if moving:
            if u<support:foot[2]+=(u-support*.5)*stride
            else:
                t=(u-support)/(1-support)
                foot[2]+=support*.5*stride*(1-2*smooth(t))
                foot[1]+=(.12 if duck else .30 if running else .18)*math.sin(PI*t)**1.3
        if not moving or u<support:flags|=1<<i
        foot[0]*=1-.58*duck
        if scare and i<2:
            attack=smooth(phase/.4);foot[1]+=.90*attack;foot[2]-=.60*attack;flags&=~(1<<i)
        hip=body(l['hip']);delta=foot-hip;distance=np.linalg.norm(delta)
        d=np.clip(distance,abs(l['a']-l['b'])+.001,l['a']+l['b']-.001);unit=normalize(delta)
        assert abs(distance-d)<.015,(kind,duck,i,phase,distance,d)
        pole=body(l['knee'])-hip if not duck else vec((-.10 if i in (0,2) else .10,-.25,-1 if i<2 else 1))
        pole=normalize(pole-unit*np.dot(pole,unit))
        along=(l['a']**2-l['b']**2+d*d)/(2*d)
        knee=hip+unit*along+pole*math.sqrt(max(0,l['a']**2-along*along))
        target.append((hip,knee,foot))
    positions=[];normals=[]
    for part in parts:
        p,n,joint=part['p'],part['n'],part['joint']
        if joint.startswith(('upper','lower')):
            i=int(joint[-1]);l=legs[i];hip,knee,foot=target[i];lower=joint.startswith('lower')
            start,end=(l['knee'],l['tip']) if lower else (l['hip'],l['knee'])
            current_start,current_end=(knee,foot) if lower else (hip,knee)
            rotation=align(end-start,current_end-current_start)
            q=(p-start)@rotation.T+current_start;ns=n@rotation.T
            if lower:
                t=((p-start)@(end-start))/np.dot(end-start,end-start)
                blend=smooth((t-.72)/.20)[:,None]
                q=q*(1-blend)+(p+foot-l['tip'])*blend
                ns=ns*(1-blend)+n*blend
        else:
            q=body(p);ns=n@body_rot.T
            if joint=='jaw':q=(q-jaw)@jaw_rot.T+jaw;ns=ns@jaw_rot.T
            if joint in ('head','jaw'):q=(q-pivot)@head_rot.T+pivot;ns=ns@head_rot.T
        positions.append(q);normals.append(ns)
    p=np.concatenate(positions);n=np.concatenate(normals);n/=np.linalg.norm(n,axis=1)[:,None]
    if scare:
        eye=(body(EYE)-pivot)@head_rot.T+pivot
        offset=vec((0,1.75,0))-eye;p+=offset;pivot+=offset
    return p,n,pivot,flags

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=OUT);args=parser.parse_args()
    output=args.output;output.mkdir(parents=True,exist_ok=True)
    clips=[('idle',16,3.2),('walk',40,1.2),('run',40,.65),('idle_duck',16,3.2),('walk_duck',40,1.2),('run_duck',40,.65),('scare',28,2.)]
    assert 100<vertex_count<20000 and 100<len(triangles)<25000
    bank=bytearray(struct.pack('<8sIIIIff',b'BRARAN3\0',3,vertex_count,len(triangles),7,1/8192,1/32767))
    for name,count,duration in clips:bank+=struct.pack('<16sIfI',name.encode(),count,duration,int(name!='scare'))
    for t in triangles:bank+=struct.pack('<IIIB3x',*t)
    for mask,uv in zip(masks,uvs):bank+=struct.pack('<BBBBBff',*mask,*uv)
    manifest=dict(name='Threshold Sentinel',origin='Original procedural geometry, texture and animation generated for this project; no imported assets',license='CC0-1.0',
        generator='tools/generate_sentinel.py',vertices=vertex_count,triangles=len(triangles),parts=len(parts),
        coordinateSystem='metres, Y-up, -Z forward',walkStride=1.30,runStride=2.,eyeLandmarks=landmarks,
        jawLandmark={'first':next(p['offset'] for p in parts if p['name']=='hinged lower shield'),'count':10},clips=[])
    for name,count,duration in clips:
        low=np.full(3,1e9);high=-low
        for frame in range(count):
            p,n,pivot,flags=pose(frame/(count-1 if name=='scare' else count),name.split('_')[0],name.endswith('_duck'))
            low=np.minimum(low,p.min(axis=0));high=np.maximum(high,p.max(axis=0))
            values=np.concatenate((p*8192,n*32767),axis=1).round()
            assert np.abs(values).max()<=32767,(name,frame,'quantization overflow')
            bank+=struct.pack('<hhhBB',*[round(v*8192) for v in pivot],flags,0)+values.astype('<i2').tobytes()
        manifest['clips'].append(dict(name=name,frames=count,duration=duration,bounds=[low.tolist(),high.tolist()]))
        print(name,'bounds',low.round(3),high.round(3),flush=True)
    (output/'aranea.brm').write_bytes(bank)
    # Original fine ceramic grain and shallow machining marks. All generated
    # analytically; there is no source image or externally sampled texture.
    yy,xx=np.mgrid[0:512,0:512];u=xx/512.;v=yy/512.
    grain=np.sin(xx*1.73+yy*.37)*np.sin(yy*2.41-xx*.23)
    bands=np.sin(u*2*PI*24+np.sin(v*2*PI*3)*.18)
    shade=np.clip(.84+.018*grain+.024*bands,0,1)
    color=np.stack((shade,shade*.985,shade*.955),axis=-1)
    nx=.07*np.cos(u*2*PI*24+np.sin(v*2*PI*3)*.18);ny=.015*np.sin(v*2*PI*3)
    normal=np.stack((nx,ny,np.ones_like(nx)),axis=-1);normal/=np.linalg.norm(normal,axis=-1)[:,:,None]
    for filename,array in [('creature-color.rgb',color),('creature-normal.rgb',normal*.5+.5)]:
        raw=struct.pack('<8sII',b'ARATEX1\0',512,512)+np.round(np.clip(array,0,1)*255).astype('uint8').tobytes()
        (output/filename).write_bytes(raw)
    manifest['files']={name:dict(bytes=(output/name).stat().st_size,sha256=hashlib.sha256((output/name).read_bytes()).hexdigest()) for name in ('aranea.brm','creature-color.rgb','creature-normal.rgb')}
    manifest['generatorSha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    (output/'aranea-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('Generated',vertex_count,'vertices,',len(triangles),'triangles,',len(bank),'bytes.',flush=True)

if __name__=='__main__':main()
