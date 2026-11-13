#define MASK_WORDS ((2*ND_DIMENSIONS+31)/32)
#define TEXTURE_MESH
#define MASK_STRIDE (rayAxisCount+1+MASK_WORDS)
#extension GL_ARB_bindless_texture : require
in vec3 meshPosition;
flat in ivec3 meshCell;
flat in uvec2 meshMaterial;
out vec4 color;
layout(std430,binding=7) readonly buffer NDFrameData {
    float eye[ND_DIMENSIONS], forward[ND_DIMENSIONS], right[ND_DIMENSIONS], up[ND_DIMENSIONS];
    int textureExtraAxes[ND_DIMENSIONS-5];
    int lookupSlice[ND_DIMENSIONS], target[ND_DIMENSIONS];
    float boxMin[ND_DIMENSIONS], boxMax[ND_DIMENSIONS];
    int textureWindowAxes[ND_DIMENSIONS];
    uint textureCachedCell[ND_DIMENSIONS];
    int textureCachedSub[ND_DIMENSIONS];
};
uniform int meshAxes[2], targetSide, maskCount, redirector;
uniform bool lineMode;
uniform int rayAxisCount, rayAxes[10];
uniform bool targetInSlice;
float pointAt(int axis) {
    if(axis==0)return meshPosition.x;
    if(axis==meshAxes[0])return meshPosition.y;
    if(axis==meshAxes[1])return meshPosition.z;
    return eye[axis];
}
int cellAt(int axis) {
    if(axis==0)return meshCell.x;
    if(axis==meshAxes[0])return meshCell.y;
    if(axis==meshAxes[1])return meshCell.z;
    return int(floor(eye[axis]));
}
layout(std430,binding=2) readonly buffer Faces { int textures[]; };
layout(std430,binding=3) readonly buffer Tiles { uvec2 tiles[]; };
layout(std430,binding=4) readonly buffer Masks { int masks[]; };
// BEGIN PROCEDURAL ND GRAIN
uint grainHash(uint x) {
    x^=x>>16u;x*=0x7feb352du;x^=x>>15u;x*=0x846ca68bu;return x^(x>>16u);
}
uniform int textureAxisCount;
uniform bool textureCacheEnabled;
int textureCount(){return textureAxisCount>0?textureAxisCount:ND_DIMENSIONS;}
int textureAxis(int index){return textureAxisCount>0?textureWindowAxes[index]:index;}
uint textureCellHash(int axis,int cell){
    if(textureCacheEnabled && axis!=0 && axis!=meshAxes[0] && axis!=meshAxes[1])return textureCachedCell[axis];
    return grainHash(uint(cell));
}
int textureSubcell(int axis,float point,int cell){
    if(textureCacheEnabled && axis!=0 && axis!=meshAxes[0] && axis!=meshAxes[1])return textureCachedSub[axis];
    return int(floor(clamp(point-float(cell),0.,.99999)*16.));
}
vec4 proceduralGrain(uint material,uint state) {
    uint h=grainHash(material^grainHash(state+0x51ed270bu));
    for(int index=0;index<textureCount();++index) {
        int axis=textureAxis(index);
        // Quantize locally to avoid multiplying large world coordinates by 16.
        int sub=textureSubcell(axis,pointAt(axis),cellAt(axis));
        h=grainHash(h^textureCellHash(axis,cellAt(axis))^grainHash(uint(axis)+0x9e3779b9u));
        h=grainHash(h^uint(sub));
    }
    vec3 rgb=vec3(grainHash(h^0x68bc21ebu),grainHash(h^0x02e5be93u),grainHash(h^0x967a889bu));
    return vec4(.12+.88*(rgb/4294967295.),1.);
}
// END PROCEDURAL ND GRAIN
// BEGIN TEXTURE PROFILE
layout(std430,binding=6) readonly buffer TextureProfiles {uint profileData[];};
uniform int profileAxes[5]=int[5](0,1,2,3,4);
vec4 profileGrain(uint material,uint state,int side) {
    if(material>=uint(profileData.length()))return proceduralGrain(material,state);
    uint base=profileData[material];
    if(base==0u)return proceduralGrain(material,state);
    int normal=4;
    for(int a=0;a<5;++a)if(profileAxes[a]==side/2)normal=a;
    int face=normal*2+(side%2);
    uint options=base+174u+uint(face*3);int count=int(profileData[base+164u+uint(face)]);
    float local[5];
    for(int a=0;a<5;++a)local[a]=clamp(pointAt(profileAxes[a])-float(cellAt(profileAxes[a])),0.,.99999);
    
    int u=normal==0?1:0,v=(normal==0||normal==1)?2:1;
    int x=clamp(int(local[u]*16.),0,15),y=clamp(int(local[v]*16.),0,15);
    // W is authored; V progressively shifts that distribution by up to one period.
    // Slice 8 is the central section at exactly W=0.5.
    float w=clamp(local[3]*16.,0.,15.);
    float derived=clamp(w+(local[4]-.5)*15.*float(profileData[options+1u])/100.,0.,15.);
    int s0=int(floor(derived)),s1=min(15,s0+1);
    float blend=profileData[options+2u]!=0u?fract(derived):0.;
    int paintedSlice=clamp(int(local[3]*16.),0,15);
    uint pin=profileData[base+2764u+uint((face*16+paintedSlice)*256+y*16+x)];
    int chosen=0;
    if(pin>0u)chosen=int(pin)-1;
    else {
        uint weightBase=base+204u+uint(face*256),biasBase=base+43724u+uint(face*48);
        int regionSlot=0;
        // Last matching region wins; forced pixels were handled above.
        for(int zone=0;zone<8;++zone){uint r=base+44204u+uint((face*8+zone)*309);
            if(profileData[r]!=0u&&x>=int(profileData[r+1u])&&y>=int(profileData[r+2u])&&x<int(profileData[r+3u])&&y<int(profileData[r+4u])){weightBase=r+5u;biasBase=r+261u;regionSlot=zone+1;}}
        float weights[16];float total=0.;
        for(int c=0;c<count;++c){weights[c]=mix(float(profileData[weightBase+uint(s0*16+c)]),float(profileData[weightBase+uint(s1*16+c)]),blend);
            for(int axis=0;axis<3;++axis){int coord=axis==0?1:axis==1?0:2;if(coord==normal)continue;
                float preference=clamp(float(profileData[biasBase+uint(c*3+axis)])/100.,.01,.99);
                float t=(floor(local[coord]*16.)+.5)/16.;weights[c]*=mix(2.*(1.-preference),2.*preference,smoothstep(.45,.55,t));}
            total+=weights[c];}
        uint tile=profileData[base+72054u+uint(face*16+s0)];
        if(tile==0u){
            uint h=grainHash(material^profileData[options]);
            for(int index=0;index<textureCount();++index){int a=textureAxis(index);
                h=grainHash(h^textureCellHash(a,cellAt(a))^grainHash(uint(a)+0x9e3779b9u));
                h=grainHash(h^uint(textureSubcell(a,pointAt(a),cellAt(a))));}
            float r=min(float(h)/4294967295.,.99999994)*total;
            for(int c=0;c<count;++c){r-=weights[c];if(r<0.){chosen=c;break;}}
        }else{
            uint shapeHash=grainHash(material^profileData[options]);
            for(int index=0;index<textureCount();++index){int axis=textureAxis(index);
                shapeHash=grainHash(shapeHash^textureCellHash(axis,cellAt(axis)));
                if(axis!=profileAxes[u]&&axis!=profileAxes[v]&&axis!=side/2)shapeHash=grainHash(shapeHash^uint(textureSubcell(axis,pointAt(axis),cellAt(axis))));}
            chosen=int(profileData[base+tile+(shapeHash%4u)*256u+uint(y*16+x)]);
        }
    }
    uint color=profileData[base+4u+uint(face*16+chosen)];
    float alpha=profileData[base+71804u+uint(face*16+chosen)]!=0u?0.:1.;
    return vec4(vec3(color&255u,(color>>8u)&255u,(color>>16u)&255u)/255.,alpha);
}
// END TEXTURE PROFILE



