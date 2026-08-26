# NeonVector Engine

ベクターグラフィックス（ネオンの線とグロー）を現代的に描く、小さな DirectX 12 ゲームエンジンライブラリ。
Geometry Wars 系の「発光する線と図形」を、`Application` を継承するだけで書けるようにするのが目的です。

## できること

`Application` を継承すればもう動く。ウィンドウを作り、メインループを回し、deltaTime を渡すところまでは
基底クラスが持っている。あとは `OnInit / OnUpdate / OnRender` を override するだけ。入力は
`IsKeyDown` と `WasKeyPressed`（押した瞬間だけ true）、マウスは `GetMousePosition` と `IsMouseButtonDown`。

描画の中心は `LineBatcher`。太さと色とグローを指定した線をためておいて、まとめて 1 回で流す。
その上に `Primitives` が乗っていて、線・円・円弧・矩形・多角形・正多角形・星・グリッドが引ける。
どれも glow を指定できる。

`Trail` は動くものの後ろに残光を引く。新しい端ほど明るく、太い。`ParticleSystem` は放射して
減衰して消える発光パーティクルで、速度の向きへストリークが伸びる。

線を実際に光らせているのは `BloomEffect`。輝度を抜き、ガウシアンでぼかし、元の絵へ足し戻している。
そのためのオフスクリーン描画とステート遷移は `RenderTarget` が持つ。

## 必要環境

- Windows 10/11
- **Visual Studio 2022**（C++20）＋ Windows SDK 10.0.22621 以降
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
| `05_NeonPlayground`| 拡張プリミティブ＋マウス追従コメット＋クリックで発光パーティクル（1-4 色切替 / Space バースト） |
| `06_Asteroids`     | **遊べるネオン Asteroids**。自機・分裂する小惑星・射撃・当たり判定・爆発・残機・ウェーブ・スコア。←→旋回 / ↑推進 / Space 射撃 / R リスタート |

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
