/*
 * Copyright (c) 2004 NONAKA Kimihiro
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "cpu.h"
#include "inst_table.h"


/*
 * opcode strings
 */
static const char *opcode_1byte[2][256] = {
/* 16bit */
{
/*00*/	"addb",  "addw",  "addb",  "addw",  "addb",  "addw",  "push",  "pop",
	"orb",   "orw",   "orb",   "orw",   "orb",   "orw",   "push",  NULL,
/*10*/	"adcb",  "adcw",  "adcb",  "adcw",  "adcb",  "adcw",  "push",  "pop",
	"sbbb",  "sbbw",  "sbbb",  "sbbw",  "sbbb",  "sbbw",  "push",  "pop",
/*20*/	"andb",  "andw",  "andb",  "andw",  "andb",  "andw",  NULL,    "daa",
	"subb",  "subw",  "subb",  "subw",  "subb",  "subw",  NULL,    "das",
/*30*/	"xorb",  "xorw",  "xorb",  "xorw",  "xorb",  "xorw",  NULL,    "aaa",
	"cmpb",  "cmpw",  "cmpb",  "cmpw",  "cmpb",  "cmpw",  NULL,    "aas",
/*40*/	"incw",  "incw",  "incw",  "incw",  "incw",  "incw",  "incw",  "incw",
	"decw",  "decw",  "decw",  "decw",  "decw",  "decw",  "decw",  "decw",
/*50*/	"push",  "push",  "push",  "push",  "push",  "push",  "push",  "push",
	"pop",   "pop",   "pop",   "pop",   "pop",   "pop",   "pop",   "pop",
/*60*/	"pusha", "popa",  "bound", "arpl",  NULL,    NULL,    NULL,    NULL,
	"push",  "imul",  "push",  "imul",  "insb",  "insw",  "outsb", "outsw",
/*70*/	"jo",    "jno",   "jc",    "jnc",   "jz",    "jnz",   "jna",   "ja",
	"js",    "jns",   "jp",    "jnp",   "jl",    "jnl",   "jle",   "jnle",
/*80*/	NULL,    NULL,    NULL,    NULL,    "testb", "testw", "xchgb", "xchgw",
	"movb",  "movw",  "movb",  "movw",  "movw",  "lea",   "movw",  "pop",
/*90*/	"nop",   "xchgw", "xchgw", "xchgw", "xchgw", "xchgw", "xchgw", "xchgw",
	"cbw",   "cwd",   "callf", "fwait", "pushf", "popf",  "sahf",  "lahf",
/*a0*/	"movb",  "movw",  "movb",  "movw",  "movsb", "movsw", "cmpsb", "cmpsw",
	"testb", "testw", "stosb", "stosw", "lodsb", "lodsw", "scasb", "scasw",
/*b0*/	"movb",  "movb",  "movb",  "movb",  "movb",  "movb",  "movb",  "movb",  
	"movw",  "movw",  "movw",  "movw",  "movw",  "movw",  "movw",  "movw",  
/*c0*/	NULL,    NULL,    "ret",   "ret",   "les",   "lds",   "movb",  "movw",
	"enter", "leave", "retf",  "retf",  "int3",  "int",   "into",  "iret",
/*d0*/	NULL,    NULL,    NULL,    NULL,    "aam",   "aad",   "salc",  "xlat",
	"esc0",  "esc1",  "esc2",  "esc3",  "esc4",  "esc5",  "esc6",  "esc7",
/*e0*/	"loopne","loope", "loop",  "jcxz",  "inb",   "inw",   "outb",  "outw",
	"call",  "jmp",   "jmpf",  "jmp",   "inb",   "inw",   "outb",  "outw",
/*f0*/	"lock:", "int1",  "repne", "repe",  "hlt",   "cmc",   NULL,    NULL,
	"clc",   "stc",   "cli",   "sti",   "cld",   "std",   NULL,    NULL,
},
/* 32bit */
{
/*00*/	"addb",  "addl",  "addb",  "addl",  "addb",  "addl",  "pushl", "popl",
	"orb",   "orl",   "orb",   "orl",   "orb",   "orl",   "pushl", NULL,
/*10*/	"adcb",  "adcl",  "adcb",  "adcl",  "adcb",  "adcl",  "pushl", "popl",
	"sbbb",  "sbbl",  "sbbb",  "sbbl",  "sbbb",  "sbbl",  "pushl", "popl",
/*20*/	"andb",  "andl",  "andb",  "andl",  "andb",  "andl",  NULL,    "daa",
	"subb",  "subl",  "subb",  "subl",  "subb",  "subl",  NULL,    "das",
/*30*/	"xorb",  "xorl",  "xorb",  "xorl",  "xorb",  "xorl",  NULL,    "aaa",
	"cmpb",  "cmpl",  "cmpb",  "cmpl",  "cmpb",  "cmpl",  NULL,    "aas",
/*40*/	"incl",  "incl",  "incl",  "incl",  "incl",  "incl",  "incl",  "incl",
	"decl",  "decl",  "decl",  "decl",  "decl",  "decl",  "decl",  "decl",
/*50*/	"pushl", "pushl", "pushl", "pushl", "pushl", "pushl", "pushl", "pushl",
	"popl",  "popl",  "popl",  "popl",  "popl",  "popl",  "popl",  "pop",
/*60*/	"pushad","popad", "bound", "arpl",  NULL,    NULL,    NULL,    NULL,
	"pushl", "imul",  "pushl", "imul",  "insb",  "insl",  "outsb", "outsl",
/*70*/	"jo",    "jno",   "jc",    "jnc",   "jz",    "jnz",   "jna",   "ja",
	"js",    "jns",   "jp",    "jnp",   "jl",    "jnl",   "jle",   "jnle",
/*80*/	NULL,    NULL,    NULL,    NULL,    "testb", "testl", "xchgb", "xchgl",
	"movb",  "movl",  "movb",  "movl",  "movl",  "lea",   "movl",  "popl",
/*90*/	"nop",   "xchgl", "xchgl", "xchgl", "xchgl", "xchgl", "xchgl", "xchgl",
	"cwde",  "cdq",   "callfl","fwait", "pushfd","popfd", "sahf",  "lahf",
/*a0*/	"movb",  "movl",  "movb",  "movl",  "movsb", "movsd", "cmpsb", "cmpsd",
	"testb", "testl", "stosb", "stosd", "lodsb", "lodsd", "scasb", "scasd",
/*b0*/	"movb",  "movb",  "movb",  "movb",  "movb",  "movb",  "movb",  "movb",  
	"movl",  "movl",  "movl",  "movl",  "movl",  "movl",  "movl",  "movl",  
/*c0*/	NULL,    NULL,    "ret",   "ret",   "les",   "lds",   "movb",  "movl",
	"enter", "leave", "retf",  "retf",  "int3",  "int",   "into",  "iretd",
/*d0*/	NULL,    NULL,    NULL,    NULL,    "aam",   "aad",   "salc",  "xlat",
	"esc0",  "esc1",  "esc2",  "esc3",  "esc4",  "esc5",  "esc6",  "esc7",
/*e0*/	"loopne","loope", "loop",  "jecxz", "inb",   "inl",   "outb",  "outl",
	"call",  "jmp",   "jmpf",  "jmp",   "inb",   "inl",   "outb",  "outl",
/*f0*/	"lock:", "int1",  "repne", "repe",  "hlt",   "cmc",   NULL,    NULL,
	"clc",   "stc",   "cli",   "sti",   "cld",   "std",   NULL,    NULL,
}
};

