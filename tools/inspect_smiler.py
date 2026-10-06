import bpy, json, sys, os
from mathutils import Vector
bpy.ops.wm.read_factory_settings(use_empty=True)
base=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
bpy.ops.import_scene.fbx(filepath=os.path.join(base,'assets/models/smiler/source/Zombie Scream.fbx'),use_anim=True)
data={'objects':[], 'materials':[], 'actions':[], 'images':[]}
for obj in bpy.context.scene.objects:
    item={'name':obj.name,'type':obj.type,'location':list(obj.location),'scale':list(obj.scale),'rotation':list(obj.rotation_euler),'bounds':[list(obj.matrix_world@Vector(v)) for v in obj.bound_box]}
    if obj.type=='MESH':
        obj.data.calc_loop_triangles()
        item.update(vertices=len(obj.data.vertices),triangles=len(obj.data.loop_triangles),materials=[m.name if m else None for m in obj.data.materials],modifiers=[m.type for m in obj.modifiers])
    if obj.type=='ARMATURE': item['bones']=[{'name':b.name,'head':list(b.head_local),'tail':list(b.tail_local),'parent':b.parent.name if b.parent else None} for b in obj.data.bones]
    data['objects'].append(item)
for m in bpy.data.materials:
    data['materials'].append({'name':m.name,'diffuse':list(m.diffuse_color),'nodes':[{'name':n.name,'type':n.type,'image':n.image.name if n.type=='TEX_IMAGE' and n.image else None} for n in m.node_tree.nodes] if m.node_tree else []})
for a in bpy.data.actions:data['actions'].append({'name':a.name,'range':list(a.frame_range),'fcurves':[c.data_path for c in a.fcurves]})
for i in bpy.data.images:data['images'].append({'name':i.name,'path':i.filepath,'packed':bool(i.packed_file),'size':list(i.size)})
out=os.path.join(base,'tests/artifacts/v12/model/import-inspection.json')
os.makedirs(os.path.dirname(out),exist_ok=True)
with open(out,'w') as f:json.dump(data,f,indent=2)
print(json.dumps(data,indent=2))
