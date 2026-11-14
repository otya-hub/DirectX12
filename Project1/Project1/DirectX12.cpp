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

/*// ディスクリプターヒープの作成例
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2; // スワップチェーンのバッファ数
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // RTV用はシェーダから参照しない

	ID3D12DescriptorHeap* rtvHeap;
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

	// RTVハンドルの取得例
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 2番目のRTVハンドルの計算
	rtvHandle.ptr += rtvDescriptorSize;

	// レンダーターゲット作成例
	ID3D12Resource* renderTaegets[2]; // スワップチェーンでバッファが二つあるのでダブルバッファで

	for (int i = 0; i < 2; i++)
	{
		// スワップチェーンからバッファを取得
		swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTaegets[i]));

		// RTVの作成
		device->CreateRenderTargetView(renderTaegets[i], nullptr, rtvHandle);
	}

	// コマンドアロケータ作成例
	ID3D12CommandAllocator* commandAllocators[2];

	for (int i = 0; i < 2; i++)
	{
		device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&commandAllocators[i])
		);
	}

	// コマンドリスト作成例
	ID3D12GraphicsCommandList* commandList;
	device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocators[0],  // 作成時は先頭のアロケータを指定
		nullptr,
		IID_PPV_ARGS(&commandList)
	);

	// 初期状態でクローズしておく
	commandList->Close();

	// 現在のバックバッファインデックスを取得
	UINT currentBackBuffer = SwapChain->GetCurrentBackBufferIndex();

	// ※ アロケータのリセット前に、子のアロケータを使ったコマンドの
	//    GPU 実行完了をフェンスで待機しておくこと(後述)
	// コマンドアロケータとコマンドリストをリセット
	commandAllocators[currentBackBuffer]->Reset();
	commandList->Reset(commandAllocators[currentBackBuffer], nullptr);

	// レンダーターゲットを「絵を描く状態」に変更する（リソースバリア）
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = renderTaegets[currentBackBuffer];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);

	// レンダーターゲットの取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += currentBackBuffer * rtvDescriptorSize;

	// 画面クリア（青色）
	float clearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// レンダーターゲットを「絵を出す状態」に変更する（リソースバリア）
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->Close();

	// ここまでやって「画面をクリアする」のコマンドリストが完成する

	// 「画面をクリアする」コマンドリストを実行 → GPU が記録内容の実行を開始する
	commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);

	// フェンスの生成例
	ID3D12Fence* fence;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent;

	// フェンスの生成
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	// フェンスイベントの生成
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// GPU で行われているコマンドリストの処理が全て終わったら 1 になるように設定する
	fenceValue = 1;
	commandQueue->Signal(fence, fenceValue);

	// まだ 1 になっていない（GPU処理が終わっていない）
	if (fence->GetCompletedValue() < fenceValue)
	{
		// フェンス値が 1 に達したら fenceEvent をシグナル状態にする
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		// fenceEvent がシグナル状態になるまで CPU を止める
		WaitForSingleObject(fenceEvent, INFINITE);
	}*/