#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <TCHAR.H>
#include <stdio.h>
#include <math.h>
#include "resource.h"
#include "map.h"
#include "comndlg.h"
#include "recorder.h"

#define MAX_RESOURCES 7

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK MAPSIZEProc(HWND, UINT, WPARAM, LPARAM);

HBITMAP hBitmap[MAX_RESOURCES];
BITMAP Bm[MAX_RESOURCES];
HINSTANCE hInst;
UINT EditMode;
MapHdr MH, NewMH;
BYTE Path[128] = { 0 };


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	HWND hWnd;
	MSG msg;
	WNDCLASS WndClass;
	HACCEL hac;

	int i;

	hInst = hInstance;

	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.lpszClassName = _T("Sokoban");
	RegisterClass(&WndClass);

	for (i = 0; i < MAX_RESOURCES; ++i)
	{
		hBitmap[i] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1 + i));
		GetObject(hBitmap[i], sizeof(BITMAP), &Bm[i]);
	}

	hWnd = CreateWindow(WndClass.lpszClassName, //Ŭ���� �̸�
		_T("Sokoban"), // ������ Ÿ��Ʋ �̸�
		WS_OVERLAPPEDWINDOW, //��Ÿ��
		CW_USEDEFAULT, //x��ǥ
		CW_USEDEFAULT, //y��ǥ
		CW_USEDEFAULT, //��
		CW_USEDEFAULT, //�ʺ�
		NULL, // �θ������� �ڵ�
		NULL, // �޴� �ڵ�
		hInstance, //�������α׷�ID
		NULL); // ������ ������ ����

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hac = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hac, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	BOOL TouchBox = FALSE;
	static HMENU hMenu;
	char ax = 0, ay = 0, Result;
	static int Select = -1, px, py, move_cnt = 0, push_cnt = 0;
	int i, ClickX, ClickY;
	char status[100];

	switch (iMsg)
	{
		case WM_CREATE:
			hMenu = GetMenu(hWnd);
			EnableMenuItem(hMenu, ID_UNDO, MF_GRAYED);
			EnableMenuItem(hMenu, ID_REDO, MF_GRAYED);
			EnableMenuItem(hMenu, ID_GAMERESTART, MF_GRAYED);
			EnableMenuItem(hMenu, ID_SAVEMAP, MF_GRAYED);
			EnableMenuItem(hMenu, ID_MAPEDIT, MF_GRAYED);
			break;
		case WM_PAINT:
			hDC = BeginPaint(hWnd, &ps);
			if (EditMode) 
				CreateTool(hDC, hBitmap, Select, MAX_RESOURCES);
			CreateMap(EditMode ? 70 : 0, hDC, hBitmap, MAX_RESOURCES, &MH, EditMode);

			if (!EditMode && MH.Stage > 0)
			{
				sprintf(status, "�������� : %d  �̵� : %d  �ڽ� �̵� : %d", MH.Stage, move_cnt, push_cnt);
				SetBkMode(hDC, TRANSPARENT);
				TextOut(hDC, 0, 0, status, strlen(status));
			}

			px = MH.PlayerX;
			py = MH.PlayerY;
			EndPaint(hWnd, &ps);
			break;
		case WM_LBUTTONDOWN:
		{
			if (EditMode)
			{
				ClickX = LOWORD(lParam);
				ClickY = HIWORD(lParam);

				if (ClickX > 70)
				{
					if (Select >= 0)
					{
						EditMap(ClickX, ClickY, Select, &MH);
						InvalidateRgn(hWnd, NULL, FALSE);
						EnableMenuItem(hMenu, ID_SAVEMAP, MF_ENABLED);
					}
				}
				else
				{
					if (SelectTool(ClickX, ClickY, MAX_RESOURCES, &Select))
						InvalidateRgn(hWnd, NULL, FALSE);
				}

			}

			break;
		}
		case WM_RBUTTONDOWN:
			Select = -1;
			InvalidateRgn(hWnd, NULL, FALSE);
			break;
		case WM_KEYDOWN:
		{
			if (!EditMode)
			{
				switch (LOWORD(wParam))
				{
					case VK_LEFT:
						ax = -1;
						break;
					case VK_RIGHT:
						ax = 1;
						break;
					case VK_UP:
						ay = -1;
						break;
					case VK_DOWN:
						ay = 1;
						break;
				}

				i = MH.MAP_WIDTH * (py + ay) + px + ax;

				switch (MH.MAP[i])
				{
					case WALL:
					{
						ax = 0;
						ay = 0;
						break;
					}
					default:
					{
						if (MH.MAP[i] & BOX)
						{
							i = MH.MAP_WIDTH * (py + ay * 2) + px + ax * 2;

							if (MH.MAP[i] == WALL || MH.MAP[i] & BOX)
							{
								ax = 0;
								ay = 0;
								break;
							}

							MH.MAP[i] |= BOX;
							TouchBox = TRUE;
							++push_cnt;
							break;
						}
					}
				}

				if (ax || ay)
				{
					MH.MAP[MH.MAP_WIDTH * py + px] &= ~(PLAYER | BOX);

					px += ax;
					py += ay;

					MH.MAP[MH.MAP_WIDTH * py + px] |= PLAYER;
					InvalidateRgn(hWnd, NULL, FALSE);

					WriteMoving(ax, ay, TouchBox);
					EnableMenuItem(hMenu, ID_UNDO, MF_ENABLED);
					EnableMenuItem(hMenu, ID_REDO, MF_GRAYED);

					++move_cnt;
				}

				if (IsGoal(&MH))
				{
					MessageBox(hWnd, "�������� Ŭ����!", "�˸�", MB_ICONINFORMATION);
					DeleteMoving();
					move_cnt = push_cnt = 0;
					EnableMenuItem(hMenu, ID_UNDO, MF_GRAYED);
					EnableMenuItem(hMenu, ID_REDO, MF_GRAYED);

					ReleaseMap(&MH);
					++MH.Stage;

					if (MH.Stage > MH.TotalMapCount)
					{
						MessageBox(hWnd, "��� ���� Ŭ���� �߽��ϴ�.", "�˸�", MB_ICONINFORMATION);
						PostQuitMessage(0);
					}

					if (!ReadMap(Path, MH.Stage, &MH))
					{
						MessageBox(hWnd, "�� �ҷ����� ����", "����", MB_ICONERROR);
						PostQuitMessage(0);
					}

					UpdateWindowSize(hWnd, &MH, FALSE);
					InvalidateRgn(hWnd, NULL, TRUE);
				}
			}

			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case ID_NEWMAP:
					if (Result = DialogBox(hInst, MAKEINTRESOURCE(IDD_MAPSIZE), hWnd, MAPSIZEProc))
					{
						if (Result == 1)
						{
							if (NewMH.TotalMapCount > 1)
							{
								SaveMap(Path, MH.Stage, Replacement, &MH);
								ReleaseMap(&MH);

								SaveMap(Path, NewMH.Stage, (NewMH.Stage >= NewMH.TotalMapCount) ? Addition : Replacement, &NewMH);
								MH = NewMH;
							}
							else
							{
								if (strlen(Path) < 1)
								{
									if (OpenSave(hWnd, Path, sizeof(Path)))
									{
										SaveMap(Path, NewMH.Stage, NewMap, &NewMH);
										MH = NewMH;
									}
									else
										return 0;
								}
							}
						}
						else if(Result == 2)
						{
							if (!ReadMap(Path, NewMH.Stage, &MH))
							{
								MessageBox(hWnd, "�� �ҷ����� ����", "����", MB_ICONERROR);
								return 0;
							}
						}

						EditMode = 1;
						Select = -1;
						InvalidateRgn(hWnd, NULL, TRUE);
						UpdateWindowSize(hWnd, &MH, EditMode);
						EnableMenuItem(hMenu, ID_GAMERESTART, MF_GRAYED);
						EnableMenuItem(hMenu, ID_GAMESTART, MF_ENABLED);
					}

					break;
				case ID_LOADMAP:
				{
					if (OpenLoad(hWnd, Path, sizeof(Path)))
					{
						MH.Stage = 1;

						if (!ReadMap(Path, MH.Stage, &MH))
						{
							MessageBox(hWnd, "�� �ҷ����� ����", "����", MB_ICONERROR);
							return 0;
						}
					}

					EditMode = 1;
					EnableMenuItem(hMenu, ID_GAMERESTART, MF_GRAYED);
					EnableMenuItem(hMenu, ID_GAMESTART, MF_ENABLED);
					InvalidateRgn(hWnd, NULL, TRUE);
					UpdateWindowSize(hWnd, &MH, EditMode);
					break;
				}
				case ID_SAVEMAP:
					if (MH.TotalMapCount > 0)
					{
						if (OpenSave(hWnd, Path, sizeof(Path)))
						{
							SaveMap(Path, MH.Stage, Replacement, &MH);
							EnableMenuItem(hMenu, ID_SAVEMAP, MF_GRAYED);
						}
					}
					else
						MessageBox(hWnd, "������ ���� �����ϴ�.", "����", MB_ICONERROR);

					break;
				case ID_MAPEDIT:
					if (move_cnt > 0)
					{
						if (MessageBox(hWnd, "�÷��� �߿� �����ϱ⸦ �����̽��ϴ�.\n�÷��� �ϴ� ���� �״�� ���� �Ͻðڽ��ϱ�?", "����", MB_ICONQUESTION | MB_YESNO) == IDNO)
						{
							if (!ReadMap(Path, MH.Stage, &MH))
							{
								MessageBox(hWnd, "�� �ҷ����� ����", "����", MB_ICONERROR);
								return 0;
							}
						}
					}

					if (MH.TotalMapCount > 0)
					{
						EditMode = 1;
						InvalidateRgn(hWnd, NULL, TRUE);

						UpdateWindowSize(hWnd, &MH, EditMode);
						EnableMenuItem(hMenu, ID_MAPEDIT, MF_GRAYED);
						EnableMenuItem(hMenu, ID_GAMERESTART, MF_GRAYED);
						EnableMenuItem(hMenu, ID_GAMESTART, MF_ENABLED);

						move_cnt = push_cnt = 0;
						DeleteMoving();
						EnableMenuItem(hMenu, ID_UNDO, MF_GRAYED);
						EnableMenuItem(hMenu, ID_REDO, MF_GRAYED);
					}
					else
						MessageBox(hWnd, "������ ���� �����ϴ�.", "����", MB_ICONERROR);
					break;
				case ID_GAMESTART:
					if (strlen(Path) < 1)
					{
						if (!OpenLoad(hWnd, Path, sizeof(Path)))
							return 0;

						if (!ReadMap(Path, 1, &MH))
						{
							MessageBox(hWnd, "�� �ҷ����� ����", "����", MB_ICONERROR);
							return 0;
						}
					}

					if (EditMode)
						SaveMap(Path, MH.Stage, Replacement, &MH);
	
					DeleteMoving();
					move_cnt = push_cnt = 0;
					EnableMenuItem(hMenu, ID_UNDO, MF_GRAYED);
					EnableMenuItem(hMenu, ID_REDO, MF_GRAYED);
					EnableMenuItem(hMenu, ID_GAMESTART, MF_GRAYED);
					EnableMenuItem(hMenu, ID_GAMERESTART, MF_ENABLED);
					EnableMenuItem(hMenu, ID_MAPEDIT, MF_ENABLED);
					EditMode = 0;
					InvalidateRgn(hWnd, NULL, TRUE);
					UpdateWindowSize(hWnd, &MH, FALSE);
					break;
				case ID_GAMERESTART:
					if (MH.Stage > 0)
					{
						ReadMap(Path, MH.Stage, &MH);
						move_cnt = push_cnt = 0;
						InvalidateRgn(hWnd, NULL, TRUE);
						UpdateWindowSize(hWnd, &MH, FALSE);
						DeleteMoving();
						EnableMenuItem(hMenu, ID_UNDO, MF_GRAYED);
						EnableMenuItem(hMenu, ID_REDO, MF_GRAYED);
					}
					else
						MessageBox(hWnd, "�ùٸ� ���� �ƴմϴ�.", "����", MB_ICONERROR);

					break;
				case ID_UNDO:
				{
					if (Record_Index > 0)
					{
						--Record_Index;
						EnableMenuItem(hMenu, ID_REDO, MF_ENABLED);

						ReadMoving(&ax, &ay, &TouchBox);
						MH.MAP[MH.MAP_WIDTH * py + px] &= ~PLAYER;

						if (TouchBox)
						{
							MH.MAP[MH.MAP_WIDTH * py + px] |= BOX;
							MH.MAP[MH.MAP_WIDTH * (py + ay) + px + ax] &= ~BOX;
							--push_cnt;
						}

						px -= ax;
						py -= ay;

						MH.MAP[MH.MAP_WIDTH * py + px] |= PLAYER;
						--move_cnt;
						InvalidateRgn(hWnd, NULL, FALSE);

						if(Record_Index == 0)
							EnableMenuItem(hMenu, ID_UNDO, MF_GRAYED);
					}
	
					break;
				}
				case ID_REDO:
				{
					if (Record_Index < Record_Conut)
					{
						EnableMenuItem(hMenu, ID_UNDO, MF_ENABLED);

						ReadMoving(&ax, &ay, &TouchBox);
						MH.MAP[MH.MAP_WIDTH * py + px] &= ~PLAYER;

						if (TouchBox)
						{
							MH.MAP[MH.MAP_WIDTH * (py + ay) + px + ax] &= ~BOX;
							MH.MAP[MH.MAP_WIDTH * (py + ay * 2) + px + ax * 2] |= BOX;
							++push_cnt;
						}

						px += ax;
						py += ay;

						MH.MAP[MH.MAP_WIDTH * py + px] |= PLAYER;

						++Record_Index;
						++move_cnt;
						InvalidateRgn(hWnd, NULL, FALSE);

						if( Record_Index == Record_Conut )
							EnableMenuItem(hMenu, ID_REDO, MF_GRAYED);
					}

					break;
				}
				case ID_HELP:
					MessageBox(hWnd, "���� ����Ű\n\n���� ���� : Z\n���� ����� : X\n���� ���� ��� : U\n���� �ٽ� ���� : R\n���� ���� : H, F1\n\n������ �����ϸ� ������ ������ ���� �ڵ� ����˴ϴ�.", "����", MB_ICONINFORMATION);
					break;
				case ID_ABOUT:
					MessageBox(hWnd, "������ : spspwl@naver.com", "", MB_ICONINFORMATION);
					break;
			}
			break;
		}
		case WM_DESTROY:
			for (i = 0; i < MAX_RESOURCES; ++i)
				DeleteObject(hBitmap[i]);

			ReleaseMap(&MH);
			PostQuitMessage(0);
			break;
	}

	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

