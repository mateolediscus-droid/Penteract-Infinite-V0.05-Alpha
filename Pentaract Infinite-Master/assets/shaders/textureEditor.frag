#version 450 core
in vec3 position;flat in int face;out vec4 color;
uniform int worldDimensions=5;uniform int blockId;uniform int xAxis=1;uniform int zAxis=2;
uniform float wSlice;uniform float vSlice;
uniform int selectedRegion=-1;uniform int selectedFaces=0;
uniform int highlightFace=-1;uniform int highlightSlice=-1;uniform int highlightMask[8];
// BEGIN PROCEDURAL ND GRAIN
uint grainHash(uint x) {
    x^=x>>16u;x*=0x7feb352du;x^=x>>15u;x*=0x846ca68bu;return x^(x>>16u);
}
vec4 proceduralGrain(float point[5],int cell[5],uint material,uint state) {
    uint h=grainHash(material^grainHash(state+0x51ed270bu));
    for(int axis=0;axis<worldDimensions;++axis) {
        // Quantize locally to avoid multiplying large world coordinates by 16.
        int sub=int(floor(clamp(point[axis]-float(cell[axis]),0.,.99999)*16.));
        h=grainHash(h^grainHash(uint(cell[axis]))^grainHash(uint(axis)+0x9e3779b9u));
        h=grainHash(h^uint(sub));
    }
    vec3 rgb=vec3(grainHash(h^0x68bc21ebu),grainHash(h^0x02e5be93u),grainHash(h^0x967a889bu));
    return vec4(.12+.88*(rgb/4294967295.),1.);
}


// BEGIN TEXTURE PROFILE
layout(std430,binding=6) readonly buffer TextureProfiles {uint profileData[];};
vec4 profileGrain(float point[5],int cell[5],uint material,uint state,int side) {
    if(material>=uint(profileData.length()))return proceduralGrain(point,cell,material,state);
    uint base=profileData[material];
    if(base==0u)return proceduralGrain(point,cell,material,state);
    int face=side<10?side:8+(side%2),normal=face/2;
    uint options=base+174u+uint(face*3);int count=int(profileData[base+164u+uint(face)]);
    float local[5];uint h=grainHash(material^profileData[options]);
    for(int a=0;a<worldDimensions;++a){
        local[a]=clamp(point[a]-float(cell[a]),0.,.99999);
        h=grainHash(h^grainHash(uint(cell[a]))^grainHash(uint(a)+0x9e3779b9u));
        h=grainHash(h^uint(floor(local[a]*16.)));
    }
    for(int a=worldDimensions;a<5;++a)local[a]=.5;
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
        uint shapeHash=grainHash(material^profileData[options]);
        for(int axis=0;axis<worldDimensions;++axis){
            shapeHash=grainHash(shapeHash^grainHash(uint(cell[axis])));
            if(axis!=u&&axis!=v&&axis!=normal)shapeHash=grainHash(shapeHash^uint(floor(local[axis]*16.)));}
        uint tile=profileData[base+72054u+uint(face*16+s0)];
        if(tile==0u){
            float r=min(float(h)/4294967295.,.99999994)*total;
            for(int c=0;c<count;++c){r-=weights[c];if(r<0.){chosen=c;break;}}
        }else{
            chosen=int(profileData[base+tile+(shapeHash%4u)*256u+uint(y*16+x)]);
        }
    }
    uint color=profileData[base+4u+uint(face*16+chosen)];
    float alpha=profileData[base+71804u+uint(face*16+chosen)]!=0u?0.:1.;
    return vec4(vec3(color&255u,(color>>8u)&255u,(color>>16u)&255u)/255.,alpha);
}
// END TEXTURE PROFILE

void main(){float p[5];int c[5];for(int i=0;i<5;++i){p[i]=.5;c[i]=0;}
p[3]=wSlice;p[4]=vSlice;p[0]=position.y;p[xAxis]=position.x;p[zAxis]=position.z;
color=profileGrain(p,c,uint(blockId),0u,face);color.rgb*=face/2==0?1.:.83;
if(selectedRegion>=0&&(selectedFaces&(1<<face))!=0){uint base=profileData[blockId];uint r=base+44204u+uint((face*8+selectedRegion)*309);
 if(base!=0u&&profileData[r]!=0u){int normal=face/2,u=normal==0?1:0,v=(normal==0||normal==1)?2:1;
 vec2 uv=vec2(p[u],p[v]),lo=vec2(profileData[r+1u],profileData[r+2u])/16.,hi=vec2(profileData[r+3u],profileData[r+4u])/16.;
 if(all(greaterThanEqual(uv,lo))&&all(lessThanEqual(uv,hi))&&min(min(uv.x-lo.x,hi.x-uv.x),min(uv.y-lo.y,hi.y-uv.y))<.012)color=vec4(1.,.8,.05,1.);}}
if(face==highlightFace&&(highlightSlice<0||clamp(int(p[3]*16.),0,15)==highlightSlice)){
 int normal=face/2,u=normal==0?1:0,v=(normal==0||normal==1)?2:1;
 int index=clamp(int(p[v]*16.),0,15)*16+clamp(int(p[u]*16.),0,15);
 if((uint(highlightMask[index/32])&(1u<<uint(index%32)))!=0u)color=vec4(mix(color.rgb,vec3(1.,.8,.05),.55),1.);}
if(color.a<=.001)discard;
}
