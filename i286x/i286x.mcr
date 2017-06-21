#define	I286CLOCK(clock)												\
				__asm {	sub		I286_REMCLOCK, clock				}
#define	I286CLOCKDEC													\
				__asm {	inc		I286_REMCLOCK						}

#define	FLAG_LOAD														\
				__asm {	mov		ah, I286_FLAGL						}	\
				__asm {	sahf										}

#define	CFLAG_LOAD														\
				__asm {	bt		I286_FLAG, 0						}

#define	FLAG_STORE														\
				__asm {	lahf										}	\
				__asm {	mov		I286_FLAGL, ah						}

#define	FLAG_STORE0														\
				__asm {	lahf										}	\
				__asm {	mov		I286_FLAGL, ah						}	\
				__asm {	and		I286_FLAG, not O_FLAG				}

#define	FLAG_STORE_OF													\
				__asm {	lahf										}	\
				__asm {	mov		I286_FLAGL, ah						}	\
				__asm {	seto	ah									}	\
				__asm {	shl		ah, 3								}	\
				__asm {	and		I286_FLAG, not O_FLAG				}	\
				__asm {	or		I286_FLAGH, ah						}

#define	FLAG_STORE_NC													\
				__asm {	clc											}	\
				__asm {	lahf										}	\
				__asm {	seto	al									}	\
				__asm {	shl		al, 3								}	\
				__asm {	and		I286_FLAG, 0f701h					}	\
				__asm {	or		I286_FLAGL, ah						}	\
				__asm {	or		I286_FLAGH, al						}

#define	FLAG_STORE_OC													\
				__asm {	seto	ah									}	\
				__asm {	setc	al									}	\
				__asm {	shl		ah, 3								}	\
				__asm {	and		I286_FLAG, not (O_FLAG or C_FLAG)	}	\
				__asm {	or		I286_FLAG, ax						}


// 毎回ストールが起きるので si加算を後に・・・
// フェッチする場所は異なってしまうが…
#define	GET_NEXTPRE1													\
				__asm {	lea		ecx, [esi + 4]						}	\
				__asm {	inc		si									}	\
				__asm {	add		ecx, CS_BASE						}	\
				__asm {	call	i286_memoryread						}	\
				__asm {	shrd	ebx, eax, 8							}	\

#define	GET_NEXTPRE2													\
				__asm {	lea		ecx, [esi + 4]						}	\
				__asm {	add		si, 2								}	\
				__asm {	add		ecx, CS_BASE						}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	shrd	ebx, eax, 16						}

#define	GET_NEXTPRE3													\
				__asm {	lea		ecx, [esi + 4]						}	\
				__asm {	add		si, 3								}	\
				__asm {	add		ecx, CS_BASE						}	\
				__asm {	call	i286_memoryread						}	\
				__asm {	shrd	ebx, eax, 8							}	\
				__asm {	inc		ecx									}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	shrd	ebx, eax, 16						}

#define	GET_NEXTPRE3a													\
				__asm {	lea		ecx, [esi + 4]						}	\
				__asm {	add		ecx, CS_BASE						}	\
				__asm {	call	i286_memoryread						}	\
				__asm {	shrd	ebx, eax, 8							}

#define	GET_NEXTPRE3b													\
				__asm {	lea		ecx, [esi + 4 + 1]					}	\
				__asm {	add		si, 3								}	\
				__asm {	add		ecx, CS_BASE						}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	shrd	ebx, eax, 16						}

#define	GET_NEXTPRE4													\
				__asm {	add		si, 4								}	\
				__asm {	mov		ecx, esi							}	\
				__asm {	add		ecx, CS_BASE						}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	shl		eax, 16								}	\
				__asm {	mov		ebx, eax							}	\
				__asm {	inc		ecx									}	\
				__asm {	inc		ecx									}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	shrd	ebx, eax, 16						}

#define	RESET_XPREFETCH													\
				__asm {	mov		ecx, esi							}	\
				__asm {	add		ecx, CS_BASE						}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	shl		eax, 16								}	\
				__asm {	mov		ebx, eax							}	\
				__asm {	inc		ecx									}	\
				__asm {	inc		ecx									}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	shrd	ebx, eax, 16						}

#define	REGPUSH(reg)													\
				__asm {	mov		dx, reg								}	\
				__asm {	sub		I286_SP, 2							}	\
				__asm {	movzx	ecx, I286_SP						}	\
				__asm {	add		ecx, SS_BASE						}	\
				__asm {	call	i286_memorywrite_w					}

#define	REGPUSH1(reg)													\
				__asm {	mov		dx, reg								}	\
				__asm {	sub		I286_SP, 2							}	\
				__asm {	movzx	ecx, I286_SP						}	\
				__asm {	add		ecx, SS_BASE						}	\
				__asm {	jmp		i286_memorywrite_w					}

#define	REGPOP(reg)														\
				__asm {	movzx	ecx, I286_SP						}	\
				__asm {	add		ecx, SS_BASE						}	\
				__asm {	call	i286_memoryread_w					}	\
				__asm {	mov		reg, ax								}	\
				__asm {	add		I286_SP, 2							}

#define INT_NUM(vect)													\
				__asm {	movzx	ebx, I286_SP						}	\
				__asm {	sub		bx, 2								}	\
				__asm {	mov		edi, SS_BASE						}	\
				__asm {	lea		ecx, [edi + ebx]					}	\
				__asm {	sub		bx, 2								}	\
				__asm { mov		dx, I286_FLAG						}	\
				__asm {	and		I286_FLAG, not (T_FLAG or I_FLAG)	}	\
				__asm { mov		I286_TRAP, 0						}	\
				__asm { call	i286_memorywrite_w					}	\
				__asm {	lea		ecx, [edi + ebx]					}	\
				__asm {	sub		bx, 2								}	\
				__asm {	mov		dx, I286_CS							}	\
				__asm {	call	i286_memorywrite_w					}	\
				__asm {	mov		I286_SP, bx							}	\
				__asm {	lea		ecx, [edi + ebx]					}	\
				__asm {	mov		dx, si								}	\
				__asm {	mov		eax, dword ptr I286_MEM[vect*4]		}	\
				__asm {	mov		si, ax								}	\
				__asm {	shr		eax, 16								}	\
				__asm {	mov		I286_CS, ax							}	\
				__asm {	shl		eax, 4								}	\
				__asm {	mov		CS_BASE, eax						}	\
				__asm {	call	i286_memorywrite_w					}	\
				RESET_XPREFETCH											\
				__asm {	ret											}

#define	STRING_DIR														\
				__asm {	xor		eax, eax							}	\
				__asm {	test	I286_FLAG, D_FLAG					}	\
				__asm {	setz	al									}	\
				__asm {	add		eax, eax							}	\
				__asm {	dec		eax									}

#define	STRING_DIRx2													\
				__asm {	xor		eax, eax							}	\
				__asm {	test	I286_FLAG, D_FLAG					}	\
				__asm {	setz	al									}	\
				__asm {	shl		eax, 2								}	\
				__asm {	sub		eax, 2								}
