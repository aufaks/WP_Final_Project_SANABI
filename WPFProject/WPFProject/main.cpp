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
bool shouldPaint(int i);

struct MAINCHARACTER{
	float x, y;
	float oldX, oldY, accX, accY;
	int hp;
	bool isSwing, isGrounded;
	MAINCHARACTER() {
		x = 100, y = 500;
		oldX = 100, oldY = 500;
		accX = 0, accY = 0;
		hp = 4;
		isSwing = false, isGrounded = false;
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

#define PLATFORMNUM 100
#define WALL_TOP 0
#define WALL_RIGHT 1
#define WALL_BOTTOM 2
#define WALL_LEFT 3
#define WALL_CONNECT 1
#define WALL_CANHOOK 2
#define WALL_CANNOTHOOK 3
#define WALL_DAMAGE 4

struct PLATFORM {
	float x, y, w, h;
	int type[4];
};

MAINCHARACTER mc;
ANCHOR anch;
CAMERA cam;
FLOOR fl;
PLATFORM platforms[PLATFORMNUM];

bool keys[256] = { 0 };

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC, mDC;
	HBITMAP hBitmap;
	RECT rt;
	static int mx, my;

	float dx, dy;

	switch (uMsg) {
	case WM_CREATE:
		GetClientRect(hWnd, &rt);
		cam.x = 0, cam.y = 0;
		cam.sizeX = rt.right, cam.sizeY = rt.bottom;
		SetTimer(hWnd, 't', 10, GameUpdateProc);
		break;
	case WM_SIZE:
		GetClientRect(hWnd, &rt);
		cam.sizeX = rt.right, cam.sizeY = rt.bottom;
		break;
	case WM_KEYDOWN:
		keys[wParam] = true;
		break;
	case WM_KEYUP:
		keys[wParam] = false;
		break;
	case WM_LBUTTONDOWN:			// 좌클릭
		mx = LOWORD(lParam), my = HIWORD(lParam);
		anch.x = mx + cam.x, anch.y = my + cam.y;
		dx = mc.x + 25 - anch.x;
		dy = mc.y + 25 - anch.y;
		anch.length = sqrt(dx * dx + dy * dy);
		if (anch.length > 25) mc.isSwing = true;
		break;
	case WM_LBUTTONUP:
		mc.isSwing = false;
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

		HPEN hPen, oldPen;
		HBRUSH hBrush, oldBrush;
		Rectangle(mDC, rt.left, rt.top, rt.right, rt.bottom);

		// 플랫폼
		for (int i = 0; i < PLATFORMNUM; i++) {
			if (!shouldPaint(i)) continue;
			for (int k = 0; k < 4; k++) {
				if (platforms[i].type[k] == WALL_CONNECT) {

				}
			}
			for (int k = 0; k < 4; k++) {
				if (platforms[i].type[k] == WALL_CANNOTHOOK) {

				}
				else if (platforms[i].type[k] == WALL_DAMAGE) {

				}
			}
			for (int k = 0; k < 4; k++) {
				if (platforms[i].type[k] == WALL_CANHOOK) {

				}
			}

		}
		// 땅 (임시)
		MoveToEx(mDC, 0, fl.y - cam.y, NULL);
		LineTo(mDC, cam.sizeX, fl.y - cam.y);
		// 주인공
		hBrush = CreateSolidBrush(RGB(255, 0, 0));
		SelectObject(mDC, hBrush);
		Ellipse(mDC, mc.x - cam.x, mc.y - cam.y, mc.x - cam.x + 50, mc.y - cam.y + 50);
		DeleteObject(hBrush);
		// 사슬
		if (mc.isSwing) {
			MoveToEx(mDC, mc.x + 25 - cam.x, mc.y + 25 - cam.y, NULL);
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
		mc.accX -= MCMOVESPEED;
	}
	// 오른쪽 이동
	if (keys['D']) {
		mc.accX += MCMOVESPEED;
	}
	// 점프
	if (keys[VK_SPACE] && mc.isGrounded) {
		mc.oldY = mc.y + MCJUMPACC;
		mc.isGrounded = false;
	}
	// 로프 매달려 있을 때 저항 줄임
	if (mc.isSwing) mc.x = mc.x + (mc.x - mc.oldX) * 0.99 + mc.accX;
	else mc.x = mc.x + (mc.x - mc.oldX) * 0.88 + mc.accX;
	mc.y = mc.y + (mc.y - mc.oldY) + mc.accY;
	
	// 로프 걸려있을 때 위치 보정
	if (mc.isSwing) {
		float centerX = mc.x + 25;
		float centerY = mc.y + 25;
		float dx = centerX - anch.x;
		float dy = centerY - anch.y;
		float currentDist = sqrt(dx * dx + dy * dy);
		if (currentDist > anch.length) {
			float ratio = anch.length / currentDist;
			float targetcenterX = anch.x + dx * ratio;
			float targetcenterY = anch.y + dy * ratio;
			mc.x = targetcenterX - 25;
			mc.y = targetcenterY - 25;
		}
	}
	// 이전 위치 갱신
	mc.oldX = tempX;
	mc.oldY = tempY;
	// (임시) 바닥 체크
	if (mc.y > fl.y - 50) {
		mc.y = fl.y - 50;
		if (!mc.isSwing) mc.oldY = mc.y;
		mc.isGrounded = true;
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

// 플랫폼 직사각형이 카메라 안에 있는가
bool shouldPaint(int i) {
	RECT pfrt = { cam.x, cam.y, cam.x + cam.sizeX, cam.y + cam.sizeY };
	POINT pfpt[4] =
	{ {platforms[i].x, platforms[i].y}, {platforms[i].x + platforms[i].h, platforms[i].y}
	,{platforms[i].x + platforms[i].w, platforms[i].y + platforms[i].h},{platforms[i].x, platforms[i].y + platforms[i].h} };
	for (int j = 0; j < 4; j++) {
		if (PtInRect(&pfrt, pfpt[j])) return true;
	}
	return false;
}