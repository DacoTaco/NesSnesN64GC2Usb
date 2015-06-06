void tetteuh(char tet,tepel)
{
#define RELEASE_DATA	"cp %4,1\n" "rjmp .+2\n" "cbi %0,4\n" "rjmp .+1\n" "cbi %0,5\n"
#define GCN64_DATA_DDR	DDRC
	if(tet == 1)
	{
		asm volatile(
			"nop			\n"
			);
	}
	else
	{
		asm volatile(
			"nop			\n"
			"nop			\n"
			"cpse %0,1\n" 
			"rjmp .+2\n" 
			"cbi 0x18,4\n" 
			"rjmp .+1\n" 
			"cbi 0x18,5\n"
			:
			:
				"r" (tet)							// %4
			);
	}
}
/*
//modded code for the ID
		//"	sbic %2, 5				\n" //first pin b/s
		"	cp %5,0				\n" 
		"	brne id_not_0_1%=		\n" 
		"	sbic %2, 4				\n"
		"	rjmp initial_wait_low	\n"
		"	rjmp waithigh			\n"
"id_not_0_1%=:\n"	
		"	sbic %2, 5				\n"
		//end*/