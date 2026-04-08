/*
================================================================================
 [가이드: 게임 엔진의 뼈대 만들기]
================================================================================
 1. Component (기능): 캐릭터가 할 수 있는 '일' (이동, 시간 재기 등)
 2. GameObject (객체): 게임에 존재하는 '물체' (플레이어, 타이머 등)
 3. GameWorld (세계): 모든 물체를 담고 있는 '바구니'

 * 구조: Component -> GameObject -> GameWorld 순으로 확장됨.
         (루프 한 번 돌 때 [입력 -> 업데이트 -> 렌더링] 순서로 모든 객체를 훑음.)
 [작동 원리]
 - Start(): 물체가 태어날 때 딱 한 번 실행되는 초기화 코드
 - Input(): 키보드/마우스 상태를 확인.
 - Update(): 수치(좌표 등)를 계산.
 - Render(): 화면에 결과를 출력.

================================================================================
*/

#include <iostream>
#include <chrono>
#include <thread>
#include <windows.h>
#include <vector>
#include <string>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// --- [전역 객체 관리] ---
// DirectX 객체들은 GPU 메모리를 직접 사용함. 
// 사용 후 'Release()'를 호출하지 않으면 프로그램 종료 후에도 메모리가 점유될 수 있음(메모리 누수).
ID3D11Device* g_pd3dDevice = nullptr;          // 리소스 생성자 (공장)
ID3D11DeviceContext* g_pImmediateContext = nullptr;   // 그리기 명령 수행 (일꾼)
IDXGISwapChain* g_pSwapChain = nullptr;          // 화면 전환 (더블 버퍼링)
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;   // 그림을 그릴 도화지(View)

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

// HLSL (High-Level Shading Language) 소스
const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f); // 3D 좌표를 4D로 확장
    output.col = input.col;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return input.col; // 정점에서 계산된 색상을 픽셀에 그대로 적용
}
)";



// [1단계: 컴포넌트 기저 클래스]
// 모든 기능(이동, 렌더링 등)은 이 클래스를 상속받아야 함.
class Component
{
public:
    class GameObject* pOwner = nullptr; // 이 기능이 누구의 것인지 저장
    bool isStarted = 0;           // Start()가 실행되었는지 체크

    virtual void Start() = 0;              // 초기화
    virtual void Input() {}                // 입력 (선택사항)
    virtual void Update(float dt) = 0;     // 로직 (필수)
    virtual void Render() {}               // 그리기 (선택사항)

    virtual ~Component() {}
};

// [2단계: 게임 오브젝트 클래스]
// 컴포넌트들을 담는 바구니 역할을 함.
class GameObject {
public:
    std::string name;
    std::vector<Component*> components;

    float x = 0.0f;
    float y = 0.0f;

    GameObject(std::string n)
    {
        name = n;
    }

    // 객체가 죽을 때 담고 있던 컴포넌트들도 메모리에서 해제함
    ~GameObject() {
        for (int i = 0; i < (int)components.size(); i++)
        {
            delete components[i];
        }
    }

    // 새로운 기능을 추가하는 함수
    void AddComponent(Component* pComp)
    {
        pComp->pOwner = this;
        pComp->isStarted = false;
        components.push_back(pComp);
    }
};

// --- [3단계: 실제 구현할 기능 컴포넌트들] ---

// 기능 1: 플레이어 조종 및 이동
class PlayerControl : public Component {
public:
    float x, y, speed;
    bool moveUp, moveDown, moveLeft, moveRight;
    int playerType = 0;

    PlayerControl(int type)
    {
        playerType = type;
    }
    void Start() override
    {
        x = 50.0f; y = 50.0f; speed = 1.0f;
        moveUp = moveDown = moveLeft = moveRight = false;
        printf("[%s] PlayerControl 기능 시작!\n", pOwner->name.c_str());
    }

    // [입력 단계] 키 상태만 체크함
    void Input() override
    {
        if (playerType == 0)
        {
            moveUp = (GetAsyncKeyState('W') & 0x8000);
            moveDown = (GetAsyncKeyState('S') & 0x8000);
            moveLeft = (GetAsyncKeyState('A') & 0x8000);
            moveRight = (GetAsyncKeyState('D') & 0x8000);
        }
        if (playerType == 1)
        {
            moveUp = (GetAsyncKeyState(VK_UP) & 0x8000);
            moveDown = (GetAsyncKeyState(VK_DOWN) & 0x8000);
            moveLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000);
            moveRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000);
        }
    }

    // [업데이트 단계] 체크된 키 상태로 좌표만 계산함
    void Update(float dt) override
    {
        if (moveUp)    y -= speed * dt;
        if (moveDown)  y += speed * dt;
        if (moveLeft)  x -= speed * dt;
        if (moveRight) x += speed * dt;
    }

    // [렌더링 단계] 계산된 좌표를 화면에 그림
    void Render() override
    {
        // 실제 엔진이라면 여기서 DirectX Draw를 부름
        

    }
};

