#include <Windows.h>
#include <string>
#include <Windowsx.h> //Button_SetCheck macro`
#include <CommCtrl.h> //CommCtrl includes sliders
#include <iostream>
#include "resource.h"
#include "SCXLoader.h"
#include "Settings.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void initialize(HINSTANCE hInstance);
void enable_settings(bool);
void refresh();
void update_state();
void get_settings();
void update_sleep_bar();
SCXParameters GetParameters();

namespace
{
  LPCSTR ResolutionOptions[4] = { "[4:3] 640x480 (Original)", "[4:3] 1024x768", "[16:9] 1280x720", "[16:10] 1280x800" };
  unsigned int SpeedValues[9] = { 1, 4, 8, 12, 16, 20, 24, 32, 64 };

  HWND PatchButton;
  HWND InstallButton;
  HWND StartButton;
  //HWND HelpButton;

  HWND settingsHwnd;

  //HWND verifyCheckbox;

  HWND fsRadioButton;
  HWND wsRadioButton;

  HWND speedTextbox;
  HWND resolutionCombobox;

  SettingsInfo info;

  int resolutionValue = 0;
  bool fullscreenValue = true;

  HWND SleepBar;
  HWND resolutionTextbox;

  WNDCLASSEX SettingsClass;

  //bool verifyInstallationValue = false;

  bool CanEnd = false;
  bool end_process = false;

  int speedMS = 16;
  HBITMAP hBitmap = NULL;
}

void update_sleep_bar()
{
  if (info.SleepTime && info.SleepTime <= (sizeof(SpeedValues) / 4))
    speedMS = SpeedValues[info.SleepTime - 1];

  std::string speedText("Game Sleep: ");
  speedText.append(std::to_string(speedMS)).append("ms");
  SetWindowText(speedTextbox, speedText.c_str());
  UpdateWindow(speedTextbox);
}

void get_settings()
{
  info = Settings::GetSettingsInfo();
  SendMessage(SleepBar, TBM_SETPOS, WPARAM(FALSE), LPARAM(info.SleepTime));
  SendMessage(resolutionCombobox, CB_SETCURSEL, (WPARAM)(info.Resolution), (LPARAM)0);
  if (info.ScreenMode)
  {
    Button_SetCheck(fsRadioButton, BST_CHECKED);
    Button_SetCheck(wsRadioButton, BST_UNCHECKED);
  }
  else
  {
    Button_SetCheck(fsRadioButton, BST_UNCHECKED);
    Button_SetCheck(wsRadioButton, BST_CHECKED);
  }
  update_sleep_bar();
  refresh();
}

SCXParameters GetParameters()
{
  SCXParameters parameters;
  parameters.fullscreen = SendMessage(fsRadioButton, BM_GETCHECK, 0, 0) == BST_CHECKED;
  info.ScreenMode = parameters.fullscreen;

  switch (resolutionValue)
  {
  case 0: //640x480
    parameters.resolution_mode = 1;
    break;
  case 1: //1024x768
    parameters.resolution_mode = 3;
    break;
  case 2: //1280x720
    parameters.resolution_mode = 2;
    break;
  case 3: //1280x800
    parameters.resolution_mode = 0;
    break;
  default:
    printf("Error: Invalid resolution combobox selection \n");
    parameters.resolution_mode = 1;
    break;
  }
  parameters.sleep_time = speedMS;
  return parameters;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
  hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP1));
  initialize(hInstance);
  PatchInfo info = Settings::GetPatchInfo();

  SCXLoader::LoadSettings();
  update_state();
  get_settings();

  MSG msg;
  while (!end_process && GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
}

void destroy()
{
  if (settingsHwnd != NULL)
    DestroyWindow(settingsHwnd);
}

void update_state()
{
  if (SCXLoader::GetValidInstallation())
  { //Patched and installed
    Button_Enable(StartButton, TRUE);
    enable_settings(true);
    Button_Enable(InstallButton, FALSE);
  }
  else if (SCXLoader::GetPatchedSCXVersion() > 0)
  { //Patched but not installed
    Button_Enable(StartButton, FALSE);
    Button_Enable(InstallButton, TRUE);
    enable_settings(false);
  }
  else
  { //Not patched or installed
    Button_Enable(StartButton, FALSE);
    Button_Enable(InstallButton, FALSE);
    enable_settings(false);
  }
}

