■■■ Neko Project II HLT IDLEドライバ for Win9x ■■■

Neko Project II上のWin9xでアイドル時にHLT命令を呼ぶVxDドライバです。
Win9x現役時代にあったRain 2.0やCPUIdle等のCPU冷却系プログラムに相当します。

ただし、これら現役時代のソフトと比較するとかなり簡易的な処理になっています。
実CPUで実行したときどうなるかは検証できていません。エミュレータ専用です。


●動作環境
【ゲスト】
Windows 95, 98

【ホスト】
Neko Project 21/W


●インストール
ドライバをゲストOSに登録して使用します。

① Win9xのSYSTEMディレクトリにnp2idle.vxdをコピーします。
② SYSEDIT等でSYSTEM.INIを編集します。
[386Enh]セクションの最後あたりに
device=np2idle.vxd
を追記して保存します。
③ Windowsを再起動してください。


●アンインストール
上記の逆の手順で行えます。
SYSTEM.INIからdevice=np2idle.vxdを削除し再起動した後、
Win9xのSYSTEMディレクトリにのnp2idle.vxdを削除します。


●使い方
登録すれば特に何もしなくてもHLT IDLEが有効になります。
ホストのタスクマネージャを見て、アイドル時にnp21/wのCPU使用率が
下がっていれば成功です。

他に、便利な可能性があるツールをいくつか付けています。
○NP2ICTRL.EXE
コマンドからHLT IDLEの有効無効を切り替えます。
オプションONで有効化、OFFで無効化、なしで現在状態表示です。

○NP2ITRAY.EXE
タスクトレイ常駐して右クリックでHLT IDLEの有効無効を切り替えます。
常駐が嫌いであれば起動して設定を変えてから終了しても害はありません。
それをするのであればコンソール版が良いと思いますが…。


●ソースコード
Neko Project 21/Wのソースに含まれます。
ライセンスはNeko Project II本体と同じ修正BSDライセンスに従います。


------------------------
Neko Project 21/W 開発者
SimK
