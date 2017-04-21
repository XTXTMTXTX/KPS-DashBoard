#include<windows.h>
#include<d3dx9.h>
#include<cstdio>
#include<cmath>
#include<cstdlib>
#define min(x,y) ((x)<(y)?(x):(y))
#define D3DFILTER D3DX_FILTER_TRIANGLE
#define BLACKBG 0x00000000
#define GREENBG 0x0000ff00
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 360
#define WINDOWNAME "KPS DashBoard"
#define FVF_VERTEX (D3DFVF_XYZ|D3DFVF_TEX1)
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#define ll long long
using namespace std;

D3DXMATRIX ScalMatrix,TransMatrix,RotateMatrix,ResultMatrix;
int KPSClock=500,KPSMax=25,MouseClock=100,MaxCursorSpeed=15000,VS=1,KT,KOri,KB,CT,COri;
double KN,KA;
volatile bool KS[256],BGcolor=1;
volatile int FPSLIMIT=120;
volatile int cspeed=0,KC=0,Kcounter=0;
volatile double cspeedcounter=0,cspeedd=0;;
volatile double T0,TC0;
DWORD g_main_tid=0;
HHOOK g_kb_hook=0;

double CPUclock(){
	LARGE_INTEGER nFreq;
	LARGE_INTEGER t1;
	double dt;
 	QueryPerformanceFrequency(&nFreq);
 	QueryPerformanceCounter(&t1);
  	dt=(t1.QuadPart)/(double)nFreq.QuadPart;
  	return(dt*1000);
}
BOOL CALLBACK con_handler(DWORD){
	PostThreadMessage(g_main_tid,WM_QUIT,0,0);
	return TRUE;
};
LRESULT CALLBACK kb_proc(int code,WPARAM w,LPARAM l){
	PKBDLLHOOKSTRUCT p=(PKBDLLHOOKSTRUCT)l;
	if((w==WM_KEYDOWN||w==WM_SYSKEYDOWN)&&(!KS[(p->vkCode)-1])){Kcounter++;KS[(p->vkCode)-1]=1;}
	if(w==WM_KEYUP||w==WM_SYSKEYUP)KS[(p->vkCode)-1]=0;
	return CallNextHookEx(g_kb_hook,code,w,l);
}
void KHK(){
	g_main_tid=GetCurrentThreadId();
	SetConsoleCtrlHandler(&con_handler,TRUE);
    g_kb_hook=SetWindowsHookEx(WH_KEYBOARD_LL,&kb_proc,GetModuleHandle (NULL),0);
	MSG msg;
    while(GetMessage(&msg,NULL,0,0)){
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	UnhookWindowsHookEx (g_kb_hook);
}
int read(FILE *fp){
	int x=0,f=1;char ch=fgetc(fp);
	while(ch<'0'||ch>'9'){if(ch=='-')f=-1;ch=fgetc(fp);}
	while(ch>='0'&&ch<='9'){x=x*10+ch-'0';ch=fgetc(fp);}
	return x*f;
}
void init(){
	FILE *fp;
	if(!(fp=fopen("config.ini","r"))){
		fp=fopen("config.ini","w");
		fprintf(fp,"[Config]\n");
		fprintf(fp,"KPSClock=%d\n",KPSClock);
		fprintf(fp,"MouseClock=%d\n",MouseClock);
		fprintf(fp,"KPSMax=%d\n",KPSMax);
		fprintf(fp,"MaxCursorSpeed=%d\n",MaxCursorSpeed);
		fprintf(fp,"FPS Limit=%d\n",FPSLIMIT);
		fprintf(fp,"Vertical Sync=%d\n",VS);
		fprintf(fp,"Use Green Background=%d\n",BGcolor);
		fclose(fp);
	}else{
		KPSClock=read(fp);
		MouseClock=read(fp);
		KPSMax=read(fp);
		MaxCursorSpeed=read(fp);
		FPSLIMIT=read(fp);
		VS=read(fp);
		BGcolor=read(fp);
		fclose(fp);
	}
}

IDirect3DDevice9*       _device;
IDirect3DVertexBuffer9* _vb;
IDirect3DIndexBuffer9*  _ib;
IDirect3DTexture9 *NAtex=0,*Num0tex=0,*Num1tex=0,*Num2tex=0,*Num3tex=0,*Num4tex=0,*Num5tex=0,*Num6tex=0,*Num7tex=0,*Num8tex=0,*Num9tex=0,*BG1tex=0,*BG2tex=0,*Ptex=0;
bool InitD3D(HINSTANCE hInstance,int width,int height,bool windowed,D3DDEVTYPE deviceType,IDirect3DDevice9 ** device);
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
D3DMATERIAL9 InitMtrl(D3DXCOLOR a,D3DXCOLOR d,D3DXCOLOR s,D3DXCOLOR e,float p);
const D3DXCOLOR      WHITE(D3DCOLOR_XRGB(255, 255, 255));
const D3DXCOLOR      BLACK(D3DCOLOR_XRGB(0, 0, 0));
const D3DMATERIAL9 WHITE_MTRL = InitMtrl(WHITE, WHITE, WHITE, BLACK, 2.0f);
struct Vertex {
	float _x,_y,_z;
	float _u,_v;
	Vertex(){}
	Vertex(float x,float y,float z,float u,float v):_x(x),_y(y),_z(z),_u(u),_v(v){}
};
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
	switch (msg) {
		case WM_DESTROY:
			::PostQuitMessage(0);
		break;
		case WM_KEYDOWN:
			if (wParam==VK_ESCAPE) ::DestroyWindow(hwnd);
		break;
	}
	return ::DefWindowProc(hwnd,msg,wParam,lParam);
}
void SetObjectPositionXYZ(D3DXMATRIX &World,const float x,const float y,const float z,const float AngleX,const float AngleY,const float AngleZ){  
    D3DXMATRIX WorldX,WorldY,WorldZ;  
    D3DXMatrixRotationX(&WorldX,AngleX);
    D3DXMatrixRotationY(&WorldY,AngleY);
    D3DXMatrixRotationZ(&WorldZ,AngleZ);
    D3DXMatrixTranslation(&World,x,y,z);
    World*=WorldX;  
    World*=WorldY;  
    World*=WorldZ;  
    return;  
}  
bool InitD3D(HINSTANCE hInstance,int width,int height,bool windowed,D3DDEVTYPE deviceType,IDirect3DDevice9 ** device){
	WNDCLASS wc;
	wc.style=CS_VREDRAW|CS_HREDRAW;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInstance;
	wc.hIcon=LoadIcon(0,IDI_APPLICATION);
	wc.hCursor=LoadCursor(0,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName="Window";
	wc.lpszMenuName=0;
	if(!(RegisterClass(&wc))){
		MessageBox(0,"RegisterClass is FAILED",0,0);
		return false;
	}
	HWND hwnd=0;
	hwnd=CreateWindow("Window",WINDOWNAME,WS_CAPTION|WS_SYSMENU,100,100,SCREEN_WIDTH+6,SCREEN_HEIGHT+28,0,0,hInstance,0);
	if(hwnd==0){
		MessageBox(0,"CreateWIndow is FAILED",0,0);
		return false;
	}
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	IDirect3D9 *d3d9=0;
	d3d9 = Direct3DCreate9((D3D_SDK_VERSION));
	if(!d3d9){
		MessageBox(0,"Direct3D9Create9 is FAILED",0,0);
		return false;
	}
	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT,deviceType,&caps);
	int vp=0;
	if(caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)vp=D3DCREATE_HARDWARE_VERTEXPROCESSING;else vp=D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth=SCREEN_WIDTH;
	d3dpp.BackBufferHeight=SCREEN_HEIGHT;
	d3dpp.BackBufferFormat=D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount=1;
	d3dpp.MultiSampleType=D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality=0;
	d3dpp.SwapEffect=D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow=hwnd;
	d3dpp.Windowed=windowed;
	d3dpp.EnableAutoDepthStencil=true;
	d3dpp.AutoDepthStencilFormat=D3DFMT_D24S8;
	d3dpp.Flags=0;
	d3dpp.FullScreen_RefreshRateInHz=D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval=(VS?D3DPRESENT_INTERVAL_DEFAULT:D3DPRESENT_INTERVAL_IMMEDIATE);
	if (FAILED(d3d9->CreateDevice(D3DADAPTER_DEFAULT,deviceType,hwnd,vp,&d3dpp,device))){
		MessageBox(0, "CreateDevice is FAILED", 0, 0);
		return false;
	}
	d3d9->Release();
	return true;
}
D3DMATERIAL9 InitMtrl(D3DXCOLOR a,D3DXCOLOR d,D3DXCOLOR s,D3DXCOLOR e,float p){
	D3DMATERIAL9 mtrl;
	mtrl.Ambient=a;
	mtrl.Diffuse=d;
	mtrl.Specular=s;
	mtrl.Emissive=e;
	mtrl.Power=p;
	return mtrl;
}
bool InitCube(){
	_device->CreateVertexBuffer(16*sizeof(Vertex),D3DUSAGE_WRITEONLY,FVF_VERTEX,D3DPOOL_MANAGED,&_vb,0);
	Vertex* v;
	_vb->Lock(0,0,(void**)&v,0);
	v[0]=Vertex(-1.0f,-1.0f,-1.0f,0.0f,1.0f);
	v[1]=Vertex(-1.0f,1.0f,-1.0f,0.0f,0.0f);
	v[2]=Vertex(1.0f,1.0f,-1.0f,1.0f,0.0f);
	v[3]=Vertex(1.0f,-1.0f,-1.0f,1.0f,1.0f);

	_vb->Unlock();
	
	_device->CreateIndexBuffer(16*sizeof(WORD),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_MANAGED,&_ib,0);
	WORD* i=0;
	_ib->Lock(0,0,(void**)&i,0);
	i[0]=0;
	i[1]=1;
	i[2]=2;
	i[3]=0;
	i[4]=2;
	i[5]=3;

	_ib->Unlock();
	return true;
}
void Drawnumber(const int &x){
	switch(x){
		case 0:
			_device->SetTexture(0,Num0tex);
		break;
		case 1:
			_device->SetTexture(0,Num1tex);
		break;
		case 2:
			_device->SetTexture(0,Num2tex);
		break;
		case 3:
			_device->SetTexture(0,Num3tex);
		break;
		case 4:
			_device->SetTexture(0,Num4tex);
		break;
		case 5:
			_device->SetTexture(0,Num5tex);
		break;
		case 6:
			_device->SetTexture(0,Num6tex);
		break;
		case 7:
			_device->SetTexture(0,Num7tex);
		break;
		case 8:
			_device->SetTexture(0,Num8tex);
		break;
		case 9:
			_device->SetTexture(0,Num9tex);
		break;
	}
}
bool Setup() {
	InitCube();
	D3DLIGHT9 light;
	::ZeroMemory(&light,sizeof(light));
	light.Type=D3DLIGHT_DIRECTIONAL;
	light.Ambient=D3DXCOLOR(0.8f,0.8f,0.8f,1.0f);
	light.Diffuse=D3DXCOLOR(1.0f,1.0f,1.0f,1.0f);
	light.Specular=D3DXCOLOR(0.2f,0.2f,0.2f,1.0f);
	light.Direction=D3DXVECTOR3(1.0f,-1.0f,0.0f);
	_device->SetLight(0,&light);
	_device->LightEnable(0,true);
	_device->SetRenderState(D3DRS_NORMALIZENORMALS, true);
	_device->SetRenderState(D3DRS_SPECULARENABLE, true);
	D3DXCreateTextureFromFileEx(_device, "SKIN/BG1.png",300,300,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&BG1tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/BG2.png",300,300,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&BG2tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/0.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num0tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/1.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num1tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/2.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num2tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/3.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num3tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/4.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num4tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/5.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num5tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/6.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num6tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/7.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num7tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/8.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num8tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/9.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Num9tex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/-.png",150,150,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&NAtex);
	D3DXCreateTextureFromFileEx(_device, "SKIN/P.png",300,300,D3DX_DEFAULT,0,D3DFMT_UNKNOWN,D3DPOOL_MANAGED,D3DFILTER,D3DFILTER,0,NULL,NULL,&Ptex);
	_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	
	_device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	_device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
	_device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
	_device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
	_device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
	_device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	_device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	_device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(&proj,D3DX_PI*0.5f,(float)SCREEN_WIDTH/(float)SCREEN_HEIGHT,1.0f,1000.0f);
	_device->SetTransform(D3DTS_PROJECTION,&proj);
	return true;
}
bool DisPlay() {
	MSG msg;
	::ZeroMemory(&msg,sizeof(MSG));
	
	POINT pt;
	GetCursorPos(&pt);
	int cx=pt.x,cy=pt.y;
	double t=CPUclock();
	while (msg.message!=WM_QUIT){
		
		GetCursorPos(&pt);
        cspeedcounter+=(double)(sqrt((pt.x-cx)*(pt.x-cx)+(pt.y-cy)*(pt.y-cy)));
        if(t-TC0>=MouseClock){
        	cspeedd=cspeedcounter*1000/(t-TC0);
        	cspeedcounter=0.0;
        	TC0=CPUclock();
        }
        cx=pt.x;cy=pt.y;
        t=CPUclock();
        if(t-T0>=KPSClock){
        	KC=Kcounter*1000/(t-T0);
        	Kcounter=0;
        	T0=CPUclock();
		}
		KT=min(100,KC);
		CT=min(cspeedd,MaxCursorSpeed);
		if(KT!=KOri){
			double x=CPUclock()-T0;
			if(x>=double(KPSClock)*3.0/4.0){KOri=KT;KN=KT;}else KN=double(16*(KOri-KT))/double(9*KPSClock*KPSClock)*x*x+double(8*(KT-KOri))/double(3*KPSClock)*x+KOri;
		}
		if(CT!=COri){
			double x=CPUclock()-TC0;
			if(x>=double(MouseClock)*3.0/4.0){COri=CT;cspeed=CT;}else cspeed=int(double(16*(COri-CT))/double(9*MouseClock*MouseClock)*x*x+double(8*(CT-COri))/double(3*MouseClock)*x+COri);
		}
		KA=min(KN,(double)KPSMax);
		KB=min((int)KN,100);
		
		if (::PeekMessage(&msg,0,0,0,PM_REMOVE)){
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}else{ 
			if(_device){
	        	const float constFps=(float)FPSLIMIT;
        		DWORD timeInPerFrame=1000.0f/constFps;
        		DWORD timeBegin=GetTickCount();
				static float angle=(3.0f*D3DX_PI)/2.0f;
				static float height=0.0f;
				D3DXVECTOR3 position(cosf(angle)*3.0f,height,sinf(angle)*3.0f);
				D3DXVECTOR3 target(0.0f,0.0f,0.0f);
				D3DXVECTOR3 up(0.0f,1.0f,0.0f);
				D3DXMATRIX V;
				D3DXMatrixLookAtLH(&V, &position, &target, &up);
				_device->SetTransform(D3DTS_VIEW, &V);
				_device->Clear(0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,(BGcolor?GREENBG:BLACKBG),1.0f,0);
				_device->BeginScene();
				_device->SetMaterial(&WHITE_MTRL);

				_device->SetStreamSource(0,_vb,0,sizeof(Vertex));
				_device->SetIndices(_ib);
				_device->SetFVF(FVF_VERTEX);
				
                _device->SetTexture(0,BG2tex);
				D3DXMatrixScaling(&ScalMatrix,1.2875f,1.2875f,1.5f);
				D3DXMatrixTranslation(&TransMatrix,1.362507,-0.57,0.0f);  
				ResultMatrix=ScalMatrix*TransMatrix;
                _device->SetTransform(D3DTS_WORLD, &ResultMatrix);  
				_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
				
				
                _device->SetTexture(0,BG1tex);
				D3DXMatrixScaling(&ScalMatrix,1.5f,1.5f,1.5f);
				D3DXMatrixTranslation(&TransMatrix,-1.25f,-0.5f,0.0f);  
				ResultMatrix=ScalMatrix*TransMatrix;
                _device->SetTransform(D3DTS_WORLD, &ResultMatrix);  
				_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
				
				if(KB==100){
					_device->SetTexture(0,NAtex);
					D3DXMatrixScaling(&ScalMatrix,0.215,0.215,1.5f);
					D3DXMatrixTranslation(&TransMatrix,-1.3812,-0.935,0.0f); 
					ResultMatrix=ScalMatrix*TransMatrix;
					_device->SetTransform(D3DTS_WORLD, &ResultMatrix); 
					_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
					D3DXMatrixScaling(&ScalMatrix,0.215,0.215,1.5f);
					D3DXMatrixTranslation(&TransMatrix,-1.10322,-0.935,0.0f); 
					ResultMatrix=ScalMatrix*TransMatrix;
					_device->SetTransform(D3DTS_WORLD, &ResultMatrix); 
					_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
				}else{
					Drawnumber(KB/10);
					D3DXMatrixScaling(&ScalMatrix,0.215,0.215,1.5f);
					D3DXMatrixTranslation(&TransMatrix,-1.3812,-0.935,0.0f); 
					ResultMatrix=ScalMatrix*TransMatrix;
					_device->SetTransform(D3DTS_WORLD, &ResultMatrix); 
					_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
					Drawnumber(KB%10);
					D3DXMatrixScaling(&ScalMatrix,0.215,0.215,1.5f);
					D3DXMatrixTranslation(&TransMatrix,-1.10322,-0.935,0.0f); 
					ResultMatrix=ScalMatrix*TransMatrix;
					_device->SetTransform(D3DTS_WORLD, &ResultMatrix); 
					_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
				}
				_device->SetTexture(0,Ptex);
				float x2=(float)cspeed*4.375f/float(MaxCursorSpeed)-2.22;
				D3DXMatrixScaling(&ScalMatrix,1.2875f,1.2875f,1.5f);
				D3DXMatrixTranslation(&TransMatrix,1.362507,-0.57,0.0f);  
				ResultMatrix=ScalMatrix*TransMatrix;
				SetObjectPositionXYZ(RotateMatrix,0.0f,0.0f,0.0f,0.0f,0.0f,-0.025-x2);
				ResultMatrix=RotateMatrix*ResultMatrix;
				_device->SetTransform(D3DTS_WORLD, &ResultMatrix);
				_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
				
				_device->SetTexture(0,Ptex);
				float x=(float)KA*4.375f/float(KPSMax)-2.22;
				D3DXMatrixScaling(&ScalMatrix,1.5f,1.5f,1.5f);
				D3DXMatrixTranslation(&TransMatrix,-1.25f,-0.5f,0.0f); 
				ResultMatrix=ScalMatrix*TransMatrix;
				SetObjectPositionXYZ(RotateMatrix,0.0f,0.0f,0.0f,0.0f,0.0f,-0.025-x);
				ResultMatrix=RotateMatrix*ResultMatrix;
				_device->SetTransform(D3DTS_WORLD, &ResultMatrix);
				_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,4,0,2);
				
				_device->EndScene();
				_device->Present(0,0,0,0);
				
				DWORD timePhase=GetTickCount()-timeBegin;
        		if(timePhase<timeInPerFrame)Sleep(DWORD(timeInPerFrame-timePhase));
			}
		}
	}
	return true;
}
int WINAPI WinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPSTR lpCmdLine,_In_ int nShowCmd){
	init();
	TC0=T0=CPUclock();
	HANDLE h1=CreateThread(0,0,(LPTHREAD_START_ROUTINE)KHK,0,1,0);ResumeThread(h1);
	
	if(!InitD3D(hInstance,SCREEN_WIDTH,SCREEN_HEIGHT,true,D3DDEVTYPE_HAL,&_device)){
		MessageBox(0,"InitD3D is FAILED",0,0);
		return 0;
	}
	if(!Setup()){
		MessageBox(0,"Setup() - FAILED",0,0);
		return 0;
	}
	DisPlay();
	_device->Release();
	return 0;
}