static const char *opcode_2byte[2][256] = {
/* 16bit */
{
/*00*/	NULL,      NULL,      "lar",     "lsl",
	NULL,      "loadall", "clts",    NULL,
	"invd",    "wbinvd",  NULL,      "UD2",
	NULL,      NULL,      NULL,      NULL,
/*10*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*20*/	"movl",    "movl",    "movl",    "movl",
	"movl",    NULL,      "movl",    NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*30*/	"wrmsr",   "rdtsc",   "rdmsr",   NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*40*/	"cmovo",   "cmovno",  "cmovc",   "cmovnc",
	"cmovz",   "cmovnz",  "cmovna",  "cmova",
	"cmovs",   "cmovns",  "cmovp",   "cmovnp",
	"cmovl",   "cmovnl",  "cmovle",  "cmovnle",
/*50*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*60*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*70*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*80*/	"jo",      "jno",     "jc",      "jnc",
	"jz",      "jnz",     "jna",     "ja",
	"js",      "jns",     "jp",      "jnp",
	"jl",      "jnl",     "jle",     "jnle",
/*90*/	"seto",    "setno",   "setc",    "setnc",
	"setz",    "setnz",   "setna",   "seta",
	"sets",    "setns",   "setp",    "setnp",
	"setl",    "setnl",   "setle",   "setnle",
/*a0*/	"push",    "pop",     "cpuid",   "bt",
	"shldb",   "shldw",   "cmpxchgb","cmpxchgw",
	"push",    "pop",     "rsm",     "bts",
	"shrdb",   "shrdw",   NULL,      "imul",
/*b0*/	"cmpxchgb","cmpxchgw","lss",     "btr",
	"lfs",     "lgs",     "movzb",   "movzw",
	NULL,      "UD2",     NULL,      "btc",
	"bsf",     "bsr",     "movsb",   "movsw",
/*c0*/	"xaddb",   "xaddw",   NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	"bswap",   "bswap",   "bswap",   "bswap",
	"bswap",   "bswap",   "bswap",   "bswap",
/*d0*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*e0*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*f0*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
},
/* 32bit */
{
/*00*/	NULL,      NULL,      "lar",     "lsl",
	NULL,      "loadall", "clts",    NULL,
	"invd",    "wbinvd",  NULL,      "UD2",
	NULL,      NULL,      NULL,      NULL,
/*10*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*20*/	"movl",    "movl",    "movl",    "movl",
	"movl",    NULL,      "movl",    NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*30*/	"wrmsr",   "rdtsc",   "rdmsr",   NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*40*/	"cmovo",   "cmovno",  "cmovc",   "cmovnc",
	"cmovz",   "cmovnz",  "cmovna",  "cmova",
	"cmovs",   "cmovns",  "cmovp",   "cmovnp",
	"cmovl",   "cmovnl",  "cmovle",  "cmovnle",
/*50*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*60*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*70*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*80*/	"jo",      "jno",     "jc",      "jnc",
	"jz",      "jnz",     "jna",     "ja",
	"js",      "jns",     "jp",      "jnp",
	"jl",      "jnl",     "jle",     "jnle",
/*90*/	"seto",    "setno",   "setc",    "setnc",
	"setz",    "setnz",   "setna",   "seta",
	"sets",    "setns",   "setp",    "setnp",
	"setl",    "setnl",   "setle",   "setnle",
/*a0*/	"push",    "pop",     "cpuid",   "bt",
	"shldb",   "shldl",   "cmpxchgb","cmpxchgl",
	"push",    "pop",     "rsm",     "bts",
	"shrdb",   "shrdl",   NULL,      "imul",
/*b0*/	"cmpxchgb","cmpxchgd","lss",     "btr",
	"lfs",     "lgs",     "movzbl",  "movzwl",
	NULL,      "UD2",     NULL,      "btc",
	"bsf",     "bsr",     "movsbl",  "movswl",
/*c0*/	"xaddb",   "xaddl",   NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	"bswapl",  "bswapl",  "bswapl",  "bswapl",
	"bswapl",  "bswapl",  "bswapl",  "bswapl",
/*d0*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*e0*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
/*f0*/	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
	NULL,      NULL,      NULL,      NULL,
}
};

static const char *opcode_0x8x[2][2][8] = {
/* 16bit */
{
	{ "addb", "orb", "adcb", "sbbb", "andb", "subb", "xorb", "cmpb" },
	{ "addw", "orw", "adcw", "sbbw", "andw", "subw", "xorw", "cmpw" }
},
/* 32bit */
{
	{ "addb", "orb", "adcb", "sbbb", "andb", "subb", "xorb", "cmpb" },
	{ "addl", "orl", "adcl", "sbbl", "andl", "subl", "xorl", "cmpl" }
}
};

static const char *opcode_shift[2][2][8] = {
/* 16bit */
{
	{ "rolb", "rorb", "rclb", "rcrb", "shlb", "shrb", "shlb", "sarb" },
	{ "rolw", "rorw", "rclw", "rcrw", "shlw", "shrw", "shlw", "sarw" }
},
/* 32bit */
{
	{ "rolb", "rorb", "rclb", "rcrb", "shlb", "shrb", "shlb", "sarb" },
	{ "roll", "rorl", "rcll", "rcrl", "shll", "shrl", "shll", "sarl" }
},
};

static const char *opcode_0xf6[2][2][8] = {
/* 16bit */
{
	{ "testb", "testb", "notb", "negb", "mul", "imul", "div", "idiv" },
	{ "testw", "testw", "notw", "negw", "mulw", "imulw", "divw", "idivw" }
},
/* 32bit */
{
	{ "testb", "testb", "notb", "negb", "mul", "imul", "div", "idiv" },
	{ "testl", "testl", "notl", "negl", "mull", "imull", "divl", "idivl" }
},
};

static const char *opcode_0xfe[2][2][8] = {
/* 16bit */
{
	{ "incb", "decb", NULL, NULL, NULL, NULL, NULL, NULL },
	{ "incw", "decw", "call", "callf", "jmp", "jmpf", "push", NULL }
},
/* 32bit */
{
	{ "incb", "decb", NULL, NULL, NULL, NULL, NULL, NULL },
	{ "incl", "decl", "call", "callf", "jmp", "jmpf", "pushl", NULL }
}
};

static const char *opcode2_g6[8] = {
	"sldt", "str", "lldt", "ltr", "verr", "verw", NULL, NULL
};

static const char *opcode2_g7[8] = {
	"sgdt", "sidt", "lgdt", "lidt", "smsw", NULL, "lmsw", "invlpg"
};

static const char *opcode2_g8[8] = {
	NULL, NULL, NULL, NULL, "bt", "bts", "btr", "btc"
};

static const char *opcode2_g9[8] = {
	NULL, "cmpxchg8b", NULL, NULL, NULL, NULL, NULL, NULL
};

#if 0
static const char *sep[2] = { " ", ", " };
#endif

/**
 * string copy
 */
static char *
ncpy(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n);
	dest[n - 1] = '\0';
	return dest;
}

