#define	SSTP_READY		0
#define	SSTP_SENDING	1
#define	SSTP_BUSY		-3
#define	SSTP_NOSEND		-2
#define	SSTP_ERROR		-1


void sstp_construct(HWND hwnd);
void sstp_destruct(void);

void sstp_connect(void);
void sstp_readSocket(void);
void sstp_disconnect(void);

BOOL sstp_send(const OEMCHAR *msg, void (*proc)(HWND hWnd, char *msg));
int sstp_result(void);

BOOL sstp_sendonly(const OEMCHAR *msg);

