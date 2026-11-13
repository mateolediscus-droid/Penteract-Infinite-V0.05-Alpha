
    struct CameraFrame
    {
        CameraFrame(){align({1,2});}
        std::array<Vector,(N-1)> axes{};
        void align(std::array<int,2> plane)
        {
            for(auto& row:axes)row.fill(0);
            axes[0][plane[0]]=1; axes[1][plane[1]]=1;
            int next=2;
            for(int i=1;i<N;++i) if(i!=plane[0] && i!=plane[1]) axes[next++][i]=1;
        }
        Vector heading(double yaw) const
        {
            Vector v{};
            for(int i=1;i<N;++i) v[i]=axes[0][i]*std::cos(yaw)+axes[1][i]*std::sin(yaw);
            return v;
        }
        Vector right(double yaw) const { return heading(yaw+1.5707963267948966); }
        void rotate(Vector a,Vector b,double angle)
        {
            for(auto& v:axes)
            {
                double x=0,y=0;
                for(int i=1;i<N;++i) {x+=v[i]*a[i];y+=v[i]*b[i];}
                double dx=x*(std::cos(angle)-1)-y*std::sin(angle);
                double dy=x*std::sin(angle)+y*(std::cos(angle)-1);
                for(int i=1;i<N;++i) v[i]+=dx*a[i]+dy*b[i];
            }
        }
        void turnHidden(double yaw,int hidden,double dx,double dy)
        {
            rotate(right(yaw),axes[hidden],dx);
            rotate(heading(yaw),axes[hidden],dy);
        }
    };

    static inline bool bodyOverlaps(const Vector& eye,const Position& block,const CameraFrame& frame=CameraFrame{})
    {
        if(eye[0]+.1<=block[0] || eye[0]-1.65>=double(block[0])+1)return false;
        return SliceColumns::intersects(eye,frame.axes[0],frame.axes[1],block,.25);
    }
    static inline std::vector<Position> collisionChunks(const Vector& eye,const CameraFrame& frame=CameraFrame{})
    {
        auto columns=SliceColumns::enumerate(eye,frame.axes[0],frame.axes[1],1.25);
        std::set<Position> wanted;
        int low=int(std::floor((eye[0]-2.65)/32.)),high=int(std::floor((eye[0]+1.1)/32.));
        for(auto p:columns)for(int a=low;a<=high;++a) {
            p[0]=a;wanted.insert(p);
            for(int axis=1;axis<N;++axis)for(int sign:{-1,1}){auto n=p;n[axis]+=sign;wanted.insert(n);}
        }
        return {wanted.begin(),wanted.end()};
    }
    static inline Vector abovePlacedBlock(Vector eye,const Position& block,const CameraFrame& frame=CameraFrame{})
    {
        if(bodyOverlaps(eye,block,frame))eye[0]=double(block[0])+2.6501;
        return eye;
    }