// 기능 2: 시스템 정보 출력 (위치 정보 없음)
/*class InfoDisplay : public Component
{
public:
    float totalTime = 0.0f;

    void Start() override
    {
        totalTime = 0.0f;
        printf("[%s] InfoDisplay 기능 시작!\n", pOwner->name.c_str());
    }

    void Update(float dt) override
    {
        totalTime += dt;
    }

    void Render() override {
        // 화면 최상단에 정보 출력
        MoveCursor(0, 0);
        printf("System Time: %.2f sec\n", totalTime);
        printf("Control: W, A, S, D | Exit: ESC\n");
    }
};*/

// 기능 3: 삼각형 그리기 컴포넌트
class TriangleRenderer :public Component {
    ID3D11Buffer* pVBuffer = nullptr;
    ID3D11InputLayout* pInputLayout = nullptr;
    ID3D11VertexShader* vShader = nullptr;
    ID3D11PixelShader* pShader = nullptr;

    Vertex vertices[3] = {
        {  0.0f,  0.5f, 0.5f,  1.0f, 0.0f, 0.0f, 1.0f },
        {  0.5f, -0.5f, 0.5f,  0.0f, 1.0f, 0.0f, 1.0f },
        { -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, 1.0f }
    };

public:
    void Start() override {
        // 3. 셰이더 컴파일 및 생성
        ID3DBlob* vsBlob, * psBlob;
        D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
        D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

        g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
        g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);

