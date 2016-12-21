#include "arwidget.hh"

//#include <GL/glew.h>
//#include <GLFW/glfw3.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glu.h>

#include <GL/osmesa.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define ai_real GLfloat
#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

struct ARWidgetPrivate{
    const aiScene* scene;
    GLuint scene_list;
    aiVector3D scene_min;
    aiVector3D scene_max;
    aiVector3D scene_center;

    void *mBuffer;
    OSMesaContext mContext;
    int w;
    int h;
    int x;int y;int z;
    int x1;int y1;int z1;
};

static void get_bounding_box_for_node (const aiScene *sc,const  aiNode* nd, aiVector3D* min, aiVector3D* max, aiMatrix4x4* trafo)
{
    aiMatrix4x4 prev;
    unsigned int n = 0, t;

    prev = *trafo;
    aiMultiplyMatrix4(trafo, &nd->mTransformation);

    for (; n < nd->mNumMeshes; ++n) {
        const  aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];
        for (t = 0; t < mesh->mNumVertices; ++t) {

            aiVector3D tmp = mesh->mVertices[t];
            aiTransformVecByMatrix4(&tmp, trafo);

            min->x = aisgl_min(min->x, tmp.x);
            min->y = aisgl_min(min->y, tmp.y);
            min->z = aisgl_min(min->z, tmp.z);

            max->x = aisgl_max(max->x, tmp.x);
            max->y = aisgl_max(max->y, tmp.y);
            max->z = aisgl_max(max->z, tmp.z);
        }
    }

    for (n = 0; n < nd->mNumChildren; ++n) {
        get_bounding_box_for_node(sc,nd->mChildren[n], min, max, trafo);
    }
    *trafo = prev;
}

static void get_bounding_box (const  aiScene *sc,aiVector3D* min,  aiVector3D* max)
{
    aiMatrix4x4 trafo;
    aiIdentityMatrix4(&trafo);

    min->x = min->y = min->z =  1e10f;
    max->x = max->y = max->z = -1e10f;
    get_bounding_box_for_node(sc,sc->mRootNode, min, max, &trafo);
}

static void color4_to_float4(const  aiColor4D *c, float f[4])
{
    f[0] = c->r;
    f[1] = c->g;
    f[2] = c->b;
    f[3] = c->a;
}

static void set_float4(float f[4], float a, float b, float c, float d)
{
    f[0] = a;
    f[1] = b;
    f[2] = c;
    f[3] = d;
}

