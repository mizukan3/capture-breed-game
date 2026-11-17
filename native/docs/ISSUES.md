# 課題と対応記録

- 初回CI: Linux/WindowsでC++20のコンパイルとリンクは成功。
- 比較テストの入力用モックにspeciesがなく、原本のログ用名称参照が失敗した。対象にspecies=0を設定して修正。ゲーム本体の処理は変更しない。
- Windows checkoutのCRLF変換で原本blob検証が失敗した。.gitattributesでindex.htmlのバイト列を保持し、他のテキストはLFへ統一した。期待ハッシュを緩める対応はしない。