/**
 * string copy at
 */
static char *
ncat(char *dest, const char *src, size_t n)
{
	const size_t offset = strlen(dest);
	ncpy(dest + offset, src, n - offset);
	return dest;
}

/*
 * fetch memory
 */
static int
convert_address(disasm_context_t *ctx)
{
	UINT32 pde_addr;	/* page directory entry address */
	UINT32 pde;		/* page directory entry */
	UINT32 pte_addr;	/* page table entry address */
	UINT32 pte;		/* page table entry */
	UINT32 addr;

	if (CPU_STAT_SREG(CPU_CS_INDEX).valid) {
		addr = CPU_STAT_SREGBASE(CPU_CS_INDEX) + ctx->eip;
		if (CPU_STAT_PAGING) {
			pde_addr = CPU_STAT_PDE_BASE + ((addr >> 20) & 0xffc);
			pde = cpu_memoryread_d(pde_addr);
			/* XXX: check */
			pte_addr = (pde & CPU_PDE_BASEADDR_MASK) + ((addr >> 10) & 0xffc);
			pte = cpu_memoryread_d(pte_addr);
			/* XXX: check */
			addr = (pte & CPU_PTE_BASEADDR_MASK) + (addr & 0x00000fff);
		}
		ctx->val = addr;
		return 0;
	}
	return 1;
}

