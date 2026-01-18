/*
 *	minisop.h
 */
 
// 先に#include <ntddk.h>が必要。

// これを定義しておくとキャッシュクリア用のWORKAROUND関数を使用可能
#define USE_CACHEHACK

// SOPリストを解放する。中身のメモリも解放される。ドライバのアンロード時に呼び出す。
BOOLEAN MiniSOP_ReleaseSOPList();

// 指定したファイル名に対応するSOPリストのインデックスを返す。
// リストに無い場合は新規割り当てする。リソース不足で割り当てできない場合は-1を返す。
// 呼ぶ度に参照カウントされるので、不要になったら必ず同じ回数HostdrvReleaseSOPByIndexまたはHostdrvReleaseSOPをすること。
LONG MiniSOP_GetSOPIndex(UNICODE_STRING path);

// 指定したSOPリストのインデックスに対応するSECTION_OBJECT_POINTERSへのポインタを返す。
// 無効なインデックスであればNULLが返る
PSECTION_OBJECT_POINTERS MiniSOP_GetSOP(LONG i);

// 指定したSOPの参照カウントを減らす　0になったら解放される。
VOID MiniSOP_ReleaseSOP(PSECTION_OBJECT_POINTERS lpSOP);
// 指定したインデックスのSOPの参照カウントを減らす　0になったら解放される。
VOID MiniSOP_ReleaseSOPByIndex(LONG i);


// HACK: おまけ機能　キャッシュ未実装でもメモリマップトファイルが使えるようにする。
#ifdef USE_CACHEHACK

// 未実装の中途半端キャッシュのための初期化。DriverEntryで呼ぶこと。
// DriverObject: DriverEntryの引数で受け取ったDriverObject
VOID MiniSOP_InitializeCache(PDRIVER_OBJECT DriverObject);
// IRP_MJ_CREATEが成功する度に呼ぶ。キャッシュ消去してディスクからの再READを強制。
// irpSp: IRPスタックポインタ
VOID MiniSOP_HandleMjCreateCache(PIO_STACK_LOCATION irpSp);
// IRP_MJ_CLEANUPが成功する度に呼ぶ。キャッシュの内容を強制的にディスクにWRITEさせる。
// irpSp: IRPスタックポインタ
VOID MiniSOP_HandleMjCleanupCache(PIO_STACK_LOCATION irpSp);
// IRP_MJ_SET_INFORMATIONで実際のファイルサイズを変更する 前に 呼ぶ。キャッシュのファイル長さ変更を通知。
// 実際の処理は、指定されたファイル長さ以上で一旦WRITEされる→SetInformationで切り捨てる　という操作になる
// irpSp: IRPスタックポインタ
VOID MiniSOP_HandlePreMjSetInformationCache(PIO_STACK_LOCATION irpSp);

#endif  /* USE_CACHEHACK */
