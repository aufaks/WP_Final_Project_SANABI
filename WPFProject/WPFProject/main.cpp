#include <Windows.h>
#include <tchar.h>
#include <random>
#include <math.h>
//#include "resource.h"
using namespace std;

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

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

#define GRAVITY 0.9
#define MCMOVESPEED 0.9
#define MCJUMPACC 16
#define MAXROPELEN 500

void CALLBACK GameUpdateProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

#define FACING_LEFT 0
#define FACING_RIGHT 1

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

struct MAINCHARACTER{
	float x, y;
	float oldX, oldY, accX, accY;
	int hp;
	int state;
	bool isGrounded, facingDirection, canjump;
	MAINCHARACTER() {
		x = 100, y = 500;
		oldX = 100, oldY = 500;
		accX = 0, accY = 0;
		hp = 4;
		canjump = false;
		isGrounded = false;
		state = ISSTANDING;
		facingDirection = FACING_RIGHT;
	}
};

struct ANCHOR {
	float x, y, length;
};

struct CAMERA {
	float x, y;
	float sizeX, sizeY;
};

#define ENEMY_NORMAL 0
#define ENEMY_SHIELD 1
#define ENEMY_FLYING 2

struct ENEMY {
	float x, y;
	int type;
};

struct FLOOR {
	int y = 700;
};
// 임시값
#define PLATFORMMAXROW 1000
#define PLATFORMMAXCOL 500
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