static int
disasm_codefetch_1(disasm_context_t *ctx)
{
	UINT8 val;
	int rv;

	rv = convert_address(ctx);
	if (rv)
		return rv;

	val = cpu_memoryread(ctx->val);
	ctx->val = val;

	ctx->opbyte[ctx->nopbytes++] = (UINT8)ctx->val;
	ctx->eip++;

	return 0;
}

#if 0
static int
disasm_codefetch_2(disasm_context_t *ctx)
{
	UINT16 val;
	int rv;

	rv = disasm_codefetch_1(ctx);
	if (rv)
		return rv;
	val = (UINT16)(ctx->val & 0xff);
	rv = disasm_codefetch_1(ctx);
	if (rv)
		return rv;
	val |= (UINT16)(ctx->val & 0xff) << 8;

	ctx->val = val;
	return 0;
}

static int
disasm_codefetch_4(disasm_context_t *ctx)
{
	UINT32 val;
	int rv;

	rv = disasm_codefetch_1(ctx);
	if (rv)
		return rv;
	val = ctx->val & 0xff;
	rv = disasm_codefetch_1(ctx);
	if (rv)
		return rv;
	val |= (UINT32)(ctx->val & 0xff) << 8;
	rv = disasm_codefetch_1(ctx);
	if (rv)
		return rv;
	val |= (UINT32)(ctx->val & 0xff) << 16;
	rv = disasm_codefetch_1(ctx);
	if (rv)
		return rv;
	val |= (UINT32)(ctx->val & 0xff) << 24;

	ctx->val = val;
	return 0;
}

