# C++20 / SDL3 完全移植プロジェクト（開発途中）

元のindex.htmlを維持し、native/へ段階的に移植する。現在のコードは基盤と数学・地形の移植であり、完成したゲーム全体ではない。

- [解析・計画](docs/ANALYSIS.md)
- [実装/未実装/未検証](docs/STATUS.md)
- 参照版の識別: data/provenance.json

## ヘッドレスビルド

CMake 3.24以降、C++20コンパイラ、Git、比較テストにはNode.js 22が必要。
最初のconfigureで、コミットを固定したnlohmann/jsonを取得する。

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCAPTURE_BUILD_APP=OFF
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
~~~

リポジトリのルートで実行する。DebugはCMAKE_BUILD_TYPE=Debug（Visual Studioでは--config Debug）。
浮動小数のfast-mathは禁止。数学・乱数・BFSの順序を保持する。

html-parityは元のJavaScriptをそのまま隔離VMへ読み込み、C++ executableとJSONで比較する。
原本のgit blobを検査し、原本変更による期待値の無断更新を防ぐ。添付v0.23.14の検証を意味するテストではない。
