#include <windows.h>
#include <gl/gl.h>
#include <math.h>
#include <mmsystem.h>
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

HWND hwnd;
/* создание куба */
/* 8 вершин */
float kube[] = {0,0,0, 0,1,0, 1,1,0, 1,0,0, 0,0,1, 0,1,1, 1,1,1, 1,0,1};
/* 6 граней, массив индексов, по два независимых треугольника на каждую грань */
GLuint kubeInd[] = {0,1,2, 2,3,0, 4,5,6, 6,7,4, 3,2,5, 6,7,3, 0,1,5, 5,4,0,
                    1,2,6, 6,5,1, 0,3,7, 7,4,0};
float pyramid[] = {0,0,2, 1,1,0, 1,-1,0, -1,-1,0, -1,1,0, 1,1,0};
BOOL showMask = FALSE;
typedef struct{
    float r,g,b;
}TColor;

typedef struct{
    TColor clr;
}TCell;

/* создание карты */
#define pW 40
#define pH 40
TCell map[pW][pH];
void Map_Init()
{
    for (int i = 0; i<pW; i++)
        for (int j=0; j<pH;j++)
        {
            /* dc - изменение цвета, чтобы все квадратики карты отличались, чтобы было понятно, что перемещение идет */
            float dc = (rand()%20)*0.01;
            map[i][j].clr.r = 0.31 + dc;
            map[i][j].clr.g = 0.5 + dc;
            map[i][j].clr.b = 0.13 + dc;
        }
}

#define enemyCnt 40
struct{
    /* координаты врага в пространстве и active - показывает, что враг в игре присутствует */
    float x,y,z;
    BOOL active
} enemy[enemyCnt];

/* инициализация врагов */
void Enemy_Init(){
    for(int i = 0; i<enemyCnt; i++){
        enemy[i].active = TRUE;
        enemy[i].x = rand()% pW;
        enemy[i].y = rand()% pH;
        enemy[i].z = rand()% 5;
    }
}

void Pyramid_Show(){

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, &pyramid);
    glColor3f(0.0f, 0.0f, 0.0f);
    glDrawArrays(GL_TRIANGLE_FAN,0,6);

    glDisableClientState(GL_VERTEX_ARRAY);
}


/* рисование врагов */
void Enemy_Show(){

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, kube);
    for(int i=0;i<enemyCnt;i++)
    {
        /* если враг не активен, то не рисуем его (его нет) */
     if(!enemy[i].active) continue;
        glPushMatrix();
        glTranslatef(enemy[i].x, enemy[i].y,enemy[i].z);
        if(showMask)
            glColor3ub(255-i,0,0);
        else
            glColor3ub(244,60,43);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, kubeInd);
        glPopMatrix();
    }

    glDisableClientState(GL_VERTEX_ARRAY);
}

/* создание переменной структурного типа для камеры */
struct{
    float x,y,z;
    float Xrot,Zrot;
}camera = {0,0,1.7,70,-40};

/* применение параметров камеры */
void Camera_Apply(){
    glRotatef(-camera.Xrot, 1,0,0);
    glRotatef(-camera.Zrot, 0,0,1);
    glTranslatef(-camera.x, -camera.y, -camera.z);
}
/* поворачивание камеры */
void Camera_Rotation(float xAngle, float zAngle){
    camera.Zrot += zAngle;
    if(camera.Zrot<0) camera.Zrot += 360;
    if(camera.Zrot<360) camera.Zrot -= 360;
    camera.Xrot += xAngle;
    if(camera.Xrot<0) camera.Xrot += 0;
    if(camera.Xrot>180) camera.Xrot += 180;
}
/* перемещение игрока */
void Player_Move(){
    /* проверка, если окно игры находится не в фокусе */
    if(GetForegroundWindow() != hwnd) return;

    float ugol = -camera.Zrot/180 * M_PI;
    float speed = 0;
    if(GetKeyState('W')<0) speed = 0.1;
    if(GetKeyState('S')<0) speed = -0.1;
    if(GetKeyState('A')<0) {speed = 0.1; ugol -=M_PI*0.5;}
    if(GetKeyState('D')<0) {speed = 0.1; ugol +=M_PI*0.5;}
    if(speed!=0){
        camera.x +=sin(ugol)*speed;
        camera.y +=cos(ugol)*speed;
    }

    POINT cur;
    static POINT base = {400,300};
    /* узнаем насколько сместился курсор и возвращаем его в исходную позицию  */
    GetCursorPos(&cur);
    Camera_Rotation((base.y-cur.y)/5.0, (base.x-cur.x)/5.0);
    SetCursorPos(base.x, base.y);
}