/*
 * get effective address.
 */

static int
ea16(disasm_context_t *ctx, char *buf, size_t size)
{
	static const char *ea16_str[8] = {
		"bx + si", "bx + di", "bp + si", "bp + di",
		"si", "di", "bp", "bx"
	};
	UINT32 val;
	UINT mod, rm;
	int rv;

	mod = (ctx->modrm >> 6) & 3;
	rm = ctx->modrm & 7;

	if (mod == 0) {
		if (rm == 6) {
			/* disp16 */
			rv = disasm_codefetch_2(ctx);
			if (rv)
				return rv;

			snprintf(buf, size, "[0x%04x]", ctx->val);
		} else {
			snprintf(buf, size, "[%s]", ea16_str[rm]);
		}
	} else {
		if (mod == 1) {
			/* disp8 */
			rv = disasm_codefetch_1(ctx);
			if (rv)
				return rv;

			val = ctx->val;
			if (val & 0x80) {
				val |= 0xff00;
			}
		} else {
			/* disp16 */
			rv = disasm_codefetch_2(ctx);
			if (rv)
				return rv;

			val = ctx->val;
		}
		snprintf(buf, size, "[%s + 0x%04x]", ea16_str[rm], val);
	}

	return 0;
}

static int
ea32(disasm_context_t *ctx, char *buf, size_t size)
{
	char tmp[32];
	UINT count[9];
	UINT32 val;
	UINT mod, rm;
	UINT sib;
	UINT scale;
	UINT idx;
	UINT base;
	int rv;
	int i, n;

	memset(count, 0, sizeof(count));

	mod = (ctx->modrm >> 6) & 3;
	rm = ctx->modrm & 7;

	/* SIB */
	if (rm == 4) {
		rv = disasm_codefetch_1(ctx);
		if (rv)
			return rv;

		sib = ctx->val;
		scale = (sib >> 6) & 3;
		idx = (sib >> 3) & 7;
		base = sib & 7;

		/* base */
		if (mod == 0 && base == 5) {
			/* disp32 */
			rv = disasm_codefetch_4(ctx);
			if (rv)
				return rv;
			count[8] += ctx->val;
		} else {
			count[base]++;
		}

		/* index & scale */
		if (idx != 4) {
			count[idx] += 1 << scale;
		}
	}

	/* MOD/RM */
	if (mod == 0 && rm == 5) {
		/* disp32 */
		rv = disasm_codefetch_4(ctx);
		if (rv)
			return rv;
		count[8] += ctx->val;
	} else {
		/* mod */
		if (mod == 1) {
			/* disp8 */
			rv = disasm_codefetch_1(ctx);
			if (rv)
				return rv;

			val = ctx->val;
			if (val & 0x80) {
				val |= 0xffffff00;
			}
			count[8] += val;
		} else if (mod == 2) {
			/* disp32 */
			rv = disasm_codefetch_4(ctx);
			if (rv)
				return rv;
			count[8] += ctx->val;
		}

		/* rm */
		if (rm != 4) {
			count[rm]++;
		}
	}

	ncpy(buf, "[", size);
	for (n = 0, i = 0; i < 8; i++) {
		if (count[i] != 0) {
			if (n > 0) {
				ncat(buf, " + ", size);
			}
			if (count[i] > 1) {
				snprintf(tmp, size, "%s * %d",
				    reg32_str[i], count[i]);
			} else {
				ncpy(tmp, reg32_str[i], sizeof(tmp));
			}
			ncat(buf, tmp, size);
			n++;
		}
	}
	if (count[8] != 0) {
		if (n > 0) {
			ncat(buf, " + ", size);
		}
		snprintf(tmp, sizeof(tmp), "0x%08x", count[8]);
		ncat(buf, tmp, size);
	}
	ncat(buf, "]", size);

	return 0;
}

