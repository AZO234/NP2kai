
#define	GET_PREFIX_DS											\
				__asm {	mov		edi, DS_FIX					}

#define	GET_PREFIX_SS											\
				__asm {	mov		edi, SS_FIX					}


// --- ea, reg8

									// dest: I286_REG[eax] src: dl
#define	PREPART_EA_REG8(regclk)									\
				__asm { movzx	eax, bh						}	\
				__asm {	mov		ebp, eax					}	\
				__asm {	shr		ebp, 3						}	\
				__asm {	bt		ebp, 2						}	\
				__asm {	adc		ebp, ebp					}	\
				__asm {	and		ebp, 7						}	\
				__asm {	cmp		al, 0c0h					}	\
				__asm {	jc		memory_eareg8				}	\
				__asm {	bt		eax, 2						}	\
				__asm { adc		eax, eax					}	\
				__asm {	and		eax, 7						}	\
				__asm {	mov		dl, I286_REG[ebp]			}	\
				I286CLOCK(regclk)								\

									// dest: MAIN_MEM[ecx] src: dl
#define	MEMORY_EA_REG8(memclk)									\
				__asm {	align	16							}	\
			memory_eareg8:										\
				I286CLOCK(memclk)								\
				__asm {	call	p_ea_dst[eax*4]				}	\
				__asm {	cmp		ecx, I286_MEMWRITEMAX		}	\
				__asm {	jae		extmem_eareg8				}	\
				__asm {	mov		dl, I286_REG[ebp]			}

									// dest: al,ecx src: I286_REG[ebx]
#define EXTMEM_EA_REG8											\
				__asm {	align	16							}	\
			extmem_eareg8:										\
				__asm {	call	i286_memoryread				}



// --- ea, reg16
									// dest: I286_REG[eax*2] src: dx
#define	PREPART_EA_REG16(regclk)								\
				__asm {	movzx	eax, bh						}	\
				__asm {	mov		ebp, eax					}	\
				__asm {	shr		ebp, 3-1					}	\
				__asm {	and		ebp, 7*2					}	\
				__asm {	cmp		al, 0c0h					}	\
				__asm {	jc		memory_eareg16				}	\
				__asm {	and		eax, 7						}	\
				__asm {	mov		dx, I286_REG[ebp]			}	\
				I286CLOCK(regclk)

									// dest: MAIN_MEM[ecx] src: dx
#define	MEMORY_EA_REG16(memclk)									\
				__asm {	align	16							}	\
			memory_eareg16:										\
				I286CLOCK(memclk)								\
				__asm {	call	p_ea_dst[eax*4]				}	\
				__asm {	cmp		ecx, (I286_MEMWRITEMAX-1)	}	\
				__asm {	jae		extmem_eareg16				}	\
				__asm {	mov		dx, I286_REG[ebp]			}

									// dest: ax,ecx src: I286_REG[ebp]
#define EXTMEM_EA_REG16											\
				__asm {	align	16							}	\
			extmem_eareg16:										\
				__asm {	call	i286_memoryread_w			}



// --- reg8, ea
									// dest: I286_REG[ebp]  src: al
#define	PREPART_REG8_EA(regclk, memclk)							\
				__asm {	movzx	eax, bh						}	\
				__asm {	mov		ebp, eax					}	\
				__asm {	shr		ebp, 3						}	\
				__asm {	bt		ebp, 2						}	\
				__asm {	adc		ebp, ebp					}	\
				__asm {	and		ebp, 7						}	\
				__asm {	cmp		al, 0c0h					}	\
				__asm {	jnc		src_register				}	\
			I286CLOCK(memclk)									\
				__asm {	call	p_ea_dst[eax*4]				}	\
				__asm {	call	i286_memoryread				}	\
				__asm {	jmp		short reg8_ea_ready			}	\
				__asm {	align	16							}	\
			src_register:										\
				__asm {	bt		eax, 2						}	\
				__asm {	adc		eax, eax					}	\
				__asm {	and		eax, 7						}	\
				__asm {	mov		edi, eax					}	\
				GET_NEXTPRE2									\
				__asm {	mov		al, I286_REG[edi]			}	\
				I286CLOCK(regclk)								\
			reg8_ea_ready:



// --- reg16, ea

									// dest: I286_REG[ebp]  src: ax
#define	PREPART_REG16_EA(regclk, memclk)						\
				__asm {	movzx	eax, bh						}	\
				__asm {	mov		ebp, eax					}	\
				__asm {	shr		ebp, 3-1					}	\
				__asm {	and		ebp, 7*2					}	\
				__asm {	cmp		al, 0c0h					}	\
				__asm {	jnc		src_register				}	\
				I286CLOCK(memclk)								\
				__asm {	call	p_ea_dst[eax*4]				}	\
				__asm {	call	i286_memoryread_w			}	\
				__asm {	jmp		reg16_ea_ready				}	\
				__asm {	align	16							}	\
			src_register:										\
				__asm {	and		eax, 7						}	\
				__asm {	mov		edi, eax					}	\
				GET_NEXTPRE2									\
				__asm {	mov		ax, I286_REG[edi*2]			}	\
				I286CLOCK(regclk)								\
			reg16_ea_ready:


// f6,f7,fe,ff
									// dest: I286_REG[eax]
#define	PREPART_EA8(regclk)										\
				__asm {	cmp		al, 0c0h					}	\
				__asm {	jc		memory_eareg8				}	\
				I286CLOCK(regclk)								\
				__asm {	bt		eax, 2						}	\
				__asm { adc		eax, eax					}	\
				__asm {	and		eax, 7						}

									// dest: MAIN_MEM[ecx]
#define	MEMORY_EA8(memclk)										\
				__asm {	align	16							}	\
			memory_eareg8:										\
				I286CLOCK(memclk)								\
				__asm {	call	p_ea_dst[eax*4]				}	\
				__asm {	cmp		ecx, I286_MEMWRITEMAX		}	\
				__asm {	jae		extmem_eareg8				}

									// dest: ecx
#define EXTMEM_EA8												\
				__asm {	align	16							}	\
			extmem_eareg8:										\
				__asm {	call	i286_memoryread				}


									// dest: I286_REG[eax*2]
#define	PREPART_EA16(regclk)									\
				__asm {	cmp		al, 0c0h					}	\
				__asm {	jc		memory_eareg16				}	\
				I286CLOCK(regclk)								\
				__asm {	and		eax, 7						}

									// dest: MAIN_MEM[ecx]
#define	MEMORY_EA16(memclk)										\
				__asm {	align	16							}	\
			memory_eareg16:										\
				I286CLOCK(memclk)								\
				__asm {	call	p_ea_dst[eax*4]				}	\
				__asm {	cmp		ecx, (I286_MEMWRITEMAX-1)	}	\
				__asm {	jae		extmem_eareg16				}

									// dest: ecx
#define EXTMEM_EA16												\
				__asm {	align	16							}	\
			extmem_eareg16:										\
				__asm {	call	i286_memoryread_w			}
