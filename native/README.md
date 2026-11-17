# C++20 / SDL3への完全移植（開発途中）

元HTMLを維持してnative/へ段階的に移植しています。
現在の実行ファイルは、元の地形・配置・モデルを確認する**移植検証アプリ**です。
ターン進行・AI・捕獲などを接続した完成ゲームではありません。

参照できたGitHubのHTMLにはv0.23.10/v53と記載されています。
添付v0.23.14との同一性と、添付v62との差分は未確認です。

- [解析と移植計画](docs/ANALYSIS.md)
- [実装・未実装・未検証](docs/STATUS.md)
- [性能と測定の範囲](docs/PERFORMANCE.md)
- [課題・対応記録](docs/ISSUES.md)
- [ライブラリ・ライセンス](licenses/README.md)
- [参照元の識別](data/provenance.json)

## Windowsでビルド

Visual StudioのC++20開発環境、CMake 3.24以上、比較テスト用のNode.js 22、
シェーダーを作成するglslangValidator（Vulkan SDK付属など）が必要です。
最初のconfigureで、コミットを固定した依存ソースと日本語フォントを取得します。

リポジトリのルートで実行します。

~~~powershell
cmake -S . -B build -DCAPTURE_BUILD_APP=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
.\build\bin\Release\capture-native.exe
~~~

glslangValidatorがない場合は、CIのnative-shaders成果物を展開し、
-DCAPTURE_SHADER_DIR=C:/path/to/shaders をconfigureへ追加できます。

実行時はVulkan対応GPU/ドライバーが必要です。
今回のWindows版にはDXILシェーダーとGLESフォールバックがまだありません。
実行ファイルと同じ場所のassetsフォルダーを保持してください。
MSVCのランタイムが未導入のPCではVisual C++再頒布可能パッケージが必要です。

## Linuxでビルド

C++20、CMake、Node.js、glslang-tools、SDL向けウィンドウ/音声の開発パッケージが必要です。
CIで使用するaptパッケージは.github/workflows/native-app.ymlに記載しています。

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCAPTURE_BUILD_APP=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/bin/capture-native
~~~

## ヘッドレスコアだけを検証

SDLとGPUを使わず、同じC++コアをビルドします。

~~~sh
cmake -S . -B build-core -DCMAKE_BUILD_TYPE=Release -DCAPTURE_BUILD_APP=OFF
cmake --build build-core --config Release --parallel 2
ctest --test-dir build-core -C Release --output-on-failure
cmake --build build-core --config Release --target profile-map
~~~

テストは元JavaScriptを隔離VMへそのまま読み込み、C++と比較します。
原本のgit blobを検査し、参照の変更による期待値の無断更新を防ぎます。
Node.jsは開発時だけ必要で、ネイティブアプリはJavaScriptやWebViewを使用しません。

## 検証アプリの操作

- 前の階・次の階: 1〜6Fの生成結果を表示
- 別シード: 別の固定乱数状態の生成結果を表示
- 回転・拡大: 元HTMLのカメラ寸法で表示
- 矢印キー: カメラを移動（ゲームキャラクターの移動ではありません）
- E: 回転、Z: 拡大、Escape: 終了
- ボタンはマウス・タッチの双方に対応
- 「音 ON」でUI効果音を有効化
- ウィンドウ変更・縦横画面・セーフエリアに合わせて配置を調整

バックグラウンドでは描画を休止し、音声を止めます。
現段階で保存するのは検証画面の設定だけです。
ゲーム保存アルゴリズムは独立したC++コアへ移植・試験済みで、ゲーム本体とファイル保存への接続は未実装です。

## Android

Android SDK 35、NDK 27.2.12479018、CMake 3.31.1、Java 17、Gradle 8.9、
Node.js 22、ホスト側glslangValidatorを使用します。
Android StudioのSDK Managerで必要なSDK/NDK/CMakeを用意してください。
Gradle wrapperは同梱していないため、Gradle 8.9をPATHに置いて使用します。

~~~sh
git init .native-deps/SDL
git -C .native-deps/SDL fetch --depth 1 https://github.com/libsdl-org/SDL.git f00dc919ca35b9756316e86c4777392de442f9b2
git -C .native-deps/SDL checkout --detach FETCH_HEAD
node native/tools/prepare_mobile_assets.mjs build-mobile/assets
gradle -p native/platform/android assembleDebug
~~~

SDKの場所は環境のANDROID_HOME、またはnative/platform/android/local.propertiesのsdk.dirで指定します。
APKの出力先はnative/platform/android/app/build/outputs/apk/debug/です。
対象ABIはarm64-v8a、minSdk 26です。現在はVulkanが必要です。

SDLのJavaソースは固定コミットを使用し、ゲーム部分はデスクトップと同じC++コードです。
別のアセット/SDLソース場所は-PcaptureAssetsDir=... / -PcaptureSdlDir=...で指定できます。

## iOS

[iOS / Xcode手順](platform/ios/README.md)を参照してください。
シミュレータービルドには署名が不要です。実機実行には利用できる開発チームと署名が必要です。
シミュレーター向けコンパイル成功と実機の動作確認を分けて記録します。

## 素材

5種、探索者、7種の道具モデルは原本のGeometryと描画関数から、
時刻0・歩行0・攻撃0の頂点/色をFloat32で抽出しています。
アニメーション曲線、影、輪郭、戦闘演出の移植は別工程です。

~~~sh
node native/tools/extract_assets.mjs --check
~~~

assets/fonts/OFL.txtをフォントと一緒に配布してください。
独自フォントを使う場合は-DCAPTURE_FONT_PATH=...で指定し、そのフォントの配布条件を確認してください。