static int
ea(disasm_context_t *ctx)
{
	char buf[256];
	char tmp[8];
	size_t len;
	int rv;

	memset(buf, 0, sizeof(buf));

	if (!ctx->as32)
		rv = ea16(ctx, buf, sizeof(buf));
	else
		rv = ea32(ctx, buf, sizeof(buf));
	if (rv)
		return rv;

	if (ctx->narg == 0) {
		ncat(ctx->next, sep[0], ctx->remain);
	} else {
		ncat(ctx->next, sep[1], ctx->remain);
	}
	len = strlen(ctx->next);
	len = (len < ctx->remain) ? len : ctx->remain;
	ctx->next += len;
	ctx->remain -= len;

	ctx->arg[ctx->narg++] = ctx->next;
	if (ctx->useseg) {
		snprintf(tmp, sizeof(tmp), "%s:", sreg_str[ctx->seg]);
		ncat(ctx->next, tmp, ctx->remain);
	}
	ncat(ctx->next, buf, ctx->remain);
	len = strlen(ctx->next);
	len = (len < ctx->remain) ? len : ctx->remain;
	ctx->next += len;
	ctx->remain -= len;

	return 0;
}
#endif

/*
 * get opcode
 */
static int
get_opcode(disasm_context_t *ctx)
{
	const char *opcode;
	UINT8 op[3];
	int prefix;
	size_t len;
	int rv;
	int i;

	for (prefix = 0; prefix < MAX_PREFIX; prefix++) {
		rv = disasm_codefetch_1(ctx);
		if (rv)
			return rv;

		op[0] = (UINT8)(ctx->val & 0xff);
		if (!(insttable_info[op[0]] & INST_PREFIX))
			break;

		if (ctx->prefix == 0)
			ctx->prefix = ctx->next;

		switch (op[0]) {
		case 0x26: 	/* ES: */
		case 0x2e: 	/* CS: */
		case 0x36: 	/* SS: */
		case 0x3e: 	/* DS: */
			ctx->useseg = TRUE;
			ctx->seg = (op[0] >> 3) & 3;
			break;

		case 0x64:	/* FS: */
		case 0x65:	/* GS: */
			ctx->useseg = TRUE;
			ctx->seg = (op[0] - 0x64) + 4;
			break;

		case 0x66:	/* OPSize: */
			ctx->op32 = !CPU_STATSAVE.cpu_inst_default.op_32;
			break;

		case 0x67:	/* AddrSize: */
			ctx->as32 = !CPU_STATSAVE.cpu_inst_default.as_32;
			break;
		}
	}
	if (prefix == MAX_PREFIX)
		return 1;

	if (ctx->prefix) {
		for (i = 0; i < prefix - 1; i++) {
			opcode = opcode_1byte[ctx->op32][ctx->opbyte[i]];
			if (opcode) {
				ncat(ctx->next, opcode, ctx->remain);
				ncat(ctx->next, " ", ctx->remain);
			}
		}
		len = strlen(ctx->next);
		len = (len < ctx->remain) ? len : ctx->remain;
		ctx->next += len;
		ctx->remain -= len;
	}

	ctx->opcode[0] = op[0];
	opcode = opcode_1byte[ctx->op32][op[0]];
	if (opcode == NULL) {
		rv = disasm_codefetch_1(ctx);
		if (rv)
			return rv;

		op[1] = (UINT8)(ctx->val & 0xff);
		ctx->opcode[1] = op[1];

		switch (op[0]) {
		case 0x0f:
			opcode = opcode_2byte[ctx->op32][op[1]];
			if (opcode == NULL) {
				rv = disasm_codefetch_1(ctx);
				if (rv)
					return rv;

				op[2] = (UINT8)(ctx->val & 0xff);
				ctx->opcode[2] = op[2];

				switch (op[1]) {
				case 0x00:
					opcode = opcode2_g6[(op[2]>>3)&7];
					ctx->modrm = op[2];
					break;

				case 0x01:
					opcode = opcode2_g7[(op[2]>>3)&7];
					ctx->modrm = op[2];
					break;

				case 0xba:
					opcode = opcode2_g8[(op[2]>>3)&7];
					ctx->modrm = op[2];
					break;

				case 0xc7:
					opcode = opcode2_g9[(op[2]>>3)&7];
					ctx->modrm = op[2];
					break;
				}
			}
			break;

		case 0x80: case 0x81: case 0x82: case 0x83:
			opcode = opcode_0x8x[ctx->op32][op[0]&1][(op[1]>>3)&7];
			ctx->modrm = op[1];
			break;

		case 0xc0: case 0xc1:
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
			opcode = opcode_shift[ctx->op32][op[0]&1][(op[1]>>3)&7];
			ctx->modrm = op[1];
			break;

		case 0xf6: case 0xf7:
			opcode = opcode_0xf6[ctx->op32][op[0]&1][(op[1]>>3)&7];
			ctx->modrm = op[1];
			break;

		case 0xfe: case 0xff:
			opcode = opcode_0xfe[ctx->op32][op[0]&1][(op[1]>>3)&7];
			ctx->modrm = op[1];
			break;
		}
	}
	if (opcode == NULL)
		return 1;

	ncat(ctx->next, opcode, ctx->remain);

	return 0;
}

