#include <Windows.h>
#include <tchar.h>
#include <fstream>
#include <random>
#include <math.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
//#include "resource.h"
#include <atlimage.h> // CImage 사용을 위해 추가
using namespace std;

bool gameStart = false;
void  GameUpdateProc(HWND hWnd);
float Distance(float x1, float y1, float x2, float y2);
bool isOutMap(float x, float y);
//주인공의 상태를 변경하는 함수 //상태당 애니메이션에 필요한 설정도 같이함
void SetCharacterState(int newState);

random_device rd;
mt19937 gen(rd());

HINSTANCE g_hinst;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hinst = hInstance;
	LPCTSTR lpszClass = L"My Window Class";
	LPCTSTR lpszWindowName = L"Window Programming Lab";

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, 1280, 960, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	DWORD lastTime = timeGetTime();
	while (1) {
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) { // 운영체제 메시지(키보드, 마우스 등) 처리 
			if (Message.message == WM_QUIT)
				break;
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		else {										// 메시지가 없을 때 (즉, 유저 입력이 없는 평상시에) 실행되는 부분
			DWORD currentTime = timeGetTime();
			if (currentTime - lastTime >= 15) {
				GameUpdateProc(hWnd);
				lastTime = currentTime;
			}
			Sleep(1);
		}
	}
	return Message.wParam;
}

#define GRAVITY 0.88
#define MCWALLCIMBSPEED 3
#define MCMOVESPEED 0.9
#define MCJUMPACC 18
#define MAXROPELEN 400
#define MAXROPESHOOTLEN 600

#define FACING_LEFT 0
#define FACING_RIGHT 1

#define NO_CLIMBING 0
#define CLIMBING_UP 1
#define CLIMBING_DOWN 2

#define MCVERTICALSIZE 50	//30 //테스트로 크기 변경해봄
#define MCHORIZONALSIZE 30	//30

#define ISSTANDING 0
#define ISSWINGING 1
#define ISJUMPING 2
#define ISLANDING 3
#define ISSWINGJUMPING 4
#define ISSTARTINGRUN 5
#define ISRUNNING 6
#define ISSTOPPING 7
#define ISDAMAGED 8
#define ISFALLING 9
#define ONWALL 10
#define ISWALLCLIMBING_UP 11
#define ISWALLCLIMBING_DOWN 12

//상태에 따른 애니메이션 최대 프레임 개수- 더 추가예정, 그런데 상태에 따른 애니메이션 이미지 종류보다 현재 mc.state의 상태가 부족함.(fall_start, wall_climbUP/Down, Death, holding 등). 
#define STARTRUN_MAXFRAME 2
#define RUNNING_MAXFRAME 20
#define STOPRUN_MAXFRAME 6

#define STANDING_MAXFRAME 8

#define JUMPING_MAXFRAME 6
#define FALLSTART_MAXFRAME 3
#define FALLING_MAXFRAME 3
#define LANDING_MAXFRAME 3


#define WALLCLIMBDOWN_MAXFRAME 7
#define WALLCLIMBUP_MAXFRAME 10

#define SWINGJUMP_MAXFRAME 5
#define SWINGING_MAXFRAME 15

#define EXHOLDINGBACK_MAXFRAME 8
#define DAMAGED_MAXFRAME 5
#define DEATH_MAXFRAME 24





struct MAINCHARACTER {
	float x, y;
	float oldX, oldY, accX, accY;
	int hp;
	int state;
	int climbingDirection;
	bool isGrounded, facingDirection, canjump, isInvincible;
	//애니메이션용 변수
	int currentFrame;     // 현재 프레임 번호
	int maxFrame;         // 최대 프레임 수  //state에 따라 갱신됨
	DWORD lastAnimTime;   // 마지막으로 프레임이 바뀐 시간
	int animDelay;        // 프레임 전환 간격 (밀리초 단위)
	//애니메이션용 배열
	CImage RunningSprites_Right[RUNNING_MAXFRAME];
	CImage RunningSprites_Left[RUNNING_MAXFRAME];
	CImage StartRunSprites_Right[STARTRUN_MAXFRAME];
	CImage StartRunSprites_Left[STARTRUN_MAXFRAME];
	CImage StopRunSprites_Right[STOPRUN_MAXFRAME];
	CImage StopRunSprites_Left[STOPRUN_MAXFRAME];

	CImage StandingSprites_Right[STANDING_MAXFRAME];
	CImage StandingSprites_Left[STANDING_MAXFRAME];

	CImage JumpingSprites_Right[JUMPING_MAXFRAME];
	CImage JumpingSprites_Left[JUMPING_MAXFRAME];

	CImage StartFallSprites_Right[FALLSTART_MAXFRAME];
	CImage StartFallSprites_Left[FALLSTART_MAXFRAME];
	CImage FallingSprites_Right[FALLING_MAXFRAME];
	CImage FallingSprites_Left[FALLING_MAXFRAME];
	CImage LandingSprites_Right[LANDING_MAXFRAME];
	CImage LandingSprites_Left[LANDING_MAXFRAME];

	CImage SwingingSprites_Right[SWINGING_MAXFRAME];
	CImage SwingingSprites_Left[SWINGING_MAXFRAME];
	CImage SwingJumpingSprites_Right[SWINGJUMP_MAXFRAME];
	CImage SwingJumpingSprites_Left[SWINGJUMP_MAXFRAME];

	CImage ClimbUpSprites_Right[WALLCLIMBUP_MAXFRAME];
	CImage ClimbUpSprites_Left[WALLCLIMBUP_MAXFRAME];
	CImage ClimbDownSprites_Right[WALLCLIMBDOWN_MAXFRAME];
	CImage ClimbDownSprites_Left[WALLCLIMBDOWN_MAXFRAME];

	CImage ExHoldingBackSprites_Right[EXHOLDINGBACK_MAXFRAME];
	CImage ExHoldingBackSprites_Left[EXHOLDINGBACK_MAXFRAME];

	CImage DamagedSprites_Right[DAMAGED_MAXFRAME];
	CImage DamagedSprites_Left[DAMAGED_MAXFRAME];
	CImage DeathSprites_Right[DEATH_MAXFRAME];
	CImage DeathSprites_Left[DEATH_MAXFRAME];

	MAINCHARACTER() { //초기화
		x = 100, y = 24900;
		oldX = 100, oldY = 24900;
		accX = 0, accY = 0;
		hp = 4;
		canjump = false;
		isGrounded = false;
		state = ISSTANDING;
		facingDirection = FACING_RIGHT;
		climbingDirection = NO_CLIMBING;
		isInvincible = false;
		//애니메이션용 변수
		currentFrame = 0;
		maxFrame = STANDING_MAXFRAME; //기본 standing(idle)의 최대 frame
		animDelay = 100; //일단 0.1초
		lastAnimTime = timeGetTime();
	}
};

