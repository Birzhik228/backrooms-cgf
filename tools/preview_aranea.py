"""Offline preview of the exact baked locomotion bank; Blender background only."""
import bpy,os,struct,math
from mathutils import Vector
root=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
out=os.path.join(root,'tests/artifacts/aranea/model');os.makedirs(out,exist_ok=True)
data=open(os.path.join(root,'assets/models/aranea/aranea.brm'),'rb').read()
version,nv,nt,nc,pu,nu=struct.unpack_from('<IIIIff',data,8)
assert version==3
offset=32;descriptors=[]
for _ in range(nc):
 name,frames,duration,loop=struct.unpack_from('<16sIfI',data,offset);offset+=28
 descriptors.append((name.rstrip(b'\0').decode(),frames,duration,loop))
topology=[struct.unpack_from('<IIIB3x',data,offset+i*16) for i in range(nt)]
offset+=nt*16;masks=[tuple(data[offset+i*13:offset+i*13+5]) for i in range(nv)];offset+=nv*13
clips=[]
for name,frames,duration,loop in descriptors:
 poses=[]
 for frame in range(frames):
  px,py,pz,flags,pad=struct.unpack_from('<hhhBB',data,offset);offset+=8
  values=struct.unpack_from('<'+'h'*(nv*6),data,offset);offset+=nv*12
  poses.append(([(values[i*6]*pu,values[i*6+1]*pu,values[i*6+2]*pu) for i in range(nv)],(px*pu,py*pu,pz*pu)))
 clips.append(poses)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene;scene.render.engine='BLENDER_WORKBENCH'
scene.display.shading.light='STUDIO';scene.display.shading.color_type='MATERIAL'
scene.display.shading.show_shadows=True;scene.display.shading.show_cavity=True
scene.display.shading.background_type='WORLD';scene.world=bpy.data.worlds.new('Preview');scene.world.color=(.14,.16,.18)
scene.render.resolution_x=440;scene.render.resolution_y=480;scene.render.resolution_percentage=100
mesh=bpy.data.meshes.new('Aranea cache');obj=bpy.data.objects.new('Aranea',mesh);scene.collection.objects.link(obj)
for color in [(.48,.47,.46,1),(.87,.86,.84,1),(.023,.024,.026,1)]:
 m=bpy.data.materials.new('Skin' if color[0]<.1 else 'Eyes and teeth');m.diffuse_color=color;mesh.materials.append(m)
bpy.ops.mesh.primitive_plane_add(size=18);bpy.context.object.location.z=0
floor=bpy.data.materials.new('Floor');floor.diffuse_color=(.22,.23,.20,1);bpy.context.object.data.materials.append(floor)
bpy.ops.object.camera_add(location=(4.2,7,3.2));cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=4.5
cam.rotation_euler=(Vector((0,0,1.4))-cam.location).to_track_quat('-Z','Y').to_euler()
for name,index,phase,yaw in [('walk0',1,.1,0),('walk1',1,.35,0),('walk2',1,.6,0),('walk3',1,.85,0),
                           ('run0',2,0,0),('run1',2,.25,0),('run2',2,.5,0),('run3',2,.75,0),
                           ('head-left',0,0,-.8),('head-right',0,0,.8),('duck',5,.25,0),('capture',6,.6,0)]:
 ps,pivot=clips[index][round(phase*len(clips[index]))%len(clips[index])]
 positions=[]
 for i,(x,y,z) in enumerate(ps):
  a=yaw*masks[i][0]/255;dx=x-pivot[0];dz=z-pivot[2]
  x=pivot[0]+dx*math.cos(a)-dz*math.sin(a);z=pivot[2]+dx*math.sin(a)+dz*math.cos(a)
  positions.append((x,-z,y))
 mesh.clear_geometry();mesh.from_pydata(positions,[],[(a,b,c) for a,b,c,m in topology])
 for poly,tri in zip(mesh.polygons,topology):poly.material_index=tri[3]-20;poly.use_smooth=True
 mesh.update();scene.render.filepath=os.path.join(out,name+'.png');bpy.ops.render.render(write_still=True)
