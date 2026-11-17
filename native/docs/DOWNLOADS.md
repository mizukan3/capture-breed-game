# 検証版のダウンロード

**ゲーム全体の完成版ではありません。** 地形・配置・原モデルを確認する移植検証アプリです。
ターン進行、戦闘AI、捕獲実行、育成、全メニューなどの接続はこれからです。

コード: `a972729c052d5d711574cdcbba15e7a99a9edbe1`。GitHubにログインして成果物をダウンロードし、zipを展開してください。

|対象|ダウンロード|生成物と確認範囲|
|---|---|---|
|Windows x64|[Windows検証版](https://github.com/mizukan3/capture-breed-game/actions/runs/43994574280/artifacts/14290185461)|exeとassets。ビルド成功、Windows実機起動は未確認|
|Android arm64|[Android検証版](https://github.com/mizukan3/capture-breed-game/actions/runs/43994574280/artifacts/14289955611)|Debug APK。ビルド成功、実機起動・タッチ操作は未確認|
|iOS arm64シミュレーター|[iOSシミュレーター検証版](https://github.com/mizukan3/capture-breed-game/actions/runs/43994574280/artifacts/14290215945)|署名なし.appのzip。iPhoneへ直接インストールするものではない|
|Linux|[Linux検証版](https://github.com/mizukan3/capture-breed-game/actions/runs/43994574280/artifacts/14288863818)|ビルド成功、Vulkanで8フレームの起動試験成功|

- Windows: assetsをexeと同じ場所に保持し、capture-native.exeを起動します。Vulkanドライバーが必要です。
- Android: zip内のapp-debug.apkを開発用端末へインストールします。Android 8.0/API26以上・arm64・Vulkanが現在の構成です。GLESフォールバックは未実装です。
- iOS: MacのSimulator用です。実機向けには[手順](../platform/ios/README.md)に従って、署名を設定してビルドしてください。
- アセットと一緒にlicenses、フォントのOFL.txtを保持してください。

[アプリのビルド記録](https://github.com/mizukan3/capture-breed-game/actions/runs/43994574280) ／ [原版比較・保存復旧・測定](https://github.com/mizukan3/capture-breed-game/actions/runs/43994574281)

GitHub Actionsの成果物には保存期限があります。期限後はREADMEに従ってソースからビルドするか、ワークフローを再実行してください。
