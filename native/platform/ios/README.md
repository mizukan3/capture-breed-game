# iOS / Xcode

現在の成果物は移植検証アプリ。ゲーム本体は開発途中。
macOS、Xcode（iOS 15以上のSDK）、CMake 3.24以上、Gitが必要。
SDL3/ttf/imageとフォントは固定した取得元からconfigure時に取得する。
MetalシェーダーはMSLソースからSDL_GPUで作成する。

## シミュレーター（署名不要）

リポジトリルートから実行する。

~~~sh
cmake -S . -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
  -DCAPTURE_BUILD_APP=ON -DCAPTURE_BUILD_TESTS=OFF
cmake --build build-ios --config Release --parallel 2
open build-ios/CaptureBreedNative.xcodeproj
~~~

Intel Macはアーキテクチャをx86_64へ変更。アセットはビルド時に.app/assetsへ配置する。
CIのコンパイル成功と、シミュレーター/実機での起動成功は別々に記録する。

## iPhone / iPad

別のbuild-ios-deviceへ生成し、sysrootをiphoneos、アーキテクチャをarm64にする。
CODE_SIGNING_ALLOWED=NOは外す。Xcodeでcapture-nativeターゲットを選び、
Signing & CapabilitiesのTeamと、利用できる一意なBundle Identifierを設定する。
接続した端末を選んでRun。証明書・Provisioning Profileはリポジトリへ保存しない。

## 検証項目

日本語の欠字、縦横画面、安全領域、タッチ、バックグラウンド/復帰、音声割り込み、
Metalエラー、メモリ、発熱、電力を実機で確認する。未確認項目を達成済みにしない。
ゲームの保存/復旧は未実装で、現段階の設定保存と混同しない。