void WndResize(int x, int y);

/* процедура работы игры */
void Game_Move(){
    Player_Move();
}

/* инициализация игры */
void Game_Init(){
    glEnable(GL_DEPTH_TEST);
    Map_Init();
    Enemy_Init();
    RECT rct;
    GetClientRect(hwnd,&rct);
    WndResize(rct.right, rct.bottom);
}
void Game_Show(){

    if(showMask)
        glClearColor(0,0,0,0);
    else
    /* очистка экрана */
    glClearColor(0.6,0.8,1,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
        Camera_Apply();
        /* разрешение использования массива вершин, в качестве массива вершин берем кубик */
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT,0,kube);
        for(int i = 0; i<pW;i++)
            for(int j = 0; j<pH;j++)
            {
                glPushMatrix();
                /* передвигаем куб, задаем цвет и рисуем */
                    glTranslatef(i,j,0);
                    if(showMask)
                        glColor3f(0,0,0);
                    else
                    glColor3f(map[i][j].clr.r, map[i][j].clr.g, map[i][j].clr.b);
                    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,kubeInd);
                glPopMatrix();
            }
        glDisableClientState(GL_VERTEX_ARRAY);

        Enemy_Show();
        Pyramid_Show();
    glPopMatrix();
}
/* выстрел */
void Player_Shoot(){
    /* рисуем сцену в виде маски */
    showMask = TRUE;
    /* рисуем всю сцену (все будет нарисовано черным, а враги с оттенком красного. Все это происходит в заднем буфере, пользователь не видит этого) */
    Game_Show();
    /* отключаем флаг маски выбора */
    showMask = FALSE;

    RECT rct;
    GLubyte clr[3];
    GetClientRect(hwnd,&rct);
    /* glReadPixels возвращает картинку заданной области экрана */
    glReadPixels( rct.right/2.0, rct.bottom/2.0, 1,1,
                 GL_RGB, GL_UNSIGNED_BYTE, clr);
    /* если есть красный цвет, то по нему вычисляется номер врага в массиве, после чего он деактивируется */
    if(clr[0]>0)
        enemy[255 - clr[0]].active = FALSE;
    /* проигрывание звука выстрела*/
    PlaySound("vyistrel-s-ruchnyim-perezaryadom.wav", NULL, SND_ASYNC);
}


/* изменяет настройки проекции в зависимости от размера области вывода */
void WndResize(int x, int y){
    glViewport(0,0, x,y);
    float k = x/(float)y;
    float sz = 0.1;
    glLoadIdentity();
    glFrustum(-k*sz,k*sz, -sz,sz, sz*2, 100);
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;

    /* register window class */
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if (!RegisterClassEx(&wcex))
        return 0;

    /* create main window */
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "OpenGL Sample",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          800,
                          600,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    /* enable OpenGL for the window */
    EnableOpenGL(hwnd, &hDC, &hRC);


    Game_Init();

    /* program main loop */
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            /* OpenGL animation code goes here */
            Game_Move();
            Game_Show();


            SwapBuffers(hDC);

            Sleep (1);
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL(hwnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;
        /* изменение настроек проекции при изменении размера окна */
        case WM_SIZE:
            WndResize(LOWORD(lParam),HIWORD(lParam));
        break;

        case WM_SETCURSOR:
            ShowCursor(FALSE);
        break;

        case WM_LBUTTONDOWN:
            Player_Shoot();
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