struct ANCHOR {
	float x, y, length;
};

struct CAMERA {
	float x, y;
	float sizeX, sizeY;
};

#define BULLET_SMALL 1
#define BULLET_BIG 2

struct BULLET {
	float x, y, direction;
	int type;
};

#define ENEMY_ISWAITING 0

// 가로 세로 동일 (50x50) 플랫폼 한 칸 크기
#define TROOPERSIZE 50

struct ENEMY_TROOPER {
	float x, y, direction;
	int state;
	bool activated;
};

#define TURRETSIZE 50

struct ENEMY_TURRET {
	float x, y, direction;
	int stickDirection;			// 벽에 붙어있는 방향 (어느쪽 벽에 붙어있는지)
	int state;
	bool activated;
};

// 가로 세로 동일 (100x100) 플랫폼 2칸 x 2칸
#define DEFENDERSIZE 100

struct ENEMY_DEFENDER {
	float x, y;
	bool facingDirection;		// 보고 있는 방향 (방패 방향)
	int state;
	bool activated;
};

#define PLATFORMMAXROW 500
#define PLATFORMMAXCOL 1000
#define PLATFORMSIZE 50

#define WALL_TOP 0
#define WALL_RIGHT 1
#define WALL_BOTTOM 2
#define WALL_LEFT 3

#define WALL_CONNECT 1
#define WALL_CANHOOK 2
#define WALL_CANNOTHOOK 3
#define WALL_DAMAGE 4

struct PLATFORM {
	bool isPlatform;
	int type[4];
	PLATFORM() {
		isPlatform = false;
		for (int i = 0; i < 4; i++) type[i] = WALL_CONNECT;
	}
};

MAINCHARACTER mc;
ANCHOR anch;
CAMERA cam;
PLATFORM platforms[PLATFORMMAXROW][PLATFORMMAXCOL];

BULLET bullets[200];
int bulletsNum = 0;

ENEMY_TROOPER trooper[100];
int troopersNum = 0;
ENEMY_TURRET turret[100];
int turretsNum = 0;
ENEMY_DEFENDER defender[100];
int defendersNum = 0;