        // 정점의 데이터 형식을 정의 (IA 단계에 알려줌)
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);
        vsBlob->Release(); psBlob->Release(); // 컴파일용 임시 메모리 해제

        // 4. 정점 버퍼 생성 (삼각형 데이터)

        D3D11_BUFFER_DESC bd = { sizeof(vertices), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, 0, 0, 0 };
        D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
        g_pd3dDevice->CreateBuffer(&bd, &initData, &pVBuffer);
    }

    void Render() override {
        // 1. 현재 GameObject의 위치(x, y)를 반영하여 정점 업데이트
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        if (SUCCEEDED(g_pImmediateContext->Map(pVBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
            Vertex* pVertices = (Vertex*)mappedResource.pData;
            for (int i = 0; i < 3; i++) {
                pVertices[i] = vertices[i];
                pVertices[i].x += pOwner->x; // 부모의 x 좌표 반영
                pVertices[i].y += pOwner->y; // 부모의 y 좌표 반영
            }
            g_pImmediateContext->Unmap(pVBuffer, 0);
        }

        // 렌더링 파이프라인 상태 설정
        UINT stride = sizeof(Vertex), offset = 0;
        g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
        g_pImmediateContext->IASetInputLayout(pInputLayout);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pImmediateContext->VSSetShader(vShader, nullptr, 0);
        g_pImmediateContext->PSSetShader(pShader, nullptr, 0);

        g_pImmediateContext->Draw(3, 0);

    }
    ~TriangleRenderer() {
        if (pVBuffer) pVBuffer->Release();
        if (pInputLayout) pInputLayout->Release();
        if (vShader) vShader->Release();
        if (pShader) pShader->Release();
    }
};

class GameLoop
{
public:
    bool isRunning;
    std::vector<GameObject*> gameWorld;
    std::chrono::high_resolution_clock::time_point prevTime;
    float deltaTime;   //delta time;

    //초기화
    void Initialize(HINSTANCE hInstance, int nCmdShow)
    {
        //초기화시 동작준비됨
        isRunning = true;
        gameWorld.clear();
        // 시간 측정 준비
        prevTime = std::chrono::high_resolution_clock::now();
        deltaTime = 0.0f;

        // 1. 윈도우 등록 및 생성

        WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.lpszClassName = L"DX11GameLoopClass";
        RegisterClassExW(&wcex);

        HWND hWnd = CreateWindowW(L"DX11GameLoopClass", L"과제: 컴포넌트 기반 게임 오브젝트 시스템 구현",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
        if (!hWnd) return;
        ShowWindow(hWnd, nCmdShow);

        //DX11 디바이스 및 스왑 체인 초기화
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = 800; sd.BufferDesc.Height = 600;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;

        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);

        // 렌더 타겟 설정 (도화지 준비)
        ID3D11Texture2D* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
        pBackBuffer->Release(); // 뷰를 생성했으므로 원본 텍스트는 바로 해제 (중요!)


    }

    void Input()
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;

        // F 키 누르면 전체 화면 토글 (0x0001은 이번에 새로 눌렸을 때만 true)
        if (GetAsyncKeyState('F') & 0x0001) {
            ToggleFullscreen();
        }

        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Input();
            }
        }
    }

    void Update()
    {
        // C. 스타트 실행
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                // Start()가 호출된 적 없다면 여기서 호출 (유니티 방식)
                if (gameWorld[i]->components[j]->isStarted == false)
                {
                    gameWorld[i]->components[j]->Start();
                    gameWorld[i]->components[j]->isStarted = true;
                }
            }
        }

        // D. 업데이트 단계 (Update Phase)
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Update(deltaTime);
            }
        }
    }

    void Render()
    {
        // 1. 도화지 지우기 (Clear) - 짙은 파란색 배경
        float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
        g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
        g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

        // 2. 모든 오브젝트 그리기
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Render();
            }
        }

        // 3. 다 그렸으니 화면 교체 (Present)
        g_pSwapChain->Present(0, 0);
    }

    /*void Run()
    {
        // --- [무한 게임 루프] ---
        while (isRunning) {

            // A. 시간 관리 (DeltaTime 계산)
            std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsed = currentTime - prevTime;
            deltaTime = elapsed.count();
            prevTime = currentTime;

            Input();
            Update();
            Render();

            // CPU 과부하 방지 (약 60~100 FPS 유지 시도)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    */

    void Tick() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - prevTime;
        deltaTime = elapsed.count();
        prevTime = currentTime;
        Input();
        Update();
        Render();
    }

    void ToggleFullscreen() {
        if (!g_pSwapChain)return;
        BOOL isFullscreen;
        g_pSwapChain->GetFullscreenState(&isFullscreen, nullptr);

        g_pSwapChain->SetFullscreenState(!isFullscreen, nullptr);
    }

    void SetIsRunning(bool run) { isRunning = run; }

    GameLoop(){}
    
    ~GameLoop()
    {
        for (int i = 0; i < (int)gameWorld.size(); i++) {
            delete gameWorld[i];
        }
        gameWorld.clear();

        // DX11 인터페이스 해제
        if (g_pRenderTargetView) g_pRenderTargetView->Release();
        if (g_pImmediateContext) g_pImmediateContext->ClearState(); // 상태 초기화
        if (g_pImmediateContext) g_pImmediateContext->Release();
        if (g_pSwapChain) g_pSwapChain->Release();
        if (g_pd3dDevice) g_pd3dDevice->Release();
    }

};
GameLoop* g_Engine = nullptr;
// --- [4단계: 메인 엔진 루프] ---
/*int main()
{
    //게임루프
    GameLoop gLoop;
    gLoop.Initialize();

    // 시스템 정보 객체 조립
    GameObject* sysInfo = new GameObject("SystemManager");
    sysInfo->AddComponent(new InfoDisplay());
    gLoop.gameWorld.push_back(sysInfo);

    // 플레이어 객체 조립
    GameObject* player1 = new GameObject("Player1");
    player1->AddComponent(new PlayerControl(0));
    gLoop.gameWorld.push_back(player1);

    // 플레이어 객체 조립
    GameObject* player2 = new GameObject("Player2");
    player2->AddComponent(new PlayerControl(1));
    gLoop.gameWorld.push_back(player2);

    //게임루프 실행
    gLoop.Run();

    return 0;
}*/

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)PostQuitMessage(0);
        if (wParam == 'F' || wParam == 'f') { if (g_Engine)g_Engine->ToggleFullscreen(); }
        break;
    case WM_DESTROY:
        PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GameLoop gLoop;
    gLoop.Initialize(hInstance, nCmdShow);
    g_Engine = &gLoop;
    MSG msg = {};
    while (gLoop.isRunning) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            gLoop.Tick();
        }
    }
    return (int)msg.wParam;
}