FLOOR fl;

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
		cam.x = 0, cam.y = 0;
		cam.sizeX = rt.right, cam.sizeY = rt.bottom;
		HowManyRow = (cam.sizeY / PLATFORMSIZE) + 1, HowManyCol = (cam.sizeX / PLATFORMSIZE) + 1;
		SetTimer(hWnd, 't', 10, GameUpdateProc);
		break;
	case WM_SIZE:
		GetClientRect(hWnd, &rt);
		cam.sizeX = rt.right, cam.sizeY = rt.bottom;
		HowManyRow = cam.sizeY / PLATFORMSIZE + 1, HowManyCol = cam.sizeX / PLATFORMSIZE + 1;
		break;
	case WM_KEYDOWN:
		keys[wParam] = true;
		// 주인공 상태 - 달리기 시작
		if ((wParam == 'A' || wParam == 'D') && mc.state != ISSWINGING && mc.isGrounded) {
			mc.state = ISSTARTINGRUN;
		}
		if (wParam == VK_SPACE) {
			mc.canjump = true;
		}

		break;
	case WM_KEYUP:
		keys[wParam] = false;
		// 주인공 상태 - 달리기 멈추기
		if ((wParam == 'A' || wParam == 'D') && mc.state != ISSWINGING && mc.isGrounded) {
			mc.state = ISSTOPPING;
		}

		break;
	case WM_LBUTTONDOWN:			// 좌클릭
		mx = LOWORD(lParam), my = HIWORD(lParam);
		anch.x = mx + cam.x, anch.y = my + cam.y;
		dx = mc.x + 15 - anch.x;
		dy = mc.y + 15 - anch.y;
		anch.length = sqrt(dx * dx + dy * dy);
		if (anch.length > 15) mc.state = ISSWINGING;
		break;
	case WM_LBUTTONUP:
		if ((mc.x - mc.oldX) * (mc.x - mc.oldX) > 100) mc.state = ISSWINGJUMPING;
		else mc.state = ISFALLING;
		break;
	case WM_MOUSEMOVE:
		mx = LOWORD(lParam), my = HIWORD(lParam);
		break;
	case WM_PAINT:
		GetClientRect(hWnd, &rt);
		hDC = BeginPaint(hWnd, &ps);
		mDC = CreateCompatibleDC(hDC);
		hBitmap = CreateCompatibleBitmap(hDC, rt.right, rt.bottom);
		SelectObject(mDC, (HBITMAP)hBitmap);

		HPEN hPen;
		HBRUSH hBrush;
		Rectangle(mDC, rt.left, rt.top, rt.right, rt.bottom);

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
				hBrush = CreateSolidBrush(RGB(50, 50, 50));
				SelectObject(mDC, hBrush);
				Rectangle(mDC, x - cam.x, y - cam.y, x + PLATFORMSIZE - cam.x, y + PLATFORMSIZE - cam.y);
				DeleteObject(hBrush);

				// 플랫폼 겉 면 그리기 (우선순위 - CANHOOK이 가장 위에 보이게 마지막에 그림)
				for (int k = 0; k < 4; k++) {
					if (platforms[i][j].type[k] == WALL_CANNOTHOOK) {

					}
					else if (platforms[i][j].type[k] == WALL_DAMAGE) {

					}
				}
				for (int k = 0; k < 4; k++) {
					if (platforms[i][j].type[k] == WALL_CANHOOK) {

					}
				}
			}
		}
		
		// 땅 (임시)
		MoveToEx(mDC, 0, fl.y - cam.y, NULL);
		LineTo(mDC, cam.sizeX, fl.y - cam.y);
		// ==================================================
		// 주인공 그리기
		// ==================================================
		hBrush = CreateSolidBrush(RGB(255, 0, 0));
		SelectObject(mDC, hBrush);
		Ellipse(mDC, mc.x - cam.x, mc.y - cam.y, mc.x - cam.x + 30, mc.y - cam.y + 30);
		DeleteObject(hBrush);
		TCHAR tchar[10];
		wsprintf(tchar, L"%d", mc.state);
		TextOut(mDC, 10, 10, tchar, lstrlen(tchar));

		// ==================================================
		// 사슬 그리기
		// ==================================================
		if (mc.state == ISSWINGING) {
			MoveToEx(mDC, mc.x + 15 - cam.x, mc.y + 15 - cam.y, NULL);
			LineTo(mDC, anch.x - cam.x, anch.y - cam.y);
		}

		BitBlt(hDC, 0, 0, rt.right, rt.bottom, mDC, 0, 0, SRCCOPY);
		DeleteDC(mDC);
		DeleteObject(hBitmap);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CALLBACK GameUpdateProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	// 베를레 적분법; 위치 = 현재 위치 + (현재 위치 - 이전 위치) * 저항 + 가속도
	float tempX = mc.x;
	float tempY = mc.y;
	// 가속도 설정, Y방향 중력 가속도 반영
	mc.accX = 0, mc.accY = GRAVITY;
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
	// 점프
	if (mc.canjump && mc.isGrounded) {
		mc.oldY = mc.y + MCJUMPACC;
		mc.isGrounded = false;
		mc.canjump = false;
		// 주인공 상태 변화 - 점프 중
		if (mc.state != ISSWINGING) mc.state = ISJUMPING;
	}
	// 로프 매달려 있을 때 저항 줄임
	float friction;
	if (mc.state == ISSWINGING) friction = 0.99;
	else friction = 0.88;
	// 베를레 적분 위치 계산
	mc.x = mc.x + (mc.x - mc.oldX) * friction + mc.accX;
	mc.y = mc.y + (mc.y - mc.oldY) + mc.accY;
	
	// 로프 걸려있을 때 위치 보정
	if (mc.state == ISSWINGING) {
		float centerX = mc.x + 15;
		float centerY = mc.y + 15;
		float dx = centerX - anch.x;
		float dy = centerY - anch.y;
		float currentDist = sqrt(dx * dx + dy * dy);
		if (currentDist > anch.length) {
			float ratio = anch.length / currentDist;
			float targetcenterX = anch.x + dx * ratio;
			float targetcenterY = anch.y + dy * ratio;
			mc.x = targetcenterX - 15;
			mc.y = targetcenterY - 15;
		}
	}

	// 주인공 상태 - 떨어지는 중
	if ((mc.y - mc.oldY) > 0 && mc.state != ISSWINGING) {
		mc.state = ISFALLING;
	}
	// 주인공 상태 - 가만히 서있음
	if ((mc.x - mc.oldX) == 0 && mc.isGrounded && mc.state != ISSWINGING && mc.state != ISLANDING) {
		//mc.state = ISSTANDING;
	}

	// 이전 위치 oldX, oldY 갱신
	mc.oldX = tempX;
	mc.oldY = tempY;

	// (임시) 바닥 체크
	if (mc.y > fl.y - 30) {
		mc.y = fl.y - 30;
		if (mc.state == ISSWINGING) mc.oldY = mc.y;
		if (mc.state == ISFALLING) {
			mc.state = ISLANDING;
		}
		mc.isGrounded = true;
	}
	InvalidateRect(hWnd, NULL, FALSE);
}