/*
 * interface
 */
int
disasm(UINT32 *eip, disasm_context_t *ctx)
{
	int rv;

	memset(ctx, 0, sizeof(disasm_context_t));
	ctx->remain = sizeof(ctx->str) - 1;
	ctx->next = ctx->str;
	ctx->prefix = 0;
	ctx->op = 0;
	ctx->arg[0] = 0;
	ctx->arg[1] = 0;
	ctx->arg[2] = 0;

	ctx->eip = *eip;
	ctx->op32 = CPU_STATSAVE.cpu_inst_default.op_32;
	ctx->as32 = CPU_STATSAVE.cpu_inst_default.as_32;
	ctx->seg = -1;

	ctx->baseaddr = ctx->eip;
	ctx->pad = ' ';

	rv = get_opcode(ctx);
	if (rv) {
		memset(ctx, 0, sizeof(disasm_context_t));
		return rv;
	}
	*eip = ctx->eip;

	return 0;
}

char *
cpu_disasm2str(UINT32 eip)
{
	static char output[2048];
	disasm_context_t d;
	UINT32 eip2 = eip;
	int rv;

	output[0] = '\0';
	rv = disasm(&eip2, &d);
	if (rv == 0) {
		char buf[256];
		char tmp[32];
		int len = d.nopbytes > 8 ? 8 : d.nopbytes;
		int i;

		buf[0] = '\0';
		for (i = 0; i < len; i++) {
			snprintf(tmp, sizeof(tmp), "%02x ", d.opbyte[i]);
			ncat(buf, tmp, sizeof(buf));
		}
		for (; i < 8; i++) {
			ncat(buf, "   ", sizeof(buf));
		}
		snprintf(output, sizeof(output), "%04x:%08x: %s%s",
		    CPU_CS, eip, buf, d.str);

		if (i < d.nopbytes) {
			char t[256];
			buf[0] = '\0';
			for (; i < d.nopbytes; i++) {
				snprintf(tmp, sizeof(tmp), "%02x ",
				    d.opbyte[i]);
				ncat(buf, tmp, sizeof(buf));
				if ((i % 8) == 7) {
					snprintf(t, sizeof(t),
					    "\n             : %s", buf);
					ncat(output, t, sizeof(output));
					buf[0] = '\0';
				}
			}
			if ((i % 8) != 0) {
				snprintf(t, sizeof(t),
				    "\n             : %s", buf);
				ncat(output, t, sizeof(output));
			}
		}
	}
	return output;
}
