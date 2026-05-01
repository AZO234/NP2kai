## wxWidgets + SDL ポート

### 技術

- GUIに、OS非依存UIライブラリである wxWidgets を使用します。  
  派手さはありませんがOSネイティブのUIに仕上がり、軽量＆高速なGUIを提供します。
  MinGW、Linux、macOSでビルドが可能です。
- 基礎ライブラリである SDL を使用します。OS非依存を実現するとともに、  
  ハードウェアアクセラレーションなども利用できます。
- 設定保存には従来のINIではなく、型による記法が可能なTOMLで記述します。  
  libtomlplusplus を使用します。
- NP2オリジナルのCDイメージ実装に加え、libcdio を使用します。
- PNG処理に、libpng を使用します。
- オプションでVST3をサポートします。
