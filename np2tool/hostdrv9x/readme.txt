■■■ Neko Project II ホスト共有ドライブドライバ for Win9x ■■■

Windows 9xからNeko Project IIのホスト共有ドライブにアクセスするためのVxD
ドライバです。エミュレータ内のWindowsからホストPCの指定フォルダにアクセス
できます。

DOS版HOSTDRV.COMのWindows 9x版に相当し、ロングファイルネームにも対応して
います。

【注意！！】
Neko Project IIのバグやゲストOSの暴走が発生した場合、その影響がホストのファイル
にも及ぶことになりますので、共有範囲とアクセス権限は必要最小限にとどめる事を
おすすめします（少なくとも共有範囲外は安全です）。
本機能を使用してファイルが消失した場合でも、作者は責任を負いません。
そのことに同意頂けない場合は使用しないでください。


●動作環境
【ゲスト】
Windows 95, 98

【ホスト】
Neko Project 21/W ver0.86 rev104 β5以降
Hostdrvを有効にし、共有するホスト側フォルダを設定している環境

古いDOS版HOSTDRVが常駐している場合、ドライブ表示が変になります。
最新のDOS版HOSTDRVを使用してください。


●インストール
ドライバをゲストOSにインストールして使用します。

① Neko Project II側でHostdrvを有効にし、共有フォルダを設定してください。
② HOSTD9X.INFを右クリック→インストールしてください。
③ Windowsを再起動してください。

アップデートも同じ操作で行えますが、反映にはシステムの再起動が必要です。

付属のHOSTD9X.REGをインポートして手動で登録することもできます。
この場合はHOSTD9X.VXD,HOSTD9X.DLLをWindows\SYSTEMへ手動でコピーしてください。
通常はINFによるインストールをおすすめします。


●アンインストール
レジストリエディタを起動し、
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\VxD\HOSTD9X
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\NetworkProvider\Order\HOSTD9XNP
HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\HOSTD9XNP
を削除して再起動してください。
再起動後に問題なければWindows\SYSTEM\HOSTD9X.VXD, HOSTD9X.DLLを削除してください。


●DOS版HOSTDRV.COMからの引き継ぎ
DOSでHOSTDRV.COMを常駐した状態からWindowsを起動できるようになっています。

既定の設定では、DOS版で使用していたドライブ文字をHOSTD9Xへ引き継ぎます。
Windows終了時にはVxD側の割り当てを解除し、DOS版HOSTDRVへ復帰します。

例：
  DOS上でHOSTDRV.COMがZ:を使用
       ↓ Windows起動
  HOSTD9XがZ:を引き継ぐ
       ↓ Windows終了
  DOS版HOSTDRVがZ:へ復帰

同じドライブ文字を正しく引き継ぐには、最新版HOSTDRV.COMを使用してください。
古いHOSTDRV.COMではDOSのCDS情報をVxD側へ引き継げないため、
空のDOS由来ドライブが残ったり、別のドライブ文字へ割り当てられる場合があります。

DOS版HOSTDRVをWindows起動前に手動で常駐解除する必要はありません。


●ドライブ文字の設定
以下のレジストリキーで設定できます。再起動で反映されます。

HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\VxD\HOSTD9X

○DriveMode
ドライブ文字の割り当て方法を指定します。

  0 = DOS版HOSTDRVのドライブ文字を引き継ぎます（既定）
      DOS版HOSTDRVが常駐していない場合は自動割り当てになります。
  1 = 自動割り当てします。
      Z:→D:の順に空いているドライブ文字を探します。
  2 = DriveLetterで指定した文字に固定します。

○DriveLetter
DriveMode=2のときに使用するドライブ文字を指定します。
A～Zの1文字を指定してください。"Z:"のようにコロンを付けても構いません。
既定値はZ:です。

○FallbackToAuto
DriveMode=0でDOS版のドライブ文字を引き継げなかった場合の動作を指定します。

  0 = 自動割り当てを行わない
  1 = Z:からD:の順に空きを探す（既定）


●ドライブ容量の設定
大容量ディスクをそのままゲストOSへ見せることによる互換性問題を避けるため、
既定ではHOSTDRVNTと同様に疑似的な容量を返します。

設定場所：
HKLM\System\CurrentControlSet\Services\VxD\HOSTD9X

○UseRealCapacity
ディスクの使用状況を実容量にするか疑似容量にするかを指定します。
OSが大容量ディスクを想定していない場合は疑似容量で問題を回避できます。
名目上の容量のため、実際にこの容量以上を使用できないわけではありません。

  0 = 疑似容量を返す（既定）
  1 = ホスト側ディスクの実容量を返す

○FakeCapacityMB
UseRealCapacity=0のときに返す総容量をMB単位のREG_DWORDで指定します。
既定値は2048MB（2GB）です。

○FakeFreeMB
UseRealCapacity=0のときに返す空き容量をMB単位のREG_DWORDで指定します。
既定値は1024MB（1GB）です。
総容量を超える値を指定した場合は総容量までに制限されます。

疑似容量モードでは512バイト/セクタ、8セクタ/アロケーションユニットとして
容量情報を返します。


●ソースコード
Neko Project 21/Wのソースに含まれます。
VxDはWindows 9x IFSMgrから受け取った情報をnp2のhostdrv9x.cに丸投げします。
DLLはネットワークドライブとして認識させるために必要なNetwork Providerです。
ライセンスはNeko Project II本体と同じ修正BSDライセンスに従います。


●安全に使うためのヒント
Neko Project IIのHostdrv設定でアクセス権限を外すと、その操作を禁止できます。
たとえばWRITEやDELETE権限を消しておけば、ゲストOSの暴走によってホスト側の
ファイルを勝手に書き換えたり削除されたりする被害を抑えることができます。

また、共有するホスト側フォルダは必要最小限の範囲にしてください。


------------------------
Neko Project 21/W 開発者
SimK
