#include	<stdio.h>
#include	<string.h>
#include	"common.h"
#include	"dosio.h"

// ----------------------------------------- メイン

int main(int argc, BYTE *argv[], BYTE *envp[]) {

	FILEH	fh;
	BYTE	buf[12];
	BYTE	out[256];
	WORD	i;
	WORD	j;

	if (argc < 2) {
		printf("ファイルを指定して下さい.\n");
		return(1);
	}
	if ((fh = file_open(argv[1])) == -1) {
		printf("ファイルが見つかりません.\n");
		return(2);
	}

	if (argc >= 3) {
		printf("\nstatic const unsigned char %s[] = {\n", argv[2]);
	}
	else {
		int		i;
		for (i=0; ; i++) {
			if (!argv[1][i]) {
				break;
			}
			if (argv[1][i] == '.') {
				break;
			}
			out[i] = argv[1][i];
		}
		out[i] = 0;
		printf("\nstatic const unsigned char %s[] = {\n", out);
	}

	while(1) {
		j = file_read(fh, buf, 12);
		if ((j == 0) || (j == -1)) {
			break;
		}
		for (i=0; i<j; i++) {
			sprintf(&out[i*5], "0x%02x,", buf[i]);
		}
		printf("\t\t\t%s\n", out);
	}
	file_close(fh);
	printf("};\n\n");

	return(0);
}

