# NeonVector Engine

ネオンの線やグローを使ったベクターグラフィックスを描画する、小規模な DirectX 12 ゲームエンジンライブラリです。Geometry Wars 系の発光表現を、`Application` の継承から実装できます。

## 機能

`Application` がウィンドウの作成、メインループ、deltaTime の受け渡しを担います。利用側で override するのは `OnInit`、`OnUpdate`、`OnRender` です。

キーボード入力には `IsKeyDown` と、押した瞬間だけ true になる `WasKeyPressed` を使用します。マウスの位置は `GetMousePosition`、ボタンの状態は `IsMouseButtonDown` で取得できます。

描画は `LineBatcher` が担います。太さ、色、グローを指定した線を蓄積し、一括で描画します。`Primitives` では次の図形を描けます。すべて glow を指定できます。

- 線
- 円
- 円弧
- 矩形
- 多角形
- 正多角形
- 星
- グリッド

`Trail` は移動するオブジェクトの後方に残光を描きます。新しい端ほど明るく、太く表示されます。

`ParticleSystem` は、放射後に減衰して消える発光パーティクルを扱います。ストリークは速度の方向へ伸びます。

発光処理を行うのは `BloomEffect` です。輝度を抽出し、ガウシアンぼかしを適用してから元の画像へ加算します。処理に必要なオフスクリーン描画とステート遷移は `RenderTarget` が管理します。

## 必要環境

- Windows 10／11
- Visual Studio 2022、C++20
- Windows SDK 10.0.22621 以降
- CMake 3.20 以降
- vcpkg

Visual Studio 2019 の Windows SDK 10.0.18362 には DirectX-Headers が要求する型がないため、ビルドできません。

## ビルド

`VCPKG_ROOT` を設定してから実行します。

```powershell
cmake -B build
cmake --build build --config Debug
```

`vcpkg.json` では `directx-headers` を **1.616.0 に固定**しています。最新版は Agility SDK 限定のシンボルを要求するため、リテール版 Windows SDK ではビルドできません。

実行ファイルは `build/bin/`、シェーダは `build/bin/shaders/` に出力されます。

## サンプル

`examples/` に段階的なサンプルがあります。ビルド後の実行ファイルは `build/bin/` に出力されます。

| 例 | 内容 |
|---|---|
| `01_HelloTriangle` | 最小構成の初期化とクリア |
| `02_LineDrawing` | 線の描画 |
| `03_BasicShapes` | 円や多角形などの描画 |
| `04_BloomDemo` | 放射線、脈動する円、六角形を bloom で発光。Space で bloom の ON／OFF を切り替え |
| `05_NeonPlayground` | 拡張プリミティブ、マウスに追従するコメット、クリックで発生する発光パーティクル。1～4 で色を切り替え、Space でバースト |
| `06_Asteroids` | プレイ可能なネオン Asteroids。自機、分裂する小惑星、射撃、当たり判定、爆発、残機、ウェーブ、スコアを実装。←／→で旋回、↑で推進、Space で射撃、R でリスタート |

## 最小コード

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

MIT License（`LICENSE`）。

Copyright (c) 2025 mogmog-0110。
