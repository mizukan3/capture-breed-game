# 課題と対応記録

- 初回CI: Linux/WindowsでC++20のコンパイルとリンクは成功。
- 比較テストの入力用モックにspeciesがなく、原本のログ用名称参照が失敗した。対象にspecies=0を設定して修正。ゲーム本体の処理は変更しない。
- Windows checkoutのCRLF変換で原本blob検証が失敗した。.gitattributesでindex.htmlのバイト列を保持し、他のテキストはLFへ統一した。期待ハッシュを緩める対応はしない。

- Android CI初回: sdkmanagerがPATHに存在せず終了。SDKセットアップを明示する処理を追加する。
- iOS初回: configureとcompileは成功したが、成果物アップロード先をbin/Release-iphonesimulatorと誤指定した。実際のbin/Releaseへ修正し、.appの実行権限を保つためdittoでzip化する。
- 依存取得: リポジトリの全履歴cloneに時間がかかったため、同一の固定コミットのソースアーカイブを使う方式へ変更。FreeTypeはSDL_ttfが指すサブモジュールのコミットを維持する。

- AndroidのGradleプロジェクトにsettings.gradleを追加し、appサブプロジェクトとGoogle/Mavenの取得先を明示する。
- バックグラウンド復帰時には測定窓をリセットし、休止時間を通常描画フレームの平均へ混ぜない。

- Android SDKセットアップの追加後、アクション既定の「tools」が現在のSDK配布から取得できず終了した。packagesを必要なplatform-toolsへ限定し、別ステップでNDK/SDK/CMakeを指定する。
- iOSのXcodeジェネレーターでの構成検査に時間がかかったため、CIは同じiOS SDKを使用するNinjaで検証する。開発者向けXcode生成手順はREADMEに維持し、初回のXcodeコンパイル成功も記録する。
