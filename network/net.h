/**
 * @file	net.h
 * @brief	Virtual LAN Interface
 *
 * @author	$Author: SimK $
 */

typedef void NP2NET_PacketHandler(const UINT8 *buf, int size);

// send_packetはデータをLANに送信したいときに外から呼ばれます。データを送信する関数を作ってセットしてやってください。
// recieve_packetはLANからデータを受信したときに呼んでください。この関数はリセット時にデバイスがセットしに来るので作る必要はありません。
// 現在はTAPデバイスのみのサポートですが、send_packetとrecieve_packetに相当する物を作ってやればTAP以外でもOKなはず
typedef struct {
	NP2NET_PacketHandler	*send_packet;
	NP2NET_PacketHandler	*recieve_packet;
} NP2NET;

#ifdef __cplusplus
extern "C" void np2net_init(void);
extern "C" void np2net_shutdown(void);
#else
extern void np2net_init(void);
extern void np2net_shutdown(void);
#endif
void np2net_reset(const NP2CFG *pConfig);
void np2net_bind(void);

extern	NP2NET	np2net;