static void apply_material(const  aiMaterial *mtl)
{
    float c[4];

    GLenum fill_mode;
    int ret1, ret2;
    aiColor4D diffuse;
    aiColor4D specular;
    aiColor4D ambient;
    aiColor4D emission;
    ai_real shininess, strength;
    int two_sided;
    int wireframe;
    unsigned int max;

    set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
    if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
        color4_to_float4(&diffuse, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
        color4_to_float4(&specular, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

    set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
    if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
        color4_to_float4(&ambient, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
        color4_to_float4(&emission, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

    max = 1;
    ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
    if (ret1 == AI_SUCCESS) {
        max = 1;
        ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
        if (ret2 == AI_SUCCESS)
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
        else
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    }
    else {
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
        set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
    }

    max = 1;
    if (AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
        fill_mode = wireframe ? GL_LINE : GL_FILL;
    else
        fill_mode = GL_FILL;
    glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

    max = 1;
    if ((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
        glDisable(GL_CULL_FACE);
    else
        glEnable(GL_CULL_FACE);
    
    
    //    printf("%s:%d\t%s\n",__FUNCTION__,__LINE__,glGetString(GL_VERSION));

}

static void recursive_render (const  aiScene *sc, const  aiNode* nd,int depth)
{
    aiMatrix4x4 m = nd->mTransformation;

    aiTransposeMatrix4(&m);
    glPushMatrix();
    glMultMatrixf((float*)&m);
//    glScalef(.25,.25,.25);

    int cnt =0;
    if(strcmp(nd->mName.C_Str(),"g 1:inner arrow")==0){
        cnt =1;
    } else if(strcmp(nd->mName.C_Str(),"g 2:outer arrow")==0){
        cnt =30;
    }

    for(int j=0;j<cnt;++j){
        glTranslatef(0,0,-50);

        for (int n = 0; n < nd->mNumMeshes; ++n) {
            const  aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];
            apply_material(sc->mMaterials[mesh->mMaterialIndex]);

            if (mesh->mNormals == NULL) {
                glDisable(GL_LIGHTING);
            } else {
                glEnable(GL_LIGHTING);
            }

            for (int t = 0; t < mesh->mNumFaces; ++t) {
                const  aiFace* face = &mesh->mFaces[t];
                GLenum face_mode;

                switch (face->mNumIndices) {
                case 1: face_mode = GL_POINTS; break;
                case 2: face_mode = GL_LINES; break;
                case 3: face_mode = GL_TRIANGLES; break;
                default: face_mode = GL_POLYGON; break;
                }

                glBegin(face_mode);

                for (int i = 0; i < face->mNumIndices; i++) {
                    int index = face->mIndices[i];

                    if (mesh->mColors[0] != NULL)
                        glColor4fv((GLfloat*)&mesh->mColors[0][index]);
                    if (mesh->mNormals != NULL)
                        glNormal3fv(&mesh->mNormals[index].x);
                    glVertex3fv(&mesh->mVertices[index].x);
                }

                glEnd();
            }
        }
    }

    for (int n = 0; n < nd->mNumChildren; ++n) {
        recursive_render(sc, nd->mChildren[n],depth+1);
    }

    glPopMatrix();
}

void ARWidget::overlayImage(cv::Mat& src, const cv::Mat &overlay, const cv::Point& location)
{
    for (int y = aisgl_max(location.y, 0); y < src.rows; ++y)
    {
        int fY = y - location.y;

        if (fY >= overlay.rows)
            break;

        for (int x = aisgl_max(location.x, 0); x < src.cols; ++x)
        {
            int fX = x - location.x;

            if (fX >= overlay.cols)
                break;

            double opacity = ((double)overlay.data[fY * overlay.step + fX * overlay.channels() + 3]) / 255;

            for (int c = 0; opacity > 0 && c < src.channels(); ++c)
            {
                unsigned char overlayPx = overlay.data[fY * overlay.step + fX * overlay.channels() + c];
                unsigned char srcPx = src.data[y * src.step + x * src.channels() + c];
                src.data[y * src.step + src.channels() * x + c] = srcPx * (1. - opacity) + overlayPx * opacity;
            }
        }
    }
}

static void init(int width,int height)  {
    GLuint fbo;
    GLuint rbo_color;
    GLuint rbo_depth;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenRenderbuffers(1, &rbo_color);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, width, height);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo_color);

    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        printf("success !\n");
    } else {
        printf("error!\n");
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);

}

ARWidget::ARWidget()
{
    p = new ARWidgetPrivate;
    p->scene_list = 0;
    p->scene = NULL;
}

ARWidget::~ARWidget()
{
    OSMesaDestroyContext( p->mContext );

    free(p->mBuffer);
    aiReleaseImport(p->scene);

    delete p;
}

int ARWidget::initGL(int width, int height)
{
    if(width<=0||height<=0||width>10000||height>10000)
        return 2;

    p->w = width;
    p->h = height;

    //    glfwInit();
    //    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    //    glfwWindowHint(GLFW_SAMPLES, 4);
    //    GLFWwindow* window = glfwCreateWindow(width, height, "", nullptr, nullptr);
    //    glfwMakeContextCurrent(window);
    //    if (glewInit() != GLEW_OK) {
    //        fprintf(stderr, "Failed to initialize GLEW\n");
    //        return 1;
    //    }


    p->mBuffer  = (GLfloat *) malloc( width * height * 4 * sizeof(GLfloat));
    p->mContext = OSMesaCreateContextExt(OSMESA_RGBA, 0, 0, 0, NULL);
    if(GL_TRUE!=OSMesaMakeCurrent(p->mContext, p->mBuffer, GL_UNSIGNED_BYTE, width, height))
        return 1;

    init(width,height);
    printf("%s:%d\t%s\n",__FUNCTION__,__LINE__,glGetString(GL_VERSION));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    glClearColor(0,0,0,.5);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(53, (float) p->w / p->h, .1, 1000.0);
    glViewport(0, 0, p->w, p->h);

    GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat mat_shininess[] = { 50.0 };
    GLfloat light_position[] = { 0.0, 5.0, -1.0, 1.0 };

    glShadeModel (GL_SMOOTH);

    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_NORMALIZE);
    if (getenv("MODEL_IS_BROKEN"))
        glFrontFace(GL_CW);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

    //    p->x = 0;
    //    p->y = 15;
    //    p->z = 45;

    return 0;
}

int ARWidget::loadModel(const std::string &filename)
{
    p->scene = aiImportFile(filename.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
    if (p->scene) {
        get_bounding_box(p->scene,&p->scene_min, &p->scene_max);
        p->scene_center.x = (p->scene_min.x + p->scene_max.x) / 2.0f;
        p->scene_center.y = (p->scene_min.y + p->scene_max.y) / 2.0f;
        p->scene_center.z = (p->scene_min.z + p->scene_max.z) / 2.0f;

        return 0;
    }
    return 1;
}

int ARWidget::render(cv::Mat &mat, const cv::Point &pos, double yaw, double pitch)
{
    if(GL_TRUE!=OSMesaMakeCurrent(p->mContext, p->mBuffer, GL_UNSIGNED_BYTE, p->w, p->h))
        return 1;

    float tmp;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    //    glMatrixMode(GL_PROJECTION);
    //    glLoadIdentity();
    //    gluPerspective(p->x, (float) p->w / p->h, 1.0, p->y);
    //    glViewport(0, 0, p->w, p->h);


    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //    glClearColor(0,0,0,.5);
    gluLookAt(0,3.7f,3.7f,0,1.8f,1.8f, 0.f, 1.f, 0.f);
    //    gluLookAt(.1*p->x,.1*p->y,.1*p->z, .1*p->x1,.1*p->y1,.1*p->z1, 0.f, 1.f, 0.f);
    //    gluLookAt(0.f, 1.f, 2.f, 0.f, 0.f, -5.f, 0.f, 1.f, 0.f);
    //    gluLookAt(0.f, 1.5f, 4.5f, 0.f, 0.f, -10.f, 0.f, 1.f, 0.f);

    glRotatef(-yaw, 0.f, 1.f, 0.f);
    glRotatef(-pitch, 1.f, 0.f, 0.f);

    //    glRotatef(90, 0.f, 0.f, 1.f);
    //    glRotatef(90, 0.f, 1.f, 0.f);

    tmp = p->scene_max.x - p->scene_min.x;
    tmp = aisgl_max(p->scene_max.y - p->scene_min.y, tmp);
    tmp = aisgl_max(p->scene_max.z - p->scene_min.z, tmp);
    tmp = 1.f / tmp;
    glScalef(tmp, tmp, tmp);

    glTranslatef( -p->scene_center.x, -p->scene_center.y, -p->scene_center.z );

    if (p->scene_list == 0) {
        p->scene_list = glGenLists(1);
        glNewList(p->scene_list, GL_COMPILE);
        recursive_render(p->scene, p->scene->mRootNode,0);
        glEndList();
    }

    glCallList(p->scene_list);
    glFlush();

    cv::Mat img(p->h,p->w, CV_8UC4);
    //glReadBuffer( GL_COLOR_ATTACHMENT0 );
    glReadPixels(0, 0, p->w, p->h, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
    cv::flip(img, img, 0);
    cvtColor(img, img, cv::COLOR_RGBA2BGRA);

    overlayImage(mat, img, pos);
}

int ARWidget::width()
{
    return p->w;
}

int ARWidget::height()
{
    return p->h;
}

void ARWidget::setValue(int x, int y, int z, int xx, int yy, int zz)
{
    p->x = x;
    p->y = y;
    p->z = z;

    p->x1 = xx;
    p->y1 = yy;
    p->z1 = zz;
}

