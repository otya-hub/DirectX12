#include "main.h"
#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:        // キーが押されたとき
		// キー入力の処理をここに書く
		return 0;

	case WM_DESTROY:        // xボタンが押されたとき
		PostQuitMessage(0); // 「アプリを終了します」とWindowsに通知
		return 0;

	case WM_PAINT:          // ウィンドウの再描画が必要な時
		// 画面を更新する処理をここに書く
		return 0;
	}
	// 自分で処理しないメッセージはWindowsに任せる
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(
	HINSTANCE hInstance,      // アプリケーションの識別番号 
	HINSTANCE hPrevInstance,  // 基本使わなくていい
	LPSTR lpCmdLine,          // コマンドライン引数(起動時のオプション)
	int nCmdShow              // ウィンドウの表示方法(最大化、最小化など)
)
{
	// ここにメインの処理を書く
	// 1. ウィンドウクラス登録
	WNDCLASS wc{};

	wc.hInstance = hInstance;                      // アプリケーションインスタンス
	wc.lpszClassName = "GameWindow";               // ウィンドウクラス名
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);      // マウスカーソル
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // 背景色
	wc.lpfnWndProc = WindowProc;                   // ウィンドウプロシージャを指定

	RegisterClass(&wc);                            // Windowに登録

	// 2. ウィンドウ作成
	HWND hwnd = CreateWindow(
		"GameWindow",                 // ウィンドウクラス名
		"My Game",                    // ウィンドウタイトル
		WS_OVERLAPPEDWINDOW,          // ウィンドウスタイル
		CW_USEDEFAULT, CW_USEDEFAULT, // 位置(自動)
		800, 600,                     // サイズ(幅、高さ)
		NULL, NULL,                   // 親ウィンドウ、メニュー
		hInstance,                    // インスタンス
		NULL                          // 追加データ
	);

	ShowWindow(hwnd, nCmdShow);       // ウィンドウを表示

	// 3. メッセージループ
	MSG msg{}; 
	while (GetMessage(&msg, NULL, 0, 0)) // メッセージが来るまで待機
	{
		TranslateMessage(&msg);          // キーボードメッセージを使いやすい形に変換
		DispatchMessage(&msg);           // 適切なウィンドウプロシージャに送信
	}
	
	return 0;
}

