#define MASK_WORDS ((2*ND_DIMENSIONS+31)/32)
#define MASK_STRIDE (rayAxisCount+1+MASK_WORDS)
#extension GL_ARB_bindless_texture : require
in vec2 uv;
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
uniform float aspect;
uniform int rayAxisCount, rayAxes[10];
uniform int lookupMask;
uniform bool directLookup;
uniform int lookupAxes[2], lookupOrigin[3], lookupSize[3];
layout(std430,binding=5) readonly buffer Keys { uint keys[]; };
layout(std430,binding=1) readonly buffer Chunks { uint blocks[]; };
layout(std430,binding=2) readonly buffer Faces { int textures[]; };
layout(std430,binding=3) readonly buffer Tiles { uvec2 tiles[]; };
layout(std430,binding=4) readonly buffer Masks { int masks[]; };
uniform int maskCount, redirector, doorOpen, targetSide;
uniform bool preview;
uniform bool targetInSlice, previewInSlice;
uniform int previewFixedBoundaries;
uniform int sideAxes[2];
int rayCell[10];
float rayPoint[10];
int axisSlot(int axis) {
    for(int j=0;j<rayAxisCount;++j)if(rayAxes[j]==axis)return j;
    return -1;
}
int cellAt(int axis) {
    int slot=axisSlot(axis);
    return slot<0?int(floor(eye[axis])):rayCell[slot];
}
float pointAt(int axis) {
    int slot=axisSlot(axis);
    return slot<0?eye[axis]:rayPoint[slot];
}
// BEGIN PROCEDURAL ND GRAIN
uint grainHash(uint x) {
    x^=x>>16u;x*=0x7feb352du;x^=x>>15u;x*=0x846ca68bu;return x^(x>>16u);
}
uniform int textureAxisCount;
int textureCount(){return textureAxisCount>0?textureAxisCount:ND_DIMENSIONS;}
int textureAxis(int index){return textureAxisCount>0?textureWindowAxes[index]:index;}
uint textureCellHash(int axis,int cell){return grainHash(uint(cell));}
int textureSubcell(int axis,float point,int cell){return int(floor(clamp(point-float(cell),0.,.99999)*16.));}
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