void refresh()
{
  UpdateWindow(wsRadioButton);
  UpdateWindow(PatchButton);
  UpdateWindow(speedTextbox);
  UpdateWindow(resolutionCombobox);
  UpdateWindow(StartButton);
  UpdateWindow(fsRadioButton);
  UpdateWindow(InstallButton);
  UpdateWindow(SleepBar);
}

void enable_settings(bool enable)
{
  Button_Enable(fsRadioButton, enable);
  Button_Enable(wsRadioButton, enable);
  ComboBox_Enable(resolutionCombobox, enable);
  Edit_Enable(SleepBar, enable);
  Edit_Enable(speedTextbox, enable);
  refresh();
}

void initialize(HINSTANCE hInstance)
{
  SettingsClass.cbClsExtra = NULL;
  SettingsClass.cbWndExtra = NULL;
  SettingsClass.lpszMenuName = NULL;
  SettingsClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  SettingsClass.hbrBackground = CreateSolidBrush(COLORREF(0xf0f0f0));
  SettingsClass.cbSize = sizeof(WNDCLASSEX);
  SettingsClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
  SettingsClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
  SettingsClass.hInstance = hInstance;
  SettingsClass.lpfnWndProc = WndProc;
  SettingsClass.lpszClassName = "SASETTINGS";
  SettingsClass.style = CS_HREDRAW | CS_VREDRAW;
  RegisterClassEx(&SettingsClass);

  unsigned int window_width = 400;
  unsigned int window_height = 300;
  unsigned int window_x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
  unsigned int window_y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;

  settingsHwnd = CreateWindowEx(
    WS_EX_STATICEDGE,
    SettingsClass.lpszClassName,
    std::string("SimCopterX").c_str(),
    WS_VISIBLE | WS_CLIPCHILDREN | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
    window_x, window_y, window_width, window_height, NULL, NULL, NULL, NULL);

  PatchButton = CreateWindow(
    "Button", "Patch Game", WS_VISIBLE | WS_CHILDWINDOW | BS_PUSHBUTTON,
    10, 65, 150, 25, settingsHwnd, NULL,
    NULL, NULL);

  InstallButton = CreateWindow(
    "Button", "Install Game", WS_VISIBLE | WS_CHILDWINDOW | BS_PUSHBUTTON,
    190, 65, 150, 25, settingsHwnd, NULL,
    NULL, NULL);

  resolutionCombobox = CreateWindow(
    "COMBOBOX", "", WS_VISIBLE | WS_CHILDWINDOW | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_BORDER,
    10, 132, 195, 100, settingsHwnd, NULL, NULL, NULL);

  SleepBar = CreateWindow(
    TRACKBAR_CLASS, "TEST", WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_AUTOTICKS,
    220, 144, 150, 20, settingsHwnd, NULL, NULL, NULL);

  speedTextbox = CreateWindow("EDIT", "Game Sleep: 16ms",
    WS_CHILD | WS_VISIBLE | ES_LEFT | ES_READONLY | ES_MULTILINE,
    230, 174, 150, 20, settingsHwnd, NULL, NULL, NULL);

  fsRadioButton = CreateWindow(
    "Button", "Fullscreen", WS_VISIBLE | WS_CHILDWINDOW | BS_AUTORADIOBUTTON,
    10, 170, 100, 25, settingsHwnd, NULL,
    NULL, NULL);
  Button_SetCheck(fsRadioButton, BST_CHECKED);

  wsRadioButton = CreateWindow(
    "Button", "Windowed", WS_VISIBLE | WS_CHILDWINDOW | BS_AUTORADIOBUTTON,
    115, 170, 100, 25, settingsHwnd, NULL,
    NULL, NULL);

  StartButton = CreateWindow(
    "Button", "Dispatch!", WS_VISIBLE | WS_CHILDWINDOW | BS_PUSHBUTTON,
    110, 225, 150, 25, settingsHwnd, NULL,
    NULL, NULL);

  SendMessage(SleepBar, TBM_SETRANGEMIN, WPARAM(FALSE), LPARAM(1));
  SendMessage(SleepBar, TBM_SETRANGEMAX, WPARAM(FALSE), LPARAM(9));
  SendMessage(SleepBar, TBM_SETPOS, WPARAM(FALSE), LPARAM(5));
  SendMessage(SleepBar, TBM_SETTICFREQ, WPARAM(1), LPARAM(0));


  for (int i = 0; i < sizeof(ResolutionOptions) / sizeof(LPCSTR); i++)
    SendMessage(resolutionCombobox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)ResolutionOptions[i]);

  SendMessage(resolutionCombobox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
  refresh();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch (Msg)
  {
  case WM_PAINT:
    PAINTSTRUCT     ps;
    HDC             hdc;
    BITMAP          bitmap;
    HDC             hdcMem;
    HGDIOBJ         oldBitmap;

    hdc = BeginPaint(hWnd, &ps);

    hdcMem = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(hdcMem, hBitmap);

    GetObject(hBitmap, sizeof(bitmap), &bitmap);
    BitBlt(hdc, 90, 10, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);

    EndPaint(hWnd, &ps);
    return 0;
  case WM_ACTIVATE:
    refresh();
    return 0;

  case WM_HSCROLL:
    {
      info.SleepTime = SendMessage((HWND)lParam, (UINT)TBM_GETPOS, (WPARAM)0, (LPARAM)0);
      speedMS = 16;
      update_sleep_bar();
    }
    return 0;
  case WM_COMMAND:

    if ((HWND)lParam == resolutionCombobox)
    {
      if (HIWORD(wParam) == CBN_SELCHANGE)
      {
        info.Resolution = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
        char ListItem[256];
        SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)info.Resolution, (LPARAM)ListItem);
        for (int i = 0; i < sizeof(ResolutionOptions) / sizeof(LPCSTR); i++)
        {
          if (_stricmp(std::string(ListItem).c_str(), ResolutionOptions[i]) == 0)
          {
            resolutionValue = i;
            break;
          }
        }
      }
    }
    else if ((HWND)lParam == resolutionTextbox)
    {
      ::SetFocus(NULL);
    }
    else if ((HWND)lParam == speedTextbox)
    {
      ::SetFocus(NULL);
    }
    else
    {
      switch (HIWORD(wParam))
      {
      case BN_CLICKED:
        if ((HWND)lParam == PatchButton)
        {
          Button_Enable(PatchButton, FALSE);
          ::SetFocus(NULL);
          char szFile[256];
          OPENFILENAME ofn;
          ZeroMemory(&ofn, sizeof(ofn));
          ofn.lStructSize = sizeof(ofn);
          ofn.hwndOwner = NULL;
          ofn.lpstrFile = szFile;
          ofn.lpstrFile[0] = '\0';
          ofn.nMaxFile = sizeof(szFile);
          ofn.lpstrFilter = "SimCopter.exe\0SimCopter.exe;\0";
          ofn.nFilterIndex = 1;
          ofn.lpstrFileTitle = NULL;
          ofn.nMaxFileTitle = 0;
          ofn.lpstrInitialDir = NULL;
          ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
          GetOpenFileName(&ofn);
          SCXParameters params = GetParameters();
          bool result = SCXLoader::CreatePatchedGame(ofn.lpstrFile, params);
          update_state();
          //Button_Enable(StartButton, params.verify_install ? SCXLoader::GetValidInstallation() : false);
          //Button_Enable(PatchButton, TRUE);
        }
        else if ((HWND)lParam == InstallButton)
        {
          SCXLoader::InstallGame();
          update_state();
        }
        else if ((HWND)lParam == StartButton)
        {
          if (SCXLoader::StartSCX(GetParameters()))
          {
            Settings::SetSettingsInfo(info);
            end_process = true;
          }
          ::SetFocus(NULL);
          UpdateWindow(StartButton);
        }
        else if ((HWND)lParam == InstallButton)
        {
          ::SetFocus(NULL);
          UpdateWindow(InstallButton);
        }
        /*
        else if ((HWND)lParam == HelpButton)
        {
          MessageBox(hWnd, std::string(
            "Version: " + std::to_string(SCXLoader::SCX_VERSION) + "\n"
            "www.alekasm.com\n"
           ).c_str(), "SimCopterX About", MB_ICONINFORMATION);
          ::SetFocus(NULL);
          UpdateWindow(HelpButton);
        }*/
        /*
        else if ((HWND)lParam == verifyCheckbox)
        {
          ::SetFocus(NULL);
          SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
          //verifyInstallationValue = chkState == BST_CHECKED;
        }
        */

      }
    }
    return 0;
  case WM_DESTROY:
    end_process = true;
    return 0;
  default:
    return DefWindowProc(hWnd, Msg, wParam, lParam);
  }
}