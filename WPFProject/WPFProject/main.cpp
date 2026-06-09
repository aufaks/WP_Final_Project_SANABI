#include <Windows.h>
#include <tchar.h>
#include <fstream>
#include <random>
#include <math.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
//#include "resource.h"
using namespace std;

bool gameStart = false;
void  GameUpdateProc(HWND hWnd);
float Distance(float x1, float y1, float x2, float y2);
bool isOutMap(float x, float y);

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
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
			if (Message.message == WM_QUIT)
				break;
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		else {
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

#define GRAVITY 0.9
#define MCWALLCIMBSPEED 3
#define MCMOVESPEED 0.9
#define MCJUMPACC 18
#define MAXROPELEN 300

#define FACING_LEFT 0
#define FACING_RIGHT 1

#define MCVERTICALSIZE 30
#define MCHORIZONALSIZE 30

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
	bool isGrounded, facingDirection, canjump, isInvincible;
	MAINCHARACTER() {
		x = 100, y = 24900;
		oldX = 100, oldY = 24900;
		accX = 0, accY = 0;
		hp = 4;
		canjump = false;
		isGrounded = false;
		state = ISSTANDING;
		facingDirection = FACING_RIGHT;
		isInvincible = false;
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

struct ENEMY_TROOPER {
	float x, y, direction;
};

struct ENEMY_TURRET {
	float x, y, direction;
	int stickDirection;			// 벽에 붙어있는 방향 (어느쪽 벽에 붙어있는지)
};

struct ENEMY_DEFENDER {
	float x, y;
	bool facingDirection;		// 보고 있는 방향 (방패 방향)
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

BULLET bullets[100];
int bulletsNum = 0;

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
					int PlatformInfo;
					in >> PlatformInfo;
					if (PlatformInfo == 0) platforms[i][j].isPlatform = 0;
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
			mc.state = ISSTARTINGRUN;
		}

		break;
	case WM_KEYUP:
		keys[wParam] = false;
		// 주인공 상태 - 달리기 멈추기
		if (wParam == 'A' && mc.facingDirection == FACING_LEFT && mc.state == ISRUNNING && mc.isGrounded) {
			mc.state = ISSTOPPING;
		}
		else if (wParam == 'D' && mc.facingDirection == FACING_RIGHT && mc.state == ISRUNNING && mc.isGrounded) {
			mc.state = ISSTOPPING;
		}

		break;
	case WM_LBUTTONDOWN:			// 좌클릭
		mx = LOWORD(lParam), my = HIWORD(lParam);
		anch.x = mx + cam.x, anch.y = my + cam.y;
		dx = mc.x + MCHORIZONALSIZE - anch.x;
		dy = mc.y + MCVERTICALSIZE - anch.y;
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



		// ==================================================
		// 주인공 그리기
		// ==================================================
		hBrush = CreateSolidBrush(RGB(255, 0, 0));
		SelectObject(mDC, hBrush);
		Ellipse(mDC, mc.x - cam.x, mc.y - cam.y, mc.x - cam.x + MCHORIZONALSIZE, mc.y - cam.y + MCVERTICALSIZE);
		DeleteObject(hBrush);
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
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

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

			// 벽 끝까지 올라가면 점프
			if ((mc.facingDirection == FACING_LEFT && (!platforms[topRow][leftCol - 1].isPlatform || platforms[topRow][leftCol - 1].type[WALL_RIGHT] != WALL_CANHOOK))
				|| (mc.facingDirection == FACING_RIGHT && (!platforms[topRow][rightCol + 1].isPlatform || platforms[topRow][rightCol + 1].type[WALL_LEFT] != WALL_CANHOOK))) {
				mc.oldY = mc.y + MCJUMPACC;
				mc.isGrounded = false;
				mc.canjump = false;

				// 주인공 상태 변화 - 점프 중
				mc.state = ISJUMPING;
			}
		}
		// 아래 이동
		if (keys['S']) {
			mc.y += MCWALLCIMBSPEED;

			// 벽 아래가 없으면 떨어짐
			if ((mc.facingDirection == FACING_LEFT && (!platforms[topRow][leftCol - 1].isPlatform || platforms[topRow][leftCol - 1].type[WALL_RIGHT] != WALL_CANHOOK))
				|| (mc.facingDirection == FACING_RIGHT && (!platforms[topRow][rightCol + 1].isPlatform || platforms[topRow][rightCol + 1].type[WALL_LEFT] != WALL_CANHOOK))) {
				mc.state = ISFALLING;
			}
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
		if (mc.state != ISSWINGING) mc.state = ISJUMPING;
	}

	// 로프 매달려 있을 때 저항 줄임
	float frictionX, frictionY;
	if (mc.state == ISSWINGING) frictionX = 0.99;
	else frictionX = 0.9;
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
			if (keys['A'] && mc.state != ISSWINGING && platforms[topRow][leftCol].type[WALL_RIGHT] == WALL_CANHOOK) mc.state = ONWALL;
		}

		// 오른쪽 벽 체크
		if (platforms[topRow][rightCol].isPlatform || platforms[bottomRow][rightCol].isPlatform) {
			mc.x = (rightCol * PLATFORMSIZE) - MCHORIZONALSIZE;
			tempX = mc.x;
			if (keys['D'] && mc.state != ISSWINGING && platforms[topRow][rightCol].type[WALL_LEFT] == WALL_CANHOOK) mc.state = ONWALL;
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
		if (mc.state == ISFALLING && !mc.isGrounded) mc.state = ISLANDING;
		mc.isGrounded = true;
		if (mc.state == ONWALL) {
			mc.state = ISSTANDING;
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
		mc.state = ISFALLING;
	}
	// 주인공 상태 - 가만히 서있음
	if ((mc.x - mc.oldX) == 0 && mc.isGrounded && mc.state != ISSWINGING && mc.state != ISLANDING && mc.state != ONWALL) {
		mc.state = ISSTANDING;
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
	// 주인공과 어느정도 가까워지면 활성화
	// 다시 멀어지면 비활성화


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