uint blockAt()
{
    if(directLookup) {
        for(int j=0;j<rayAxisCount;++j){int i=rayAxes[j];if(i==0)continue;
            if(i!=lookupAxes[0] && i!=lookupAxes[1] && rayCell[j]!=lookupSlice[i])return 65535u;
        }
        int a=int(floor(float(rayCell[0])/32.));
        ivec3 q=ivec3(a,cellAt(lookupAxes[0]),cellAt(lookupAxes[1]))-ivec3(lookupOrigin[0],lookupOrigin[1],lookupOrigin[2]);
        if(any(lessThan(q,ivec3(0))) || any(greaterThanEqual(q,ivec3(lookupSize[0],lookupSize[1],lookupSize[2]))))return 65535u;
        uint offset=keys[q.x+lookupSize[0]*(q.y+lookupSize[1]*q.z)];
        return offset==0u?65535u:blocks[(offset-1u)*32u+uint(rayCell[0]-a*32)];
    }
    int key[10];uint h=2166136261u;
    for(int j=0;j<rayAxisCount;++j) {
        int i=rayAxes[j];key[j]=i==0?int(floor(float(rayCell[j])/32.)):rayCell[j];
        h=(h^uint(key[j]))*16777619u;
    }
    uint slot=h&uint(lookupMask);
    for(int attempt=0;attempt<=lookupMask;++attempt) {
        uint offset=keys[slot*uint(rayAxisCount+1)+uint(rayAxisCount)];
        if(offset==0u)return 65535u;
        bool same=true;
        for(int i=0;i<rayAxisCount;++i)same=same && keys[slot*uint(rayAxisCount+1)+uint(i)]==uint(key[i]);
        if(same)return blocks[(offset-1u)*32u+uint(rayCell[0]-key[0]*32)];
        slot=(slot+1u)&uint(lookupMask);
    }
    return 65535u;
}
uniform bool sideView;
uniform vec2 sideHalfSize, playerSize;
uniform float sidePhase;
uniform sampler2D playerSprite;
vec4 sideTex(uint id,int side)
{
    uint material=id;
    int tex=textures[int(id)*(2*ND_DIMENSIONS)+side];
    if(id==uint(redirector)) {
        int mask=0;
        for(int m=0;m<maskCount;++m) {
            bool same=true;for(int j=0;j<rayAxisCount;++j)same=same&&masks[m*MASK_STRIDE+j]==rayCell[j];
            if(same){int base=m*MASK_STRIDE+rayAxisCount;mask=masks[base];
                material+=((uint(masks[base+1+side/32])>>(side%32))&1u);break;}
        }
        tex=mask;
    }
    return profileGrain(material,id==uint(redirector)?uint(tex):0u,side);
}
void renderSide()
{
    vec2 offset=(uv*2.-1.)*sideHalfSize;
    int visibleAxis=sideAxes[0],normalAxis=sideAxes[1];
    for(int j=0;j<rayAxisCount;++j){int i=rayAxes[j];rayPoint[j]=eye[i]+right[i]*offset.x+up[i]*offset.y;rayCell[j]=int(floor(rayPoint[j]));}
    color=vec4(0.,0.,0.,1.);
    uint id=blockAt();
    float edge=1e30;
    for(int j=0;j<rayAxisCount;++j) {int i=rayAxes[j];
        float weight=i==0?1.:abs(right[i]);
        if(weight>1e-8)edge=min(edge,min(fract(rayPoint[j]),1.-fract(rayPoint[j]))/weight);
    }
    if(id!=0u && id!=65535u && id!=uint(doorOpen)) {
        int baseSide=normalAxis*2+1;
        vec4 base=sideTex(id,baseSide);
        color.rgb=base.rgb*base.a*.9;
        bool selected=targetInSlice && targetSide>=0;for(int j=0;j<rayAxisCount;++j)selected=selected&&rayCell[j]==target[rayAxes[j]];
        if(selected && edge<.035)color.rgb=mix(color.rgb,vec3(1.),.7);
    }
    if(preview && previewInSlice) {
        bool inside=true;float border=1e30;
        for(int j=0;j<rayAxisCount;++j) {int i=rayAxes[j];
            inside=inside&&rayPoint[j]>=boxMin[i]-.04&&rayPoint[j]<=boxMax[i]+.04;
            float weight=i==0?1.:abs(right[i]);
            if(weight>1e-8)border=min(border,min(abs(rayPoint[j]-boxMin[i]),abs(rayPoint[j]-boxMax[i]))/weight);
        }
        if(inside&&border<.04)color.rgb=vec3(.1,1.,.25);
    }
    vec2 spriteUV=vec2(offset.x/playerSize.x+.5,(.1-offset.y)/playerSize.y);
    if(all(greaterThanEqual(spriteUV,vec2(0.)))&&all(lessThan(spriteUV,vec2(1.)))) {
        vec4 sprite=texture(playerSprite,spriteUV);
        if(all(greaterThan(sprite.rgb,vec3(254.5/255.))))sprite.a=0.;
        color.rgb=mix(color.rgb,sprite.rgb,sprite.a);
    }
}
void main()
{
    if(sideView){renderSide();return;}
    float rd[10], next[10], delta[10]; int step[10];
    float length2=0.;
    for(int j=0;j<rayAxisCount;++j)rayCell[j]=int(floor(eye[rayAxes[j]]));
    for(int j=0;j<rayAxisCount;++j) {int i=rayAxes[j];rd[j]=forward[i]+right[i]*(uv.x*2.-1.)*aspect+up[i]*(uv.y*2.-1.);length2+=rd[j]*rd[j];}
    for(int j=0;j<rayAxisCount;++j) {int i=rayAxes[j];
        rd[j]/=sqrt(length2);step[j]=rd[j]>=0.?1:-1;
        delta[j]=abs(rd[j])<1e-8?1e30:abs(1./rd[j]);
        next[j]=abs(rd[j])<1e-8?1e30:(step[j]>0?1.-fract(eye[i]):fract(eye[i]))*delta[j];
    }
    vec3 sky=vec3(0.);
    color=vec4(sky,1.);
    float distance=0.; int side=0;
    for(int iteration=0;iteration<220 && distance<160.;++iteration) {
        uint id=blockAt();
        if(id==65535u) break;
        if(id!=0u && id!=uint(doorOpen)) {
            for(int j=0;j<rayAxisCount;++j)rayPoint[j]=eye[rayAxes[j]]+rd[j]*distance;
            vec4 sampleColor=sideTex(id,side);
            if(sampleColor.a>.001) {
                float light=side==1?1.:side==0?.5:.72+float(side%3)*.07;
                bool selected=targetInSlice && targetSide==side;for(int j=0;j<rayAxisCount;++j)selected=selected&&rayCell[j]==target[rayAxes[j]];
                if(selected) light+=.15;
                light*=.893;
                color=vec4(mix(sampleColor.rgb*light,sky,clamp(distance/160.,0.,1.)*.3),1.);
                break;
            }
        }
        float nearest=1e30;
        for(int j=0;j<rayAxisCount;++j)nearest=min(nearest,next[j]);
        side=-1;
        for(int j=0;j<rayAxisCount;++j) if(next[j]<=nearest+1e-6) {int i=rayAxes[j];
            rayCell[j]+=step[j];next[j]+=delta[j];if(side<0)side=i*2+(step[j]>0?0:1);
        }
        distance=nearest;
    }
    if(preview && previewInSlice) {
        float enter=-1e30, leave=1e30;
        for(int j=0;j<rayAxisCount;++j) {int i=rayAxes[j];
            float direction=rd[j];
            if(abs(direction)<1e-8) {if(eye[i]<boxMin[i] || eye[i]>boxMax[i]) leave=-1e30;}
            else {float a=(boxMin[i]-eye[i])/direction, b=(boxMax[i]-eye[i])/direction; enter=max(enter,min(a,b)); leave=min(leave,max(a,b));}
        }
        float t=enter>=0.?enter:leave;
        if(t>=0. && leave>=max(enter,0.) && t<=distance+.03) {
            int boundaries=previewFixedBoundaries;
            for(int j=0;j<rayAxisCount;++j) {int i=rayAxes[j];
                float v=eye[i]+rd[j]*t;
                if(min(abs(v-boxMin[i]),abs(v-boxMax[i]))<.045)++boundaries;
            }
            if(boundaries>=2) color=vec4(.1,1.,.25,1.);
        }
    }
}