bool keys[256] = { 0 };

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC, mDC;
	HBITMAP hBitmap;
	RECT rt;

	// 마우스 위치
	static float mx, my;
	// 사슬 거리 계산용 x,y
	float dx, dy;
	// 카메라 안에서 그려야 할 행, 열 개수
	static int HowManyRow, HowManyCol;

	switch (uMsg) {
	case WM_CREATE:
		GetClientRect(hWnd, &rt);
		cam.sizeX = rt.right, cam.sizeY = rt.bottom;
		cam.x = 0, cam.y = PLATFORMMAXROW * PLATFORMSIZE - cam.sizeY + 50;
		HowManyRow = (cam.sizeY / PLATFORMSIZE) + 2, HowManyCol = (cam.sizeX / PLATFORMSIZE) + 2;

		// 맵 로딩
		{
			ifstream in{ "Platform_Info.txt" };
			for (int i = 0; i < PLATFORMMAXROW; i++) {
				for (int j = 0; j < PLATFORMMAXCOL; j++) {
					float x = j * PLATFORMSIZE, y = i * PLATFORMSIZE;
					int PlatformInfo;
					in >> PlatformInfo;
					if (PlatformInfo == 0) platforms[i][j].isPlatform = 0;
					else if (PlatformInfo == 7) {
						trooper[troopersNum].x = x;
						trooper[troopersNum].y = y;
						trooper[troopersNum].activated = false;
						//trooper[troopersNum].state =
						troopersNum++;
					}
					else if (PlatformInfo == 8) {
						turret[turretsNum].x = x;
						turret[turretsNum].y = y;
						turret[turretsNum].activated = false;
						//turret[turretsNum].state =
						turretsNum++;
					}
					else if (PlatformInfo == 9) {
						defender[defendersNum].x = x;
						defender[defendersNum].y = y - PLATFORMSIZE;
						defender[defendersNum].activated = false;
						//defender[defendersNum].state = 
						defendersNum++;
					}
					else {
						platforms[i][j].isPlatform = true;
						platforms[i][j].type[0] = PlatformInfo / 1000;
						platforms[i][j].type[1] = (PlatformInfo % 1000) / 100;
						platforms[i][j].type[2] = (PlatformInfo % 100) / 10;
						platforms[i][j].type[3] = PlatformInfo % 10;
					}
				}
			}
			in.close();
		}

		///주인공 Sprite 불러오기 //상대경로 사용 // Resources 폴더 안의 파일명을 매칭
		WCHAR filepath[256];
		for (int i = 0; i < 30; i++) { //일단 최대 30프레임 짜리 

			if (i < STANDING_MAXFRAME) {
				//오른쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\standing\\right\\Spr_SNB_Idle (lp) (%d).png", i + 1); //Spr_SNB_Idle (lp) (1).png //Resource\MainCharacter\standing\right\Spr_SNB_Idle (lp) (1).png"
				if (FAILED(mc.StandingSprites_Right[i].Load(filepath))) {//로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, L"이미지를 찾을 수 없습니다.", filepath, MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\standing\\left\\Spr_SNB_Idle (lp) (%d).png", i + 1); //Spr_SNB_Idle (lp) (1).png //Resource\MainCharacter\standing\left\Spr_SNB_Idle (lp) (1).png"
				if (FAILED(mc.StandingSprites_Left[i].Load(filepath))) {//로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, L"이미지를 찾을 수 없습니다.", filepath, MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < RUNNING_MAXFRAME) {
				//오른쪽 
				//if (i == 7) continue; //running 8번이미지 누락되어있음.
				wsprintf(filepath, L"Resource\\MainCharacter\\running\\running\\right\\Spr_SNB_Running (lp) (%d).png", i + 1); //Resource\MainCharacter\running\running\right\Spr_SNB_Running (lp) (1).png"
				if (FAILED(mc.RunningSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\running\\running\\left\\Spr_SNB_Running (lp) (%d).png", i + 1); //Resource\MainCharacter\running\running\left\Spr_SNB_Running (lp) (1).png"
				if (FAILED(mc.RunningSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < STARTRUN_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\running\\start_run\\right\\Spr_SNB_RunStart (%d).png", i + 1);//Resource\MainCharacter\running\start_run\right\Spr_SNB_RunStart (1).png"
				if (FAILED(mc.StartRunSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\running\\start_run\\left\\Spr_SNB_RunStart (%d).png", i + 1);//Resource\MainCharacter\running\start_run\left\Spr_SNB_RunStart (1).png"
				if (FAILED(mc.StartRunSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < STOPRUN_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\running\\stop_run\\right\\Spr_SNB_RunStop (%d).png", i + 1);//Resource\MainCharacter\running\stop_run\right\Spr_SNB_RunStop (1).png"
				if (FAILED(mc.StopRunSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\running\\stop_run\\left\\Spr_SNB_RunStop (%d).png", i + 1);//Resource\MainCharacter\running\stop_run\left\Spr_SNB_RunStop (1).png"
				if (FAILED(mc.StopRunSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < JUMPING_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\jumping\\right\\Spr_SNB_Jumping (lp) (%d).png", i + 1);//Resource\MainCharacter\jumping\right\Spr_SNB_Jumping (lp) (1).png"
				if (FAILED(mc.JumpingSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\jumping\\left\\Spr_SNB_Jumping (lp) (%d).png", i + 1);//Resource\MainCharacter\jumping\left\Spr_SNB_Jumping (lp) (1).png"
				if (FAILED(mc.JumpingSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < FALLSTART_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\falling\\fall_start\\right\\Spr_SNB_FallStart (%d).png", i + 1);//Resource\MainCharacter\falling\fall_start\right\Spr_SNB_FallStart (1).png"
				if (FAILED(mc.StartFallSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\falling\\fall_start\\left\\Spr_SNB_FallStart (%d).png", i + 1);//Resource\MainCharacter\falling\fall_start\left\Spr_SNB_FallStart (1).png"
				if (FAILED(mc.StartFallSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < FALLING_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\falling\\falling\\right\\Spr_SNB_Falling (lp) (%d).png", i + 1);//\Resource\MainCharacter\falling\falling\right\Spr_SNB_Falling (lp) (1).png"
				if (FAILED(mc.FallingSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\falling\\falling\\left\\Spr_SNB_Falling (lp) (%d).png", i + 1);//\Resource\MainCharacter\falling\falling\left\Spr_SNB_Falling (lp) (1).png"
				if (FAILED(mc.FallingSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < LANDING_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\falling\\landing\\right\\Spr_SNB_Landing (%d).png", i + 1);//Resource\MainCharacter\falling\landing\right\Spr_SNB_Landing (1).png"
				if (FAILED(mc.LandingSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\falling\\landing\\left\\Spr_SNB_Landing (%d).png", i + 1);//Resource\MainCharacter\falling\landing\left\Spr_SNB_Landing (1).png"
				if (FAILED(mc.LandingSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < SWINGING_MAXFRAME) {
				//오른쪽 //"D:\GitHub\WP_Final_Project_SANABI\WPFProject\WPFProject\Resource\MainCharacter\swing\swinging\right\Spr_SNB_Swing (1).png"
				wsprintf(filepath, L"Resource\\MainCharacter\\swing\\swinging\\right\\Spr_SNB_Swing (%d).png", i + 1);//Resource\MainCharacter\swing\swinging\right\Spr_SNB_Swing (1).png"
				if (FAILED(mc.SwingingSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\swing\\swinging\\left\\Spr_SNB_Swing (%d).png", i + 1);//Resource\MainCharacter\swing\swinging\left\Spr_SNB_Swing (1).png"
				if (FAILED(mc.SwingingSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < SWINGJUMP_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\swing\\swing_jumping\\right\\Spr_SNB_SwingJumpUp (%d).png", i + 1);//Resource\MainCharacter\swing\swing_jumping\right\Spr_SNB_SwingJumpUp (1).png"
				if (FAILED(mc.SwingJumpingSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\swing\\swing_jumping\\left\\Spr_SNB_SwingJumpUp (%d).png", i + 1);//Resource\MainCharacter\swing\swing_jumping\left\Spr_SNB_SwingJumpUp (1).png"
				if (FAILED(mc.SwingJumpingSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < WALLCLIMBUP_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\wallclimb\\climb_up\\right\\Spr_SNB_WallClimbUp (lp) (%d).png", i + 1);//Resource\MainCharacter\wallclimb\climb_up\right\Spr_SNB_WallClimbUp (lp) (1).png"
				if (FAILED(mc.ClimbUpSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\wallclimb\\climb_up\\left\\Spr_SNB_WallClimbUp (lp) (%d).png", i + 1);//Resource\MainCharacter\wallclimb\climb_up\left\Spr_SNB_WallClimbUp (lp) (1).png"
				if (FAILED(mc.ClimbUpSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < WALLCLIMBDOWN_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\wallclimb\\climb_down\\right\\Spr_SNB_WallClimbDown (lp) (%d).png", i + 1);//Resource\MainCharacter\wallclimb\climb_down\right\Spr_SNB_WallClimbDown (lp) (1).png"
				if (FAILED(mc.ClimbDownSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\wallclimb\\climb_down\\left\\Spr_SNB_WallClimbDown (lp) (%d).png", i + 1);//Resource\MainCharacter\wallclimb\climb_down\left\Spr_SNB_WallClimbDown (lp) (1).png"
				if (FAILED(mc.ClimbDownSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < EXHOLDINGBACK_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\holding\\right\\Spr_SNB_ExcHolding_Back (lp) (%d).png", i + 1);//Resource\MainCharacter\holding\right\Spr_SNB_ExcHolding_Back (lp) (1).png"
				if (FAILED(mc.ExHoldingBackSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\holding\\left\\Spr_SNB_ExcHolding_Back (lp) (%d).png", i + 1);//Resource\MainCharacter\holding\left\Spr_SNB_ExcHolding_Back (lp) (1).png"
				if (FAILED(mc.ExHoldingBackSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < DAMAGED_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\damaged\\right\\Spr_SNB_Damaged (%d).png", i + 1);//Resource\MainCharacter\damaged\right\Spr_SNB_Damaged (1).png"
				if (FAILED(mc.DamagedSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\damaged\\left\\Spr_SNB_Damaged (%d).png", i + 1);//Resource\MainCharacter\damaged\left\Spr_SNB_Damaged (1).png"
				if (FAILED(mc.DamagedSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}
			if (i < DEATH_MAXFRAME) {
				//오른쪽 
				wsprintf(filepath, L"Resource\\MainCharacter\\death\\right\\Spr_SNB_Death (%d).png", i + 1);//Resource\MainCharacter\death\right\Spr_SNB_Death (1).png"
				if (FAILED(mc.DeathSprites_Right[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
				//왼쪽
				wsprintf(filepath, L"Resource\\MainCharacter\\death\\left\\Spr_SNB_Death (%d).png", i + 1);//Resource\MainCharacter\death\left\Spr_SNB_Death (1).png"
				if (FAILED(mc.DeathSprites_Left[i].Load(filepath))) { //로드 경고 뛰우기 위한 방법
					MessageBox(hWnd, filepath, L"이미지를 찾을 수 없습니다.", MB_OK); // 로드 실패 시 디버깅을 위해 경고창을 띄우도록 설정
				}
			}

		}//sprite load for문 끝




		gameStart = true;
		break;
	case WM_SIZE:
		GetClientRect(hWnd, &rt);
		cam.sizeX = rt.right, cam.sizeY = rt.bottom;
		HowManyRow = cam.sizeY / PLATFORMSIZE + 2, HowManyCol = cam.sizeX / PLATFORMSIZE + 2;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_SPACE && !keys[VK_SPACE] && (mc.isGrounded || mc.state == ONWALL)) {
			mc.canjump = true;
		}
		keys[wParam] = true;
		// 주인공 상태 - 달리기 시작
		if ((wParam == 'A' || wParam == 'D') && mc.state != ISSWINGING && mc.isGrounded) {
			if (mc.state != ISRUNNING && mc.state != ISSTARTINGRUN) { // 이미 달리고 있거나(ISRUNNING), 출발하는 중(ISSTARTINGRUN)이 아닐 때만 상태를 바꾸도록
				SetCharacterState(ISSTARTINGRUN);//mc.state = ISSTARTINGRUN; --> startingrun에서 running으로 바뀌게 프레임이 끝나면 변경함(프레임 갱신 부분에서)
			}
			
		}

		break;
	case WM_KEYUP:
		keys[wParam] = false;
		// 주인공 상태 - 달리기 멈추기
		if (wParam == 'A' && mc.facingDirection == FACING_LEFT && mc.isGrounded) {
			if (mc.state == ISRUNNING || mc.state == ISSTARTINGRUN) { //ISSTARTINGRUN 상태일 때 또는 ISRUNNING 상태일 때도 키를 떼어도 멈출 수 있도록
				SetCharacterState(ISSTOPPING);//mc.state = ISSTOPPING;  --> stoprun에서 standing으로 바뀌게 프레임이 끝나면 변경함(프레임 갱신 부분에서)
			}
			
		}
		else if (wParam == 'D' && mc.facingDirection == FACING_RIGHT && mc.isGrounded) {
			if (mc.state == ISRUNNING || mc.state == ISSTARTINGRUN) {  //ISSTARTINGRUN 상태일 때 또는 ISRUNNING 상태일 때도 키를 떼어도 멈출 수 있도록
				SetCharacterState(ISSTOPPING);//mc.state = ISSTOPPING;  --> stoprun에서 standing으로 바뀌게 프레임이 끝나면 변경함(프레임 갱신 부분에서)
			}
		}

		break;
	case WM_LBUTTONDOWN:			// 좌클릭
		mx = LOWORD(lParam), my = HIWORD(lParam);
		{
			float mouseX = mx + cam.x, mouseY = my + cam.y;
			float centerX = mc.x + (MCHORIZONALSIZE / 2), centerY = mc.y + (MCVERTICALSIZE / 2);

			// 기울기 (방향) 구하기 위한 요소
			dx = mouseX - centerX, dy = mouseY - centerY;
			float anchDist = sqrt(dx * dx + dy * dy);
			float dirX = dx / anchDist, dirY = dy / anchDist;
			float curDist = 0;
			// 광선 발사 플랫폼 체크
			float step = 5;
			int oldRow = centerY / PLATFORMSIZE, oldCol = centerX / PLATFORMSIZE;
			while (curDist < MAXROPESHOOTLEN) {
				float curX = centerX + (dirX * curDist), curY = centerY + (dirY * curDist);
				int curRow = curY / PLATFORMSIZE, curCol = curX / PLATFORMSIZE;

				// 배열 밖으로 나가면 종료
				if (curRow < 0 || curRow >= PLATFORMMAXROW || curCol < 0 || curCol >= PLATFORMMAXCOL) {
					break;
				}

				if (platforms[curRow][curCol].isPlatform) {
					if (curRow > oldRow) {
						if (platforms[curRow][curCol].type[WALL_TOP] == WALL_CANHOOK) {
							anch.x = curX, anch.y = curY;
							float ropeDx = curX - centerX, ropeDy = curY - centerY;
							anch.length = sqrt(ropeDx * ropeDx + ropeDy * ropeDy);
							if (anch.length > MAXROPELEN) anch.length = MAXROPELEN;
							SetCharacterState(ISSWINGING);//mc.state = ISSWINGING;
						}
						if (platforms[curRow][curCol].type[WALL_BOTTOM] != WALL_CONNECT) break;
					}
					if (curRow < oldRow) {
						if (platforms[curRow][curCol].type[WALL_BOTTOM] == WALL_CANHOOK) {
							anch.x = curX, anch.y = curY;
							float ropeDx = curX - centerX, ropeDy = curY - centerY;
							anch.length = sqrt(ropeDx * ropeDx + ropeDy * ropeDy);
							if (anch.length > MAXROPELEN) anch.length = MAXROPELEN;
							SetCharacterState(ISSWINGING);//mc.state = ISSWINGING;
						}
						if (platforms[curRow][curCol].type[WALL_BOTTOM] != WALL_CONNECT) break;
					}
					if (curCol < oldCol) {
						if (platforms[curRow][curCol].type[WALL_RIGHT] == WALL_CANHOOK) {
							anch.x = curX, anch.y = curY;
							float ropeDx = curX - centerX, ropeDy = curY - centerY;
							anch.length = sqrt(ropeDx * ropeDx + ropeDy * ropeDy);
							if (anch.length > MAXROPELEN) anch.length = MAXROPELEN;
							SetCharacterState(ISSWINGING);//mc.state = ISSWINGING;
						}
						if (platforms[curRow][curCol].type[WALL_BOTTOM] != WALL_CONNECT) break;
					}
					if (curCol > oldCol) {
						if (platforms[curRow][curCol].type[WALL_LEFT] == WALL_CANHOOK) {
							anch.x = curX, anch.y = curY;
							float ropeDx = curX - centerX, ropeDy = curY - centerY;
							anch.length = sqrt(ropeDx * ropeDx + ropeDy * ropeDy);
							if (anch.length > MAXROPELEN) anch.length = MAXROPELEN;
							SetCharacterState(ISSWINGING);//mc.state = ISSWINGING;
						}
						if (platforms[curRow][curCol].type[WALL_BOTTOM] != WALL_CONNECT) break;
					}
				}
				curDist += step;
				oldRow = curRow;
				oldCol = curCol;
			}
		}

		if (mc.state == ISSWINGING) {
			float centerX = mc.x + (MCHORIZONALSIZE / 2);
			float centerY = mc.y + (MCVERTICALSIZE / 2);
			float dx = centerX - anch.x;
			float dy = centerY - anch.y;
			float currentDist = sqrt(dx * dx + dy * dy);
			if (currentDist > anch.length) {
				float ratio = anch.length / currentDist;
				float targetcenterX = anch.x + dx * ratio;
				float targetcenterY = anch.y + dy * ratio;
				mc.x = targetcenterX - (MCHORIZONALSIZE / 2);
				mc.y = targetcenterY - (MCVERTICALSIZE / 2);
				mc.oldX = mc.x;
				mc.oldY = mc.y;
			}
		}

		break;
	case WM_LBUTTONUP:
		if ((mc.oldY - mc.y) > 10) SetCharacterState(ISSWINGJUMPING);//mc.state = ISSWINGJUMPING;
		else SetCharacterState(ISFALLING);//mc.state = ISFALLING;
		break;
	case WM_MOUSEMOVE:
		mx = LOWORD(lParam), my = HIWORD(lParam);
		break;
	case WM_PAINT:
	{
		GetClientRect(hWnd, &rt);
		hDC = BeginPaint(hWnd, &ps);
		mDC = CreateCompatibleDC(hDC);
		hBitmap = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
		SelectObject(mDC, (HBITMAP)hBitmap);

		HPEN hPen;
		HBRUSH hBrush;
		Rectangle(mDC, rt.left, rt.top, rt.right, rt.bottom);
		hPen = (HPEN)GetStockObject(BLACK_PEN);

		// (임시) 배경 그리기
		hPen = CreatePen(0, 0, RGB(150, 180, 255));
		SelectObject(mDC, hPen);
		hBrush = CreateSolidBrush(RGB(150, 180, 255));
		SelectObject(mDC, hBrush);
		Rectangle(mDC, rt.left, rt.top, rt.right, rt.bottom);
		DeleteObject(hPen);
		DeleteObject(hBrush);

		// ==================================================
		// 플랫폼 그리기
		// ==================================================
		int camrow, camcol;
		// 카메라 행, 열 구하기
		camrow = cam.y / PLATFORMSIZE, camcol = cam.x / PLATFORMSIZE;
		// 카메라 안쪽 플랫폼만 그리기
		for (int i = camrow; i < camrow + HowManyRow; i++) {
			for (int j = camcol; j < camcol + HowManyCol; j++) {
				// 플랫폼이 아니면 (비어있는 공간이면) 패스
				if (platforms[i][j].isPlatform == false) continue;

				// 행, 열로 좌표 구하기
				float x = j * PLATFORMSIZE, y = i * PLATFORMSIZE;

				// 검은색 정사각형 땅 그리기
				hPen = CreatePen(0, 0, RGB(10, 10, 10));
				SelectObject(mDC, hPen);
				hBrush = CreateSolidBrush(RGB(10, 10, 10));
				SelectObject(mDC, hBrush);
				Rectangle(mDC, x - cam.x, y - cam.y, x + PLATFORMSIZE - cam.x, y + PLATFORMSIZE - cam.y);
				DeleteObject(hPen);
				DeleteObject(hBrush);

				// 플랫폼 겉 면 그리기 (우선순위 - CANHOOK이 가장 위에 보이게 마지막에 그림)
				float x1, y1, x2, y2;
				for (int k = 0; k < 4; k++) {
					if (k == WALL_TOP) x1 = x, y1 = y, x2 = x + PLATFORMSIZE, y2 = y;
					else if (k == WALL_RIGHT) x1 = x + PLATFORMSIZE, y1 = y, x2 = x + PLATFORMSIZE, y2 = y + PLATFORMSIZE;
					else if (k == WALL_BOTTOM) x1 = x, y1 = y + PLATFORMSIZE, x2 = x + PLATFORMSIZE, y2 = y + PLATFORMSIZE;
					else if (k == WALL_LEFT) x1 = x, y1 = y, x2 = x, y2 = y + PLATFORMSIZE;
					if (platforms[i][j].type[k] == WALL_CANNOTHOOK) {
						hPen = CreatePen(0, 2, RGB(0, 255, 0));
					}
					else if (platforms[i][j].type[k] == WALL_DAMAGE) {
						hPen = CreatePen(0, 2, RGB(255, 0, 0));
					}
					else hPen = CreatePen(0, 0, RGB(10, 10, 10));
					SelectObject(mDC, hPen);
					// 플랫폼 면 선 긋기
					MoveToEx(mDC, x1 - cam.x, y1 - cam.y, NULL);
					LineTo(mDC, x2 - cam.x, y2 - cam.y);
					// 펜 초기화
					DeleteObject(hPen);
					SelectObject(mDC, GetStockObject(BLACK_PEN));
				}
				for (int k = 0; k < 4; k++) {
					if (k == WALL_TOP) x1 = x, y1 = y, x2 = x + PLATFORMSIZE, y2 = y;
					else if (k == WALL_RIGHT) x1 = x + PLATFORMSIZE, y1 = y, x2 = x + PLATFORMSIZE, y2 = y + PLATFORMSIZE;
					else if (k == WALL_BOTTOM) x1 = x, y1 = y + PLATFORMSIZE, x2 = x + PLATFORMSIZE, y2 = y + PLATFORMSIZE;
					else if (k == WALL_LEFT) x1 = x, y1 = y, x2 = x, y2 = y + PLATFORMSIZE;
					if (platforms[i][j].type[k] == WALL_CANHOOK) {
						hPen = CreatePen(0, 2, RGB(255, 255, 0));
					}
					else hPen = CreatePen(0, 0, RGB(10, 10, 10));
					SelectObject(mDC, hPen);
					// 플랫폼 면 선 긋기
					MoveToEx(mDC, x1 - cam.x, y1 - cam.y, NULL);
					LineTo(mDC, x2 - cam.x, y2 - cam.y);
					// 펜 초기화
					DeleteObject(hPen);
					SelectObject(mDC, GetStockObject(BLACK_PEN));
				}
			}
		}
		// ==================================================
		// 적 그리기
		// ==================================================



#define ONWALL 10
		// ==================================================
		// 주인공 그리기
		// ==================================================
		//주인공 그릴 위치
		int posx = (int)(mc.x - cam.x);
		int posy = (int)(mc.y - cam.y);
		//그리는 크기

		//현재 프레임
		int frame = mc.currentFrame;
		if (mc.facingDirection == FACING_RIGHT) {
			switch (mc.state) {
			case ISSTANDING: {
				mc.StandingSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSTARTINGRUN: {
				mc.StartRunSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISRUNNING: {

				////running 에서 8번 이미지가 누락되어있어서 그거 스킵 배열인덱스 7번
				//if (mc.state == ISRUNNING && frame == 7) {
				//	break;
				//}
				mc.RunningSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSTOPPING: {
				mc.StopRunSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISJUMPING: {
				mc.JumpingSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
						  /*	case ISSTARTFALL: {
								  mc.StartFallSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }*/
			case ISFALLING: {
				mc.FallingSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISLANDING: {
				mc.LandingSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSWINGING: {
				mc.SwingingSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSWINGJUMPING: {
				mc.SwingJumpingSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISDAMAGED: {
				mc.DamagedSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
						  /*	case ISCLIMINGBUP: {
								  mc.ClimbUpSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }
							  case ISCLIMBINGDOWN: {
								  mc.ClimbDownSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }
							  case ISHOLDING: {
								  mc.ExHoldingBackSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }
							  case ISDEATH: {
								  mc.DeathSprites_Right[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }*/
			default: {
				hBrush = CreateSolidBrush(RGB(255, 0, 0));
				SelectObject(mDC, hBrush);
				Ellipse(mDC, mc.x - cam.x, mc.y - cam.y, mc.x - cam.x + MCHORIZONALSIZE, mc.y - cam.y + MCVERTICALSIZE);
				DeleteObject(hBrush);
				break;
			}

			}//switch문 끝
		}
		else {//왼쪽 방향
			switch (mc.state) {
			case ISSTANDING: {
				mc.StandingSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSTARTINGRUN: {
				mc.StartRunSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISRUNNING: {

				////running 에서 8번 이미지가 누락되어있어서 그거 스킵 배열인덱스 7번
				//if (mc.state == ISRUNNING && frame == 7) {
				//	break;
				//}
				mc.RunningSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSTOPPING: {
				mc.StopRunSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISJUMPING: {
				mc.JumpingSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
						  /*	case ISSTARTFALL: {
								  mc.StartFallSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }*/
			case ISFALLING: {
				mc.FallingSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISLANDING: {
				mc.LandingSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSWINGING: {
				mc.SwingingSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISSWINGJUMPING: {
				mc.SwingJumpingSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
			case ISDAMAGED: {
				mc.DamagedSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
				break;
			}
						  /*	case ISCLIMINGBUP: {
								  mc.ClimbUpSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }
							  case ISCLIMBINGDOWN: {
								  mc.ClimbDownSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }
							  case ISHOLDING: {
								  mc.ExHoldingBackSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }
							  case ISDEATH: {
								  mc.DeathSprites_Left[frame].Draw(mDC, posx, posy, MCHORIZONALSIZE, MCVERTICALSIZE);
								  break;
							  }*/
			default: {
				hBrush = CreateSolidBrush(RGB(255, 0, 0));
				SelectObject(mDC, hBrush);
				Ellipse(mDC, mc.x - cam.x, mc.y - cam.y, mc.x - cam.x + MCHORIZONALSIZE, mc.y - cam.y + MCVERTICALSIZE);
				DeleteObject(hBrush);
				break;
			}
			}//switch문 끝
		}//주인공 좌/우if문 끝


			// 상태 확인용
		TCHAR tchar[10];
		wsprintf(tchar, L"%d %d", mc.state, mc.isGrounded);
		TextOut(mDC, 10, 10, tchar, lstrlen(tchar));

		// ==================================================
		// 사슬 그리기
		// ==================================================
		if (mc.state == ISSWINGING) {
			MoveToEx(mDC, mc.x + (MCHORIZONALSIZE / 2) - cam.x, mc.y + (MCVERTICALSIZE / 2) - cam.y, NULL);
			LineTo(mDC, anch.x - cam.x, anch.y - cam.y);
		}

		// ==================================================
		// 총알 그리기
		// ==================================================
		for (int i = 0; i < bulletsNum; i++) {
			if (bullets[i].type == BULLET_SMALL) {

			}
			else if (bullets[i].type == BULLET_BIG) {

			}
		}



		BitBlt(hDC, 0, 0, rt.right, rt.bottom, mDC, 0, 0, SRCCOPY);
		DeleteDC(mDC);
		DeleteObject(hBitmap);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// =============================================== GameUpdateProc ===========================
void GameUpdateProc(HWND hWnd)
{
	if (!gameStart) return;

	// 베를레 적분법; 위치 = 현재 위치 + (현재 위치 - 이전 위치) * 저항 + 가속도
	float tempX = mc.x;
	float tempY = mc.y;

	int leftCol, rightCol, topRow, bottomRow;
	leftCol = mc.x / PLATFORMSIZE;
	rightCol = (mc.x + MCHORIZONALSIZE - 1) / PLATFORMSIZE;
	topRow = tempY / PLATFORMSIZE;
	bottomRow = (tempY + MCVERTICALSIZE - 1) / PLATFORMSIZE;

	// 가속도 설정, Y방향 중력 가속도 반영
	mc.accX = 0, mc.accY = GRAVITY;

	if (mc.state != ONWALL) {
		// 왼쪽 이동
		if (keys['A']) {
			mc.facingDirection = FACING_LEFT;
			mc.accX -= MCMOVESPEED;
		}
		// 오른쪽 이동
		if (keys['D']) {
			mc.facingDirection = FACING_RIGHT;
			mc.accX += MCMOVESPEED;
		}
	}
	// 벽타기 중
	if (mc.state == ONWALL) {
		// 위 이동
		if (keys['W']) {
			mc.y -= MCWALLCIMBSPEED;
			mc.climbingDirection = CLIMBING_UP;

			// 벽 끝까지 올라가면 점프
			if ((mc.facingDirection == FACING_LEFT && (!platforms[topRow][leftCol - 1].isPlatform || platforms[topRow][leftCol - 1].type[WALL_RIGHT] != WALL_CANHOOK))
				|| (mc.facingDirection == FACING_RIGHT && (!platforms[topRow][rightCol + 1].isPlatform || platforms[topRow][rightCol + 1].type[WALL_LEFT] != WALL_CANHOOK))) {
				mc.oldY = mc.y + MCJUMPACC;
				mc.isGrounded = false;
				mc.canjump = false;

				// 주인공 상태 변화 - 점프 중
				SetCharacterState(ISJUMPING);//mc.state = ISJUMPING;
			}
		}
		// 아래 이동
		if (keys['S']) {
			mc.y += MCWALLCIMBSPEED;
			mc.climbingDirection = CLIMBING_DOWN;

			// 벽 아래가 없으면 떨어짐
			if ((mc.facingDirection == FACING_LEFT && (!platforms[topRow][leftCol - 1].isPlatform || platforms[topRow][leftCol - 1].type[WALL_RIGHT] != WALL_CANHOOK))
				|| (mc.facingDirection == FACING_RIGHT && (!platforms[topRow][rightCol + 1].isPlatform || platforms[topRow][rightCol + 1].type[WALL_LEFT] != WALL_CANHOOK))) {
				SetCharacterState(ISFALLING);//mc.state = ISFALLING;
			}
		}
		// 벽에 붙어서 가만히 있으면
		if (abs(mc.oldY - mc.y) < 1) {
			mc.climbingDirection = NO_CLIMBING;
		}
		tempY = mc.y;
	}

	// 점프
	if ((mc.isGrounded || mc.state == ONWALL) && mc.canjump) {
		mc.oldY = mc.y + MCJUMPACC;
		mc.isGrounded = false;
		mc.canjump = false;

		if (mc.state == ONWALL) {
			if (mc.facingDirection == FACING_LEFT) mc.accX += MCJUMPACC / 2;
			if (mc.facingDirection == FACING_RIGHT) mc.accX -= MCJUMPACC / 2;
		}
		// 주인공 상태 변화 - 점프 중
		if (mc.state != ISSWINGING) SetCharacterState(ISJUMPING); //mc.state = ISJUMPING;
	}

	// 로프 매달려 있을 때 저항 줄임
	float frictionX, frictionY;
	if (mc.state == ISSWINGING) frictionX = 0.99;
	else frictionX = 0.85;
	if (mc.state == ISSWINGING) frictionY = 0.999;
	else frictionY = 0.95;

	// 베를레 적분 위치 계산
	if (mc.state != ONWALL) {
		mc.x = mc.x + (mc.x - mc.oldX) * frictionX + mc.accX;
		mc.y = mc.y + (mc.y - mc.oldY) * frictionY + mc.accY;
	}

	// 로프 걸려있을 때 위치 보정
	if (mc.state == ISSWINGING) {
		float centerX = mc.x + (MCHORIZONALSIZE / 2);
		float centerY = mc.y + (MCVERTICALSIZE / 2);
		float dx = centerX - anch.x;
		float dy = centerY - anch.y;
		float currentDist = sqrt(dx * dx + dy * dy);
		if (currentDist > anch.length) {
			float ratio = anch.length / currentDist;
			float targetcenterX = anch.x + dx * ratio;
			float targetcenterY = anch.y + dy * ratio;
			mc.x = targetcenterX - (MCHORIZONALSIZE / 2);
			mc.y = targetcenterY - (MCVERTICALSIZE / 2);
		}
	}

	// 맵 밖으로 안나가게 막기
	if (mc.x < 0) mc.x = 0;
	if (mc.x >= (PLATFORMMAXCOL * PLATFORMSIZE) - MCHORIZONALSIZE) mc.x = (PLATFORMMAXCOL * PLATFORMSIZE) - MCHORIZONALSIZE - 1;
	if (mc.y < 0) mc.y = 0;

	// ==================================================
	// X 좌표 계산
	// ==================================================

	// 주인공 위치한 행, 열 구하기
	leftCol = mc.x / PLATFORMSIZE;
	rightCol = (mc.x + MCHORIZONALSIZE - 1) / PLATFORMSIZE;
	topRow = tempY / PLATFORMSIZE;
	bottomRow = (tempY + MCVERTICALSIZE - 1) / PLATFORMSIZE;

	if (mc.state != ONWALL) {
		// 왼쪽 벽 체크
		if (platforms[topRow][leftCol].isPlatform || platforms[bottomRow][leftCol].isPlatform) {
			mc.x = (leftCol + 1) * PLATFORMSIZE;
			tempX = mc.x;
			if (keys['A'] && mc.state != ISSWINGING && platforms[topRow][leftCol].type[WALL_RIGHT] == WALL_CANHOOK) SetCharacterState(ONWALL);//mc.state = ONWALL;
		}

		// 오른쪽 벽 체크
		if (platforms[topRow][rightCol].isPlatform || platforms[bottomRow][rightCol].isPlatform) {
			mc.x = (rightCol * PLATFORMSIZE) - MCHORIZONALSIZE;
			tempX = mc.x;
			if (keys['D'] && mc.state != ISSWINGING && platforms[topRow][rightCol].type[WALL_LEFT] == WALL_CANHOOK) SetCharacterState(ONWALL);//mc.state = ONWALL;
		}
	}

	// ==================================================
	// Y 좌표 계산
	// ==================================================

	// 주인공 위치한 행, 열 구하기
	leftCol = mc.x / PLATFORMSIZE;
	rightCol = (mc.x + MCHORIZONALSIZE - 1) / PLATFORMSIZE;
	topRow = mc.y / PLATFORMSIZE;
	bottomRow = (mc.y + MCVERTICALSIZE) / PLATFORMSIZE;

	// 바닥 체크
	if (platforms[bottomRow][leftCol].isPlatform || platforms[bottomRow][rightCol].isPlatform) {
		mc.y = bottomRow * PLATFORMSIZE - MCVERTICALSIZE;
		tempY = mc.y;
		// ISFALLING -> ISLANDING
		if (mc.state == ISFALLING && !mc.isGrounded) SetCharacterState(ISLANDING);//mc.state = ISLANDING;
		mc.isGrounded = true;
		if (mc.state == ONWALL) {
			SetCharacterState(ISSTANDING);//mc.state = ISSTANDING;
		}
	}
	else {
		mc.isGrounded = false;
	}

	// 천장 체크
	if (platforms[topRow][leftCol].isPlatform || platforms[topRow][rightCol].isPlatform) {
		mc.y = (topRow + 1) * PLATFORMSIZE;
		tempY = mc.y;
	}

	// 주인공 상태 - 떨어지는 중
	if ((mc.y - mc.oldY) > 0 && !mc.isGrounded && mc.state != ISSWINGING && mc.state != ONWALL) {
		SetCharacterState(ISFALLING);//mc.state = ISFALLING;
	}
	// 주인공 상태 - 가만히 서있음																										//mc.state != ISSTOPPING 추가 //stopping 하고 standing하도록
	if (abs(mc.x - mc.oldX) < 0.01f && mc.isGrounded && mc.state != ISSWINGING && mc.state != ISLANDING && mc.state != ONWALL && mc.state != ISSTOPPING) { //(mc.x - mc.oldX) == 0 --> abs(mc.x - mc.oldX)로 변경 이유 절댓값을 이용해야하고, 실수는 0이 될수 없음
		mc.x = mc.oldX; // 미세하게 움직이는 물리 좌표를 완전히 고정
		SetCharacterState(ISSTANDING);//mc.state = ISSTANDING;
	}

	// 이전 위치 oldX, oldY 갱신
	mc.oldX = tempX;
	mc.oldY = tempY;

	// ==================================================
	// 카메라 좌표 계산
	// ==================================================

	cam.x = mc.oldX - (cam.sizeX / 2);
	cam.y = mc.oldY - (cam.sizeY / 2);

	// 카메라 위치 맵 안으로 고정
	if (cam.x <= 0) cam.x = 0;
	if (cam.x + cam.sizeX >= PLATFORMMAXCOL * PLATFORMSIZE) cam.x = PLATFORMMAXCOL * PLATFORMSIZE - cam.sizeX;
	if (cam.y <= 0) cam.y = 0;
	if (cam.y + cam.sizeY >= PLATFORMMAXROW * PLATFORMSIZE) cam.y = PLATFORMMAXROW * PLATFORMSIZE - cam.sizeY;

	// ==================================================
	// 적 움직임
	// ==================================================
	
	// trooper
	for (int i = 0; i < troopersNum; i++) {
		if (trooper[i].activated) {
			if (Distance(mc.x, mc.y, trooper[i].x, trooper[i].y) > 1000) {
				trooper[i].activated = false;
			}
			// 조준 - 공격
		}
		else {
			if (Distance(mc.x, mc.y, trooper[i].x, trooper[i].y) < 1000) {
				trooper[i].activated = true;
			}
		}
	}

	// turret
	for (int i = 0; i < turretsNum; i++) {
		if (turret[i].activated) {
			if (Distance(mc.x, mc.y, turret[i].x, turret[i].y) > 1000) {
				turret[i].activated = false;
			}
			// 조준 - 공격
		}
		else {
			if (Distance(mc.x, mc.y, turret[i].x, turret[i].y) < 1000) {
				turret[i].activated = true;
			}
		}
	}

	// defender
	for (int i = 0; i < defendersNum; i++) {
		if (defender[i].activated) {
			if (Distance(mc.x, mc.y, defender[i].x, defender[i].y) > 1000) {
				defender[i].activated = false;
			}
			// 조준 - 공격
		}
		else {
			if (Distance(mc.x, mc.y, defender[i].x, defender[i].y) < 1000) {
				defender[i].activated = true;
			}
		}
	}


	// ==================================================
	// 총알 움직임
	// ==================================================
	for (int i = 0; i < bulletsNum; i++) {
		int bulletRow = bullets[i].y / PLATFORMSIZE, bulletCol = bullets[i].x / PLATFORMSIZE;
		if (bullets[i].type == BULLET_SMALL) {
			// 이동
			


			// 벽에 부딪히면 삭제
			if (platforms[bulletRow][bulletCol].isPlatform || isOutMap(bullets[i].x, bullets[i].y)) {
				for (int j = i; j < bulletsNum - 1; j++) {
					bullets[j] = bullets[j + 1];
				}
				i--;
			}
		}
		else if (bullets[i].type == BULLET_BIG) {

		}
	}

	// ==================================================
	// 애니메이션 프레임 갱신
	// ==================================================
	DWORD currentTime = timeGetTime();
	if (currentTime - mc.lastAnimTime >= mc.animDelay) { // 설정한 딜레이(ms)보다 시간이 더 흘렀다면 다음 프레임으로
		mc.currentFrame++; // 다음 프레임으로 이동



		// 마지막 프레임에 도달하면 다시 처음으로 루프
		if (mc.currentFrame >= mc.maxFrame) {
			mc.currentFrame = 0;

			// 애니메이션이 끝나면 자동으로 다음 상태로 넘어가야 하는 상태들 처리
			if (mc.state == ISSTARTINGRUN) {
				SetCharacterState(ISRUNNING);
			}
			else if (mc.state == ISSTOPPING) {
				SetCharacterState(ISSTANDING);
			}
			else if (mc.state == ISLANDING) {
				SetCharacterState(ISSTANDING);
			}


		}
		mc.lastAnimTime = currentTime; // 시간 갱신
	}


	InvalidateRect(hWnd, NULL, FALSE);
}

float Distance(float x1, float y1, float x2, float y2) {
	float dx = x1 - x2, dy = y1 - y2;
	return sqrt(dx * dx + dy * dy);
}

bool isOutMap(float x, float y) {
	if (x < 0) return true;
	if (y < 0) return true;
	if (x > PLATFORMMAXCOL * PLATFORMSIZE) return true;
	if (y > PLATFORMMAXROW * PLATFORMSIZE) return true;
	return false;
}

//주인공의 상태를 변경하는 함수 //상태당 애니메이션에 필요한 설정도 같이함
void SetCharacterState(int newState)
{
	// 이미 해당 상태라면 변경하지 않음 (프레임이 도중에 0으로 리셋되는 것 방지)
	if (mc.state == newState) return;

	mc.state = newState;	//플레이어의 상태 변경
	mc.currentFrame = 0; // 새로운 애니메이션을 위해 프레임 리셋
	mc.lastAnimTime = timeGetTime(); // 타이머 리셋

	//// 상태별 애니메이션 설정 부여 //애니메이션 프레임 간격 // 애니메이션 당 최대 프레임 개수

	switch (newState) {
	case ISSTANDING:
		mc.maxFrame = STANDING_MAXFRAME;
		mc.animDelay = 100; // 대기 상태는 조금 느긋하게 (0.12초 간격)
		break;
	case ISSTARTINGRUN:
		mc.maxFrame = STARTRUN_MAXFRAME;
		mc.animDelay = 50;
		break;
	case ISRUNNING:
		mc.maxFrame = RUNNING_MAXFRAME;
		mc.animDelay = 50;  // 달리기는 빠르게 (0.05초 간격)
		break;
	case ISSTOPPING:
		mc.maxFrame = STOPRUN_MAXFRAME;
		mc.animDelay = 50;
		break;
	case ISJUMPING:
		mc.maxFrame = JUMPING_MAXFRAME;
		mc.animDelay = 100;
		break;
	case ISLANDING:
		mc.maxFrame = LANDING_MAXFRAME;
		mc.animDelay = 100;
		break;
	case ISFALLING:
		mc.maxFrame = FALLING_MAXFRAME;
		mc.animDelay = 100;
		break;
	/*case ONWALL:
		mc.maxFrame = ;
		mc.animDelay = 100;
		break;*/
	case ISSWINGING:
		mc.maxFrame = SWINGING_MAXFRAME;
		mc.animDelay = 100;
		break;
	case ISSWINGJUMPING:
		mc.maxFrame = SWINGJUMP_MAXFRAME;
		mc.animDelay = 100;
		break;
	case ISDAMAGED:
		mc.maxFrame = DAMAGED_MAXFRAME;
		mc.animDelay = 100;
		break;
	}
}