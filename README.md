# NeonVector Engine

ベクターグラフィックス（ネオンの線とグロー）を現代的に描く、小さな DirectX 12 ゲームエンジンライブラリ。
Geometry Wars 系の「発光する線と図形」を、`Application` を継承するだけで書けるようにするのが目的です。

## できること

- **Application フレームワーク** — ウィンドウ生成・メインループ・入力・deltaTime を面倒みる基底クラス。`OnInit / OnUpdate / OnRender` を override するだけ。
- **LineBatcher** — 太さ・色・グロー付きの線をバッチングして高速描画。
- **Primitives** — 円・多角形などを線で描くヘルパー。
- **BloomEffect** — 輝度抽出 → ガウシアンブラー → 合成のポストプロセスで、線を発光させる。
- **RenderTarget** — オフスクリーン描画とステート遷移。

## 必要環境

- Windows 10/11
- **Visual Studio 2022**（C++20）＋ **Windows SDK 10.0.22621 以降**
  （2019 の 10.0.18362 では DirectX-Headers が要求する型が足りずビルドできません）
- CMake 3.20+、vcpkg

## ビルド

```powershell
# 依存取得 (vcpkg) — VCPKG_ROOT を設定してから
cmake -B build
cmake --build build --config Debug
```

`vcpkg.json` で `directx-headers` を **1.616.0 に固定**しています（最新版は Agility SDK 限定のシンボルを要求し、
リテール Windows SDK ではビルドできないため）。

実行ファイルは `build/bin/` に、シェーダは `build/bin/shaders/` に出力されます。

## 例

`examples/` に段階的なサンプルがあります（ビルドすると `build/bin/` に exe ができます）。

| 例 | 内容 |
|---|---|
| `01_HelloTriangle` | 最小の初期化とクリア |
| `02_LineDrawing`   | 線を引く |
| `03_BasicShapes`   | 円・多角形などの図形 |
| `04_BloomDemo`     | 放射線＋脈動する円＋六角形を bloom で発光（Space で bloom ON/OFF） |

## 使い方（最小コード）

```cpp
#include <NeonVector/NeonVector.h>
using namespace NeonVector;

class MyApp : public Application {
public:
    MyApp() : Application({ "My Neon App", 1280, 720 }) {}

    void OnRender() override {
        auto* lines = GetLineBatcher();
        lines->AddLine({100, 100}, {400, 300}, Color::Cyan, 3.0f, 1.0f);
        lines->Flush();
    }
};

int main() { MyApp app; return app.Run(); }
```

## ライセンス

MIT License（`LICENSE`）。Copyright (c) 2025 mogmog-0110。