INT_PTR CALLBACK MAPSIZEProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT Result;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			SendDlgItemMessage(hDlg, IDC_EDIT2, WM_SETTEXT, 0, (LPARAM)"5");
			SendDlgItemMessage(hDlg, IDC_EDIT3, WM_SETTEXT, 0, (LPARAM)"5");
			SetDlgItemInt(hDlg, IDC_EDIT4, MH.TotalMapCount + 1, 0);
			break;
		}
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				NewMH.Stage = GetDlgItemInt(hDlg, IDC_EDIT4, NULL, 0);

				if (NewMH.Stage <= 0 || NewMH.Stage > MH.TotalMapCount + 1)
				{
					MessageBox(hDlg, "�ùٸ� �������� ���� �Է��ϼ���.", "����", MB_ICONEXCLAMATION);
					return 0;
				}

				switch (LOWORD(wParam))
				{
					case ID_GOTOST:
						Result = 2;
						break;
					case IDCANCEL:
						Result = 0;
						break;
					default:
					{
						if (LOWORD(wParam) == ID_COVERST || LOWORD(wParam) == IDOK)
						{
							Result = 1;
							NewMH.MAP_WIDTH = GetDlgItemInt(hDlg, IDC_EDIT2, NULL, 0);
							NewMH.MAP_HEIGHT = GetDlgItemInt(hDlg, IDC_EDIT3, NULL, 0);
							NewMH.TotalMapCount = MH.TotalMapCount;
							NewMH.MAP = (BYTE*)malloc(NewMH.MAP_WIDTH * NewMH.MAP_HEIGHT);
							memset(NewMH.MAP, 0, NewMH.MAP_WIDTH * NewMH.MAP_HEIGHT);

							if (LOWORD(wParam) == IDOK)
								NewMH.TotalMapCount++;
						}
					}
				}

				EndDialog(hDlg, Result);
			}

			if (HIWORD(wParam) == EN_CHANGE)
			{
				if (LOWORD(wParam) == IDC_EDIT4)
				{
					if (GetDlgItemInt(hDlg, IDC_EDIT4, NULL, 0) > MH.TotalMapCount)
					{
						ShowWindow(GetDlgItem(hDlg, IDOK), TRUE);
						ShowWindow(GetDlgItem(hDlg, ID_COVERST), FALSE);
						ShowWindow(GetDlgItem(hDlg, ID_GOTOST), FALSE);
					}
					else
					{
						ShowWindow(GetDlgItem(hDlg, IDOK), FALSE);
						ShowWindow(GetDlgItem(hDlg, ID_COVERST), TRUE);
						ShowWindow(GetDlgItem(hDlg, ID_GOTOST), TRUE);
					}
				}
			}

			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hDlg, 0);
			break;
		}
	}
	return (INT_PTR)FALSE;
}