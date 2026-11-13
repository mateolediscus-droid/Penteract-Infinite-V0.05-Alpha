#pragma once
#include <GL/glew.h>
#include <algorithm>

// Single-sample world target; HUD and menus are drawn after restoring the output.
class WorldRenderResolution {
    GLuint framebuffer=0,color=0,depth=0;
    int width=0,height=0;
    GLint output=0,readOutput=0,viewport[4]{};
    bool active=false;
public:
    int percent=100;
    void cleanup() {
        if(framebuffer)glDeleteFramebuffers(1,&framebuffer);
        if(color)glDeleteRenderbuffers(1,&color);
        if(depth)glDeleteRenderbuffers(1,&depth);
        framebuffer=color=depth=0;width=height=0;active=false;
    }
    bool begin(int w,int h) {
        if(percent>=100 || w<=0 || h<=0)return true;
        int rw=std::max(1,w*percent/100),rh=std::max(1,h*percent/100);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&output);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readOutput);
        glGetIntegerv(GL_VIEWPORT,viewport);
        if(rw!=width || rh!=height) {
            cleanup();width=rw;height=rh;
            glCreateFramebuffers(1,&framebuffer);
            glCreateRenderbuffers(1,&color);glNamedRenderbufferStorage(color,GL_RGBA8,width,height);
            glCreateRenderbuffers(1,&depth);glNamedRenderbufferStorage(depth,GL_DEPTH_COMPONENT24,width,height);
            glNamedFramebufferRenderbuffer(framebuffer,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color);
            glNamedFramebufferRenderbuffer(framebuffer,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
            if(glCheckNamedFramebufferStatus(framebuffer,GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){cleanup();return false;}
        }
        glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);glViewport(0,0,width,height);active=true;return true;
    }
    void finish() {
        if(!active)return;
        glBlitNamedFramebuffer(framebuffer,output,0,0,width,height,viewport[0],viewport[1],
            viewport[0]+viewport[2],viewport[1]+viewport[3],GL_COLOR_BUFFER_BIT,GL_LINEAR);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,output);glBindFramebuffer(GL_READ_FRAMEBUFFER,readOutput);
        glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);active=false;
    }
};
