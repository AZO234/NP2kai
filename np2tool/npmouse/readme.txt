■■■ Neko Project II シームレスマウスドライバ for WinNT ■■■

Windows上においてNeko Project II上でのシームレスマウス操作を実現します。
【注意！！！】
手順を間違えるとマウスが使えなくなったりブルースクリーンで起動不能になったりします。
マウスが使えない分はキーボード操作で回復できますが、ブルースクリーンの場合は別OSで起動して、
WINNT\System32\driversにあるnpmouse.sysを削除する必要があり、復旧が大変です。
先にイメージ毎バックアップを取っておくのが簡単でおすすめです。

●動作環境
【ゲスト】
Windows NT3.51, NT4.0, 2000
【ホスト】
Neko Project 21/W ver0.86 rev94β11以降
Device->Mouse->Non-capture Controlが有効な環境


●インストール
・Windows 2000の場合
サービスのインストールとマウスドライバへのフィルタ登録が必要です。
サービスはw2k\npmouse.infを右クリック→インストール
でインストールできます。
アップデートも同じ操作でできますが、反映にはシステムの再起動が必要です。
マウスドライバへのフィルタ登録（必須）は、regedt32（regeditではない）を起動し、
HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4D36E96F-E325-11CE-BFC1-08002BE10318}
へ移動します。その中にあるUpperFiltersがmouclassであることを確認します。
そうでない環境は危ないので中断してください。
問題なさそうなら、UpperFiltersの項目をダブルクリックし、以下の2行の内容にします
mouclass
npmouse
順番も大事ですので、逆にしないでください。
これで再起動すればシームレスマウス操作が有効になります。

・Windows NT4.0の場合
レガシードライバ扱いでインストールします。
wnt\npmouse.infを右クリック→インストール
でインストールできます。
アップデートも同じ操作でできますが、反映にはシステムの再起動が必要です。

・Windows NT3.51の場合
付属のwnt\instinf.exeを実行してください。
手動でインストールしたい場合、
wnt\npmouse.inf内に記載の[NP2Mouse.AddReg]セクションの内容を参考に、
regedt32を使用して手動で打ち込んでください。
WINNT\System32\driversにwnt\npmouse.sysをコピーするのを忘れずに。


●アンインストール
・Windows 2000の場合
必ずマウスドライバへのフィルタ登録解除を先に行ってください。
順番を逆にするとマウスドライバがエラーになり、マウス操作ができなくなります。
（キーボード操作だけで頑張れば復旧は出来ます）
regedt32（regeditではない）を起動し、
HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4D36E96F-E325-11CE-BFC1-08002BE10318}
へ移動します。その中にあるUpperFiltersをmouclassの1行だけに戻します。
その後、一旦再起動してシームレス操作が無効化されていることを確認してください。
シームレス操作が無効化されていることを確認したら、レジストリの
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\npmouse
を削除し、WINNT\System32\driversにあるnpmouse.sysを削除すればアンインストール完了です。

・Windows NT3.51，NT4.0の場合
レジストリの
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\npmouse
を削除し、WINNT\System32\driversにあるnpmouse.sysを削除すればアンインストール完了です。


●使い方
シームレス操作は以下の条件を満たす場合に自動的に有効になります。
①マウスキャプチャ状態ではない
②Device→Mouse→Non-capture ControlがON
上記の条件以外では、通常のマウスドライバと同じ相対座標の動作になります。
つまり、シームレスマウスの絶対座標で問題を起こすソフトでは、マウス中ボタンか
F12キーで通常のマウスキャプチャ操作を行えば問題なく使えると思います。


●ソースコード
srcフォルダにソースコードがあります。
いちおう猫本体と同じ修正BSDライセンスとしますが、実質的に自由に使っていただいて大丈夫です。


●更新履歴
2025/06/08 ver.1.0 初版
2025/06/26 ver.1.1 マウスを左端や上端に持っていくと操作が無視される問題を修正


------------------------
Neko Project 21/W 開発者
SimK
