■■■ Neko Project II システムポートドライバ for WinNT ■■■

Windows NT系からNeko Project IIのシステムポートにアクセスするためのデバイスドライバです。
エミュレータ内の仮想環境からNeko Project IIと通信できます。
Neko Project 21/Wの場合、動的CPUクロック変更などが行えます。

●動作環境
Windows NT3.51, NT4.0, 2000


●インストール
・Windows NT4.0, 2000の場合
レガシードライバ扱いでインストールします。
npsysprt.infを右クリック→インストール
でインストールできます。
アップデートも同じ操作でできますが、反映にはシステムの再起動が必要です。
ほか、付属のregファイルをインポートしてもインストールできます。

・Windows NT3.51の場合
付属のinstinf.exeを実行してください。
手動でインストールしたい場合、
npsysprt.inf内に記載の[NP2SystemPort.AddReg]セクションの内容を参考に、
regedt32を使用して手動で打ち込んでください。


●アンインストール
・Windows NT3.51, NT4.0の場合
コントロールパネル→デバイスでNeko Project II System Portのスタートアップを無効にしてください。
完璧に消したい場合、レジストリエディタで
HKLM\System\CurrentControlSet\Services\npsysprt
を消してください。
ファイルも消したい場合は
System32\drivers\npsysprt.sys
を消してください。

・Windows 2000の場合
デバイスマネージャ→表示→非表示のデバイスの表示
でプラグアンドプレイではないドライバを表示させ、
Neko Project II System Portを削除してください。
上手く消えない場合はWindows NT3.51, NT4.0の場合と同じ手順で削除してください。


●使い方
npcngclk.exeはNeko Project 21/Wの動的CPUクロック変更のためのツールです。
npcngclk <変更したいクロック倍率>
で、動的にCPUクロック倍率が変わります。この変更はEmulate→Resetで元に戻ります。

npsyscmd.exeはNeko Project IIのシステムポートに任意のコマンドを送信できるツールです。
npsyscmd <コマンド> [引数byte1] [引数byte2] [引数byte3] [引数byte4]
で送信できます。なんでも送れてしまうので要注意（例えばpoweroffとかを送るとnp2が終了します）。

例えば、
npsyscmd NP2
を実行すると、NP2と返ってきます。

送信できるコマンドはNeko Project IIのソースコードや以下のページに記載があります。
https://simk98.github.io/np21w/docs/npcngcfg.html

npgetpos.exeはNeko Project 21/Wにおいてマウス絶対座標を読み取るテストツールです。
対象バージョンはNeko Project 21/W rev94 beta11以降です。対応していない場合はエラーになります。
npgetpos
で、マウス絶対座標を取得できます。
生の値（X,Yともに0～65535でマップされた値）とスクリーン座標に換算された値を表示します。


●ソースコード
np21/wソースのnp2tool\npsysprtにソースコードがあります。
いちおう猫本体と同じ修正BSDライセンスとしますが、実質的に自由に使っていただいて大丈夫です。
煮るなり焼くなり切り貼りするなり自由にしてください。


------------------------
Neko Project 21/W 開発者
SimK
