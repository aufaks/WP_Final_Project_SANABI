#include <Windows.h>
#include <tchar.h>
#include <math.h>
#include <fstream>
using namespace std;

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
	WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, 1200, 700, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}
//==============================================================
// CANHOOK - 노랑 // CANNOTHOOK - 초록 // DAMAGE - 빨강
//==============================================================
struct PLATFORM {
	bool isPlatform;
	// 0: connected 1: canhook 2:cannothook 3: damage
	int type[4];
	PLATFORM() {
		isPlatform = false;
		for (int i = 0; i < 4; i++) type[i] = 4;
	}
};

struct CAMERA {
	int r, c;
};

PLATFORM p[500][1000];
CAMERA c;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC, hMemDC, mDC;
	static RECT rect;
	static int mx, my;
	static BITMAP bmp;
	static HBITMAP hBitmap;

	static int srow = -1, scol = -1, sside = 0;
	static bool lb = 0, rb = 0, isp = 1;

	switch (uMsg) {
	case WM_CREATE:
		GetClientRect(hWnd, &rect);
		c.r = 0, c.c = 0;
		break;
	case WM_SIZE:
		GetClientRect(hWnd, &rect);
		break;

	case WM_KEYDOWN:
		if (wParam == 'Q') {
			//PostQuitMessage(0);
			break;
		}
		else if (wParam == VK_SPACE) {
			isp = !isp;
		}
		else if (wParam == 'P') {
			ofstream out{ "Platform_Info.txt" };
			for (int i = 0; i < 500; i++) {
				for (int j = 0; j < 1000; j++) {
					int pinfo;
					if (p[i][j].isPlatform) pinfo = p[i][j].type[0] * 1000 + p[i][j].type[1] * 100 + p[i][j].type[2] * 10 + p[i][j].type[3];
					else pinfo = 0;
					out << pinfo << " ";
				}
				out << endl;
			}
		}
		else if (wParam == 'L') {
			ifstream in{ "Platform_Info.txt" };
			for (int i = 0; i < 500; i++) {
				for (int j = 0; j < 1000; j++) {
					int pinfo;
					in >> pinfo;
					if (pinfo == 0) p[i][j].isPlatform = 0;
					else {
						p[i][j].isPlatform = true;
						p[i][j].type[0] = pinfo / 1000;
						p[i][j].type[1] = (pinfo % 1000) / 100;
						p[i][j].type[2] = (pinfo % 100) / 10;
						p[i][j].type[3] = pinfo % 10;
					}
				}
			}
		}
		else if (wParam == 'W') {
			if (c.r > 0) c.r -= 10;
		}
		else if (wParam == 'A') {
			if (c.c > 0) c.c -= 20;
		}
		else if (wParam == 'S') {
			if (c.r < 480) c.r += 10;
		}
		else if (wParam == 'D') {
			if (c.c < 960) c.c += 20;
		}
		else if (wParam == VK_UP) {
			sside = 0;
		}
		else if (wParam == VK_RIGHT) {
			sside = 1;
		}
		else if (wParam == VK_DOWN) {
			sside = 2;
		}
		else if (wParam == VK_LEFT) {
			sside = 3;
		}

		if (srow >= 0 && scol >= 0) {
			if (wParam == '0') {
				p[srow][scol].type[sside] = 4;
			}
			else if (wParam == '1') {
				p[srow][scol].type[sside] = 1;
			}
			else if (wParam == '2') {
				p[srow][scol].type[sside] = 2;
			}
			else if (wParam == '3') {
				p[srow][scol].type[sside] = 3;
			}
		}

		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_LBUTTONDOWN:			// 좌클릭
		mx = LOWORD(lParam), my = HIWORD(lParam);
		if (mx < 1000 && my < 500) {
			lb = 1;
			srow = my / 25, scol = mx / 25;
			srow += c.r, scol += c.c;
			p[srow][scol].isPlatform = 1;
		}
		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_RBUTTONDOWN:			// 좌클릭
		mx = LOWORD(lParam), my = HIWORD(lParam);
		if (mx < 1000 && my < 500) {
			rb = 1;
			srow = my / 25, scol = mx / 25;
			srow += c.r, scol += c.c;
			p[srow][scol].isPlatform = 0;
		}
		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_LBUTTONUP:
		lb = 0;
		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_RBUTTONUP:
		rb = 0;
		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_MOUSEMOVE:
		if (lb) {
			mx = LOWORD(lParam), my = HIWORD(lParam);
			if (mx < 1000 && my < 500) {
				srow = my / 25, scol = mx / 25;
				srow += c.r, scol += c.c;
				p[srow][scol].isPlatform = 1;
			}
		}
		else if (rb) {
			mx = LOWORD(lParam), my = HIWORD(lParam);
			if (mx < 1000 && my < 500) {
				srow = my / 25, scol = mx / 25;
				srow += c.r, scol += c.c;
				p[srow][scol].isPlatform = 0;
			}
		}
		else break;

		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		mDC = CreateCompatibleDC(hDC);
		hBitmap = CreateCompatibleBitmap(hDC, rect.right, rect.bottom);
		SelectObject(mDC, (HBITMAP)hBitmap);
		HBRUSH hBrush;
		HPEN hPen;
		hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		Rectangle(mDC, 0, 0, rect.right, rect.bottom);

		// 칸 나누기
		hPen = (HPEN)GetStockObject(BLACK_PEN);
		SelectObject(mDC, hPen);
		for (int i = 0; i <= 20; i++) {
			MoveToEx(mDC, 0, i * 25, NULL);
			LineTo(mDC, 1000, i * 25);
		}
		for (int i = 0; i <= 40; i++) {
			MoveToEx(mDC, i * 25, 0, NULL);
			LineTo(mDC, i * 25, 500);
		}

		// 블럭 그리기
		for (int i = c.r; i < c.r + 20; i++) {
			for (int j = c.c; j < c.c + 40; j++) {

				int x = (j - c.c) * 25, y = (i - c.r) * 25;

				if (p[i][j].isPlatform) {
					SelectObject(mDC, GetStockObject(BLACK_BRUSH));
					Rectangle(mDC, x, y, x + 25, y + 25);

					if (srow == i && scol == j) {
						hBrush = CreateSolidBrush(RGB(255, 0, 0));
						SelectObject(mDC, hBrush);
						Ellipse(mDC, x + 5, y + 5, x + 20, y + 20);
						DeleteObject(hBrush);
					}

					for (int k = 0; k < 4; k++) {
						if (p[i][j].type[k] == 4) continue;
						else if (p[i][j].type[k] == 1) hPen = CreatePen(0, 3, RGB(250, 250, 0));
						else if (p[i][j].type[k] == 2) hPen = CreatePen(0, 3, RGB(0, 250, 0));
						else if (p[i][j].type[k] == 3) hPen = CreatePen(0, 3, RGB(255, 0, 0));
						SelectObject(mDC, hPen);
						int x1, y1, x2, y2;
						if (k == 0) x1 = x, y1 = y, x2 = x + 25, y2 = y;
						else if (k == 1) x1 = x + 25, y1 = y, x2 = x + 25, y2 = y + 25;
						else if (k == 2) x1 = x, y1 = y + 25, x2 = x + 25, y2 = y + 25;
						else if (k == 3) x1 = x, y1 = y, x2 = x, y2 = y + 25;
						MoveToEx(mDC, x1, y1, NULL);
						LineTo(mDC, x2, y2);
						DeleteObject(hPen);
						SelectObject(mDC, GetStockObject(BLACK_PEN));
					}
				}
			}
		}

		TCHAR posi[50];
		wsprintf(posi, L"현재 위치(좌측 상단 기준): %d, %d", c.r, c.c);
		TextOut(mDC, 100, 550, posi, lstrlen(posi));

		TCHAR side[20];
		wsprintf(side, L"선택된 면: 위");
		if (sside == 1) wsprintf(side, L"선택된 면: 오른쪽");
		else if (sside == 2) wsprintf(side, L"선택된 면: 아래");
		else if (sside == 3) wsprintf(side, L"선택된 면: 왼쪽");
		TextOut(mDC, 500, 550, side, lstrlen(side));

		TCHAR inst[120];
		wsprintf(inst, L"1. CANHOOK - 노랑, 2. CANNOTHOOK - 초록, 3.DAMAGE - 빨강            저장: P, 로드: L");
		TextOut(mDC, 80, 600, inst, lstrlen(inst));

		BitBlt(hDC, 0, 0, rect.right, rect.bottom, mDC, 0, 0, SRCCOPY);
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