void main() {
    if(lineMode){color=vec4(.1,1.,.25,1.);return;}
    int side=int(meshMaterial.y),id=int(meshMaterial.x);
    float distance2=0.;
    if((side%2==0?-1.:1.)*(eye[side/2]-pointAt(side/2))<0.)discard;
    for(int k=0;k<3;++k){int i=k==0?0:meshAxes[k-1];float delta=pointAt(i)-eye[i];distance2+=delta*delta;}
    float distance=sqrt(distance2);if(distance>=160.)discard;
    uint material=uint(id);
    int tex=textures[id*(2*ND_DIMENSIONS)+side];
    if(id==redirector) {
        int mask=0;
        for(int m=0;m<maskCount;++m) {
            bool same=true;for(int j=0;j<rayAxisCount;++j)same=same && masks[m*MASK_STRIDE+j]==cellAt(rayAxes[j]);
            if(same){int base=m*MASK_STRIDE+rayAxisCount;mask=masks[base];
                material+=((uint(masks[base+1+side/32])>>(side%32))&1u);break;}
        }
        tex=mask;
    }
    vec4 sampleColor=profileGrain(material,id==redirector?uint(tex):0u,side);
    if(sampleColor.a<=.001)discard;
    float light=side==1?1.:side==0?.5:.72+float(side%3)*.07;
    bool selected=targetInSlice && targetSide==side;
    for(int j=0;j<rayAxisCount;++j)selected=selected && cellAt(rayAxes[j])==target[rayAxes[j]];
    if(selected)light+=.15;
    light*=.893;
    color=vec4(sampleColor.rgb*light*(1.-clamp(distance/160.,0.,1.)*.3),1.);
}
