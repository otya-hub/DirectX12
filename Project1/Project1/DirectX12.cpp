#include "DirectX12.h"
#include <d3d12.h>   // Directx12 本体を利用するためのヘッダ
#include <dxgi1_4.h> // Windows グラフィックス基盤機能を利用するためのヘッダ


void EnableDebugLayer() // 1.デバッグレイヤーの有効化
{
#if defined(_DEBUG)
	// デバッグインターフェースを取得
	ID3D12Debug* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		// デバッグレイヤーを有効化
		debugController->EnableDebugLayer();

		// より詳細な検証を有効化(任意。動作は重くなる)
		ID3D12Debug1* debugController1 = nullptr;
		if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
		{
			debugController1->SetEnableGPUBasedValidation(TRUE);
			debugController1->Release(); // 設定後な不要なので解放
		}

		debugController->Release(); // 設定後は不要なので解放
	}
#endif
}
IDXGIFactory4* CreateFactory()
{
	IDXGIFactory4* factory = nullptr;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	// デバッグビルドではデバッグフラグを設定
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		// エラーハンドリング：ファクトリー作成失敗
		OutputDebugStringA("Failed to create DXGI Factory\n");
		return nullptr;
	}
	return factory;
	
}

IDXGIAdapter1* GetHardwareAdapter(IDXGIFactory4* factory)
{
	IDXGIAdapter1* adapter = nullptr;

	// ハードウェアアダプターを順番に検索
	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter))
		{
			break; // アダプターが見つからない場合は終了
		}

		// アダプター情報を取得
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// ソフトウェアアダプターをスキップ
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter->Release();
			continue;
		}

		// DirectX12 対応テスト(最後の引数を nullptr にすると作成せず確認のみ行う)
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			return adapter; // 適切なアダプターを発見
		}

		adapter->Release();
		
	}
	return nullptr;
}

ID3D12Device* CreateD3D12Device(IDXGIFactory4* factory, IDXGIAdapter1* adapter)
{
	ID3D12Device* device = nullptr;
	HRESULT hr = E_FAIL;

	// ハードウェアアダプターでデバイス作成を試行
	if (adapter != nullptr)
	{
		hr = D3D12CreateDevice(
			adapter,                // 使用するアダプター
			D3D_FEATURE_LEVEL_11_0, // 最小機能レベル
			IID_PPV_ARGS(&device)   // 作成されるデバイス
		);
	}

	if (FAILED(hr))
	{
		// フォールバック：WARP(ソフトウェアアダプター)を試行
		IDXGIAdapter* warpAdapter = nullptr;
		hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
		if (SUCCEEDED(hr))
		{
			hr = D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
			warpAdapter->Release();      // デバイス作成後は不要なので解放
		}

		if (FAILED(hr))
		{
			OutputDebugStringA("FAiled to create D3D12 Device\n");
			return nullptr;
		}

		OutputDebugStringA("Using WARP adapter\n");
	}
	return device;
	
}

ID3D12CommandQueue* CreateCommandQueue(ID3D12Device* device)
{
	// コマンドキューの設定
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;          // 直接実行型
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // 通常優先度
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;          // 特別フラグなし
	queueDesc.NodeMask = 0;                                   // 単一GPU使用

	ID3D12CommandQueue* commandQueue = nullptr;
	HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));

	if (FAILED(hr))
	{
		OutputDebugStringA("Failed to create Command Queue\n");
		return nullptr;
	}

	// デバッグ用の名前設定(任意だが推奨)
	commandQueue->SetName(L"Main Command Queue");   // SetName は常にワイド文字列を受け取るため L を付ける

	return commandQueue;
	
}

IDXGISwapChain3* CreateSwapChain(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, HWND hwnd)
{
	// スワップチェーンの詳細設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount      = 2;                               // ダブルバッファリング 
	swapChainDesc.Width            = 1280;                            // 画面幅
	swapChainDesc.Height           = 720;                             // 画面高
	swapChainDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;      // ピクセルフォーマット
	swapChainDesc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT; // レンダーターゲット用
	swapChainDesc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // 高速切替
	swapChainDesc.SampleDesc.Count = 1;                               // マルチサンプリングなし

	IDXGISwapChain1* swapChain1 = nullptr;
	HRESULT hr = factory->CreateSwapChainForHwnd(
		commandQueue,   // コマンドキュー
		hwnd,           // ターゲットウィンドウ
		&swapChainDesc, // 設定
		nullptr,        // フルスクリーン設定
		nullptr,        // 出力制限
		&swapChain1     // 作成されるスワップチェーン
	);

	if (FAILED(hr))
	{
		OutputDebugStringA("Faild to create Swap Chain\n");
		return nullptr;
	}

	// より高機能なインターフェースを取得
	IDXGISwapChain3* swapChain = nullptr;
	hr = swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain));
	swapChain1->Release();
	
	if (FAILED(hr))
	{
		OutputDebugStringA("Failed to cast to SwapChain3\n");
		return nullptr;
	}

	return swapChain;
	
}