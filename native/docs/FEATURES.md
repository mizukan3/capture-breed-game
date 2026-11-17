# 原HTMLとの対応

参照コミット: 19575b6658adda051020359d87b6ff0cc2871360、index.html。
「完了」は記載した関数/データの範囲だけを意味し、ゲーム全体の完了を意味しません。

|元モジュール/関数|C++/データの場所|範囲・状態|
|---|---|---|
|data.js 定義|data/game.json、core/data.cpp|参照版の全定義を抽出。JSON全体を比較|
|growthValue / speciesStats / playerBaseStats|core/balance.cpp|計算式移植。種ごとLv1〜100を比較|
|normalDamage / xpNeeded|core/balance.cpp|計算式移植|
|Game.random / rand|include/capture/rng.hpp|xorshift32と乱数消費を比較|
|Game.generateFloor|core/map.cpp|部屋・通路・敵・道具・階段配置を移植。初期同行なしの状態で照合|
|reachableDistances / canStep / lineOfSight / reveal|core/map.cpp|8方向順・視線・探索済みを維持|
|Game.stats|core/balance.cpp|状態補正の計算部。装備/融合の接続は未実装|
|Game.hit|core/combat.cpp|命中/ガード/会心/凍結の計算。HP適用・イベント・撃破報酬は未接続|
|Game.captureCondition|core/balance.cpp|純粋な条件判定。捕獲実行と資源・仲間更新は未実装|
|view.js|core/camera.cpp|正射影寸法と可視範囲、画面方向変換。補間は未移植|
|save-store.js|core/save_store.cpp|主/予備/一時・連番・チェックサム・選択・復旧・削除を移植|
|save-slots.js|core/save_store.cpp|スロットキーのみ。5スロットUI・移行は未実装|
|native-runtime.js envelope|core/save_store.cpp|ネイティブ包絡形式の生成・検査。ゲームへのファイル保存は未接続|
|Geometry / world-art.js|app/geometry.cpp|床壁と階段、台座・たいまつ土台を移植|
|beginning-art.js / explorer / lootModel|assets/meshes、tools/extract_assets.mjs|原本の静止姿勢をFloat32抽出。動作の手続き生成は未移植|
|renderer.js|app/gpu.cpp / main.cpp|SDL_GPUの深度・霧・光・モデル表示。原版の全演出は未移植|
|audio.js|app/audio.hpp|UI音のみ。BGM・全SE・鳴き声は未移植|
|inventory-ui.js / HTML/CSS/UI|app/ui.cpp|文字・画像・タッチ基盤のみ。ゲームメニューは未移植|
|monster-combat.js|未移植|AI計画・行動順・同時移動・各技の全移植が必要|
|motion.js / skill-visuals.js / status-icons.js|未移植|アニメーションと画面効果・状態表示が必要|
|monetization.js|未移植|課金/広告/購入復元のネイティブ対応は未実装|

元HTMLは実行時に必要ありません。開発側の抽出・比較テストだけが参照します。
