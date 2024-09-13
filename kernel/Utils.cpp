
#include "Utils.h"
#include "video.h"
#include "cmosAlarm.h"
#include "cmosPeriodTimer.h"
#include "acpi.h"
#include "hardware.h"


int unicode2asc(short* unicode, int unicodelen, char* asc) {
	int i = 0;
	int j = 0;
	for (; i < unicodelen; )
	{
		asc[i] = (char)unicode[j];
		j++;
		i++;
	}

	*(asc + i) = 0;
	return unicodelen;
}

int asc2unicode(char* asc, int asclen, short* unicode) {
	int i = 0;
	int j = 0;
	for (; i < asclen; )
	{
		unicode[j] = asc[i];
		j++;
		i++;
	}

	*(WORD*)(unicode + j) = 0;
	return asclen;
}

int __memset(char* dst, int value, int len) {
#ifdef _DEBUG
	return 0;
#else
	if (dst == 0) {
		return 0;
	}
	for (int i = 0; i < len; i++) {
		dst[i] = value;
	}
	return len;
#endif
}

int __memcpy(char* dst, char* src, int len) {
	if (dst == 0 || src == 0) {
		return 0;
	}
	for (int i = 0; i < len; i++) {
		dst[i] = src[i];
	}
	return len;
}

int __wmemcpy(wchar_t* dst, wchar_t* src, int len) {
	if (dst == 0 || src == 0) {
		return 0;
	}
	for (int i = 0; i < len; i++) {
		dst[i] = src[i];
	}
	return len;
}

int __wmemcmp(wchar_t* src, wchar_t* dst, int len) {
	if (dst == 0 || src == 0) {
		return 0;
	}
	for (int i = 0; i < len; i++) {
		int v = src[i] - dst[i];
		if (v) {
			return v;
		}
		else {
			continue;
		}
	}

	return 0;
}


int __memcmp(char* src, char* dst, int len) {
	if (dst == 0 || src == 0) {
		return 0;
	}
	for (int i = 0; i < len; i++) {
		int v = src[i] - dst[i];
		if (v) {
			return v;
		}
		else {
			continue;
		}
	}

	return 0;
}

int __strlen(char* s) {
	int len = 0;
	while (s && *s) {
		s++;
		len++;
	}
	return len;
}

int __wcslen(wchar_t* s) {
	int len = 0;
	while (s && *s )
	{
		len++;
		s++;
	}
	return len;
}

int __strcpy(char* dst, char* src) {
	if (dst == 0 || src == 0) {
		return 0;
	}
	int len = __strlen(src);
	__memcpy(dst, src, len);
	dst[len] = 0;
	return  len;
}

int __wcscpy(wchar_t* dst, wchar_t* src) {
	if (dst == 0 || src == 0) {
		return 0;
	}
	int len = __wcslen(src);
	__wmemcpy(dst, src, len );
	dst[len] = 0;
	return len ;
}

int __strcat(char* src, char* dst) {
	int srclen = __strlen(src);
	int dstlen = __strlen(dst);
	__memcpy(src + srclen, dst, dstlen);
	*(src + srclen + dstlen) = 0;
	return srclen + dstlen;
}


int __wcscat(wchar_t* src, wchar_t* dst) {
	int srclen = __wcslen(src);
	int dstlen = __wcslen(dst);
	__wmemcpy( (src + srclen), dst, dstlen);
	*(src + srclen + dstlen) = 0;
	return srclen + dstlen;
}

int __strncpy(char* dst, char* src, int limit) {
	if (limit <= 0)
	{
		return FALSE;
	}

	int l = __strlen(src);
	if (l >= limit)
	{
		l = limit;
		__memcpy(dst, src, l);
		*(dst + l) = 0;
		return l;
	}
	else {
		return __strcpy(dst, src);
	}
}


char* __strstr(char * src, char * dst) {

	int dstlen = __strlen(dst);
	int srclen = __strlen(src);
	if (dstlen > srclen || dstlen == 0 || srclen == 0)
	{
		return 0;
	}
	
	for (int i = 0; i < srclen - dstlen + 1 ;i++) {

		if (__memcmp(src + i,dst ,dstlen) == 0) {
			return src + i;
		}
	}

	return 0;
}


wchar_t* __wcsstr(wchar_t* src, wchar_t* dst) {

	int dstlen = __wcslen(dst);
	int srclen = __wcslen(src);
	if (dstlen > srclen || dstlen == 0 || srclen == 0)
	{
		return 0;
	}

	for (int i = 0; i < srclen - dstlen + 1; i++) {

		if (__wmemcmp(src + i, dst, dstlen) == 0) {
			return src + i;
		}
	}

	return 0;
}


int __strcmp(char * src, char * dst) {
	int srclen = __strlen(src);
	int dstlen = __strlen(dst);
	if (srclen != dstlen)
	{
		return -1;
	}

	for (int i = 0; i < srclen; i++) {
		int v = src[i] - dst[i];
		if (v) {
			return v;
		}
		else {
			continue;
		}
	}

	return 0;
}

int __wcscmp(wchar_t* src, wchar_t* dst) {
	int srclen = __wcslen(src);
	int dstlen = __wcslen(dst);
	if (srclen != dstlen)
	{
		return -1;
	}

	for (int i = 0; i < srclen; i++) {
		int v = src[i] - dst[i];
		if (v) {
			return v;
		}
		else {
			continue;
		}
	}

	return 0;
}


int __substrLen(char * src, int pos, int len,char * dst) {
	__memcpy(dst, src + pos,len);
	*(dst + len) = 0;
	return len;
}

int __substr(char * src, int pos,char * dst) {
	return __strcpy(dst, src + pos);
}

int __dump(char * src,int len,int lowercase, unsigned char * dstbuf) {
	if (len >= 0x1000)
	{
		*dstbuf = 0;
		return FALSE;
	}

	int no = 55;
	if (lowercase)
	{
		no = 87;
	}

	
	int lineno = 0;
	unsigned char *dst = dstbuf;

	char szlineno[16];
	__memset(szlineno, 0x20, 16);
	int lnl =__sprintf(szlineno, "%d.", lineno);
	//*(szlineno + lnl) = 0x20;
	//__strcpy((char*)dst, szlineno);
	//dst += lnl;
	for (int k = 0;k < 16; k ++)
	{
		if (szlineno[k] == 0)
		{
			szlineno[k] = 0x20;
		}
	}
	__memcpy((char*)dst, szlineno, 8);
	dst += 8;
	

	for (int i = 0; i < len; i++)
	{
		if ((i != 0) && (i % 16 == 0))
		{
			*dst = '\n';
			dst++;
			lineno++;

			__memset(szlineno, 0x20, 16);
			lnl = __sprintf(szlineno, "%d.", lineno);
			//*(szlineno + lnl) = 0x20;
			//__strcpy((char*)dst, szlineno);
			//dst += lnl;
			for (int k = 0; k < 16; k++)
			{
				if (szlineno[k] == 0)
				{
					szlineno[k] = 0x20;
				}
			}
			__memcpy((char*)dst, szlineno, 8);
			dst += 8;
		}
		else if ((i != 0) && (i % 8 == 0))
		{
			*dst = ' ';
			dst++;
			*dst = ' ';
			dst++;
		}

		unsigned char c = src[i] ;
		unsigned char h = (c & 0x0f0) >> 4;
		unsigned char l = c & 0x0f;
		if (h >= 0 && h <= 9)
		{
			h += 48;
		}
		else if (h >= 10 && h <= 15)
		{
			h += no;
		}

		*dst = h;
		dst++;

		if (l >= 0 && l <= 9)
		{
			l += 48;
		}
		else if (l >= 10 && l <= 15)
		{
			l += no;
		}

		*dst = l;
		dst++;

		*dst = ' ';
		dst++;
	}

	*dst = '\n';
	dst++;

	*dst = 0;
	dst++;

	return dst - dstbuf;
}


int __i2strh(unsigned int n,int lowercase,unsigned char * buf) {
	buf[0] = 0x30;
	buf[1] = 'X';

	int no = 55;
	if (lowercase)
	{
		no = 87;
		buf[1] = 'x';
	}

	int b = 24;

	int tag = 0;

	unsigned char* dst = buf + 2;

	for (int i = 0; i < 4; i ++)
	{
		unsigned char c = n >> b;

		unsigned char h = (c & 0x0f0) >> 4;
		unsigned char l = c & 0x0f;

		unsigned char tmp = h;

		if (h >= 0 && h <= 9)
		{
			h += 48;
		}else if (h >= 10 && h <= 15)
		{
			h += no;
		}

		if (tag) {
			*dst = h;
			dst++;
		}
		else {
			if (tmp) {
				tag = TRUE;
				*dst = h;
				dst++;
			}
			else {

			}
		}

		tmp = l;
		if (l >= 0 && l <= 9)
		{
			l += 48;
		}
		else if (l >= 10 && l <= 15)
		{
			l += no;
		}

		if (tag) {
			*dst = l;
			dst++;
		}
		else {
			if (tmp) {
				tag = TRUE;
				*dst = l;
				dst++;
			}
			else {

			}
		}

		b -= 8;
	}

	*(dst) = 0;

	if (dst - buf == 2) {
		dst[2] = 0x30;
		dst[3] = 0;
		dst++;
	}
	return dst - buf;
}

int __i2strd(unsigned int h, char * strd) {

	__memset(strd, 0, 11);

	unsigned int divid = 1000000000;

	int flag = FALSE;

	int cnt = 0;

	for (int i = 0; i < 10; i++)
	{

		unsigned int d = h / divid;

		if (d)
		{
			*strd = d + 0x30;

			strd++;

			cnt++;

			h = h % divid;;

			flag = TRUE;
		}
		else if (flag) {
			*strd = 0x30;
			strd++;
			cnt++;
		}

		divid = divid / 10;
	}

	if (cnt == 0)
	{
		*strd = 0x30;
		return 1;
	}
	return cnt;
}

int __strh2i(unsigned char * str) {
	int ret = 0;

	int len = __strlen((char*)str);
	for (int i = 0; i < len ; i++)
	{
		ret = ret << 4;

		unsigned char c = str[i];
		if (c >= '0' && c <= '9')
		{
			c = c - 48;
		}else if (c >= 'A' && c <= 'F')
		{
			c = c - 55;
		}else if (c >= 'a' && c <= 'f')
		{
			c = c - 87;
		}
		else {
			return 0;
		}
		
		ret += c;
	}

	return ret;
}


int __strd2i(char * istr) {
	int len = __strlen(istr);
	if (len <= 0)
	{
		return 0;
	}

	int negtive = 0;
	int k = 0;
	if (istr[0] == '-')
	{
		negtive = 1;
		k++;
	}else if (istr[0] == '+')
	{
		k++;
	}

	int ret = 0;
	for (; k < len; k ++)
	{
		int v = istr[k] - 0x30;
		if (v >= 0 && v <= 9)
		{
			ret = ret * 10 + v;
		}
		else {
			break;
		}
	}

	if (negtive)
	{
		ret = -ret;
	}
	return ret;
}

int __printf(char* buf, char* format, ...) {

		if (format == 0 || buf == 0) {
			return FALSE;
		}

		int formatLen = __strlen(format);
		if (formatLen == 0) {
			return FALSE;
		}

		DWORD* params = 0;
		int param_cnt = 0;
		__asm {
			lea eax, format
			add eax, 4
			mov params, eax
		}

		char* dst = buf;
		int spos = 0;
		int dpos = 0;
		char numstr[64];
		int len = 0;
		for (spos = 0; spos < formatLen; ) {

			if (format[spos] == '%' && format[spos + 1] == 'd') {
				spos += 2;
				DWORD num = *params;
				params++;

				len = __i2strd(num, numstr);
				__memcpy(dst + dpos, numstr, len);
				dpos += len;
			}
			else if (format[spos] == '%' && format[spos + 1] == 'x') {

				DWORD num = *params;
				params++;

				len = __i2strh(num, 1, (unsigned char*)numstr);
				__memcpy(dst + dpos, numstr, len);

				spos += 2;
				dpos += len;
			}
			else if (format[spos] == '%' && format[spos + 1] == 'u') {

				spos += 2;
				DWORD num = *params;
				params++;

				len = __i2strd(num, numstr);
				__memcpy(dst + dpos, numstr, len);
				dpos += len;
			}
			else if (format[spos] == '%' && format[spos + 1] == 's') {
				char* str = (char*)*params;
				params++;
				int tmpstrlen = __strlen(str);

				__strcpy(dst + dpos, str);
				dpos += tmpstrlen;
				spos += 2;
			}
			else if (format[spos] == '%' && format[spos + 1] == 'X') {
				DWORD num = *params;
				params++;

				len = __i2strh(num, 0, (unsigned char*)numstr);
				__memcpy(dst + dpos, numstr, len);

				spos += 2;
				dpos += len;
			}
			else if (format[spos] == '%' && __memcmp(format + spos + 1, "i64d", 4) == 0) {
				spos += 5;

				DWORD numl = *params;
				params++;
				DWORD numh = *params;
				params++;

				len = __i2strd(numh, numstr);
				__memcpy(dst + dpos, numstr, len);
				dpos += len;
				len = __i2strd(numl, numstr);
				__memcpy(dst + dpos, numstr, len);
				dpos += len;
			}
			else if (format[spos] == '%' && __memcmp(format + spos + 1, "i64x", 4) == 0) {
				spos += 5;

				DWORD numl = *params;
				params++;
				DWORD numh = *params;
				params++;

				len = __i2strh(numh, 1, (unsigned char*)numstr);
				__memcpy(dst + dpos, numstr, len);
				dpos += len;
				len = __i2strh(numl, 1, (unsigned char*)numstr);
				__memcpy(dst + dpos, numstr, len);
				dpos += len;
			}
			else if (format[spos] == '%' && format[spos + 1] == 'S') {
				wchar_t* wstr = (wchar_t*)*params;
				params++;
				int tmpstrlen = 2 * __wcslen(wstr);
				spos += 2;
				__wcscpy((wchar_t*)dst + dpos, (wchar_t*)wstr);
				dpos += tmpstrlen;
			}
			else {
				dst[dpos] = format[spos];
				dpos++;
				spos++;
			}
		}
		dst[dpos] = 0;
		dst[dpos + 1] = 0;

	if (g_ScreenMode) {
		int showlen = __drawGraphChars((unsigned char*)buf, 0);
	}
	return dpos;
}


int __sprintf(char* buf, char* format, ...) {

	if (format == 0 || buf == 0) {
		return FALSE;
	}

	int formatLen = __strlen(format);
	if (formatLen == 0) {
		return FALSE;
	}

	DWORD* params = 0;
	int param_cnt = 0;
	__asm {
		lea eax,format
		add eax,4
		mov params,eax
	}

	char* dst = buf;
	int spos = 0;
	int dpos = 0;
	char numstr[64];
	int len = 0;
	for (spos = 0; spos < formatLen; ) {

		if (format[spos] == '%' && format[spos + 1] == 'd') {
			spos+=2;
			DWORD num = *params;
			params++;

			len = __i2strd(num, numstr);
			__memcpy(dst + dpos, numstr, len);
			dpos += len;
		}
		else if (format[spos] == '%' && format[spos + 1] == 'x') {

			DWORD num = *params;
			params++;

			len = __i2strh(num,1, (unsigned char*)numstr);
			__memcpy(dst + dpos, numstr, len);

			spos += 2;
			dpos += len;
		}
		else if (format[spos] == '%' && format[spos + 1] == 'u') {

			spos += 2;
			DWORD num = *params;
			params++;

			len = __i2strd(num, numstr);
			__memcpy(dst + dpos, numstr, len);
			dpos += len;
		}
		else if (format[spos] == '%' && format[spos + 1] == 's') {
			char * str = (char*)*params;
			params++;
			int tmpstrlen = __strlen(str);
			
			__strcpy(dst + dpos, str);
			dpos += tmpstrlen;
			spos += 2;
		}
		else if (format[spos] == '%' && format[spos + 1] == 'X') {
			DWORD num = *params;
			params++;

			len = __i2strh(num, 0, (unsigned char*)numstr);
			__memcpy(dst + dpos, numstr, len);

			spos += 2;
			dpos += len;
		}
		else if (format[spos] == '%' && __memcmp(format + spos + 1,"i64d",4) == 0 ) {
			spos += 5;

			DWORD numl = *params;
			params++;
			DWORD numh = *params;
			params++;

			len = __i2strd(numh, numstr);
			__memcpy(dst + dpos, numstr, len);
			dpos += len;
			len = __i2strd(numl, numstr);
			__memcpy(dst + dpos, numstr, len);
			dpos += len;
		}
		else if (format[spos] == '%' && __memcmp(format + spos + 1, "i64x", 4) == 0) {
			spos += 5;

			DWORD numl = *params;
			params++;
			DWORD numh = *params;
			params++;

			len = __i2strh(numh,1, (unsigned char*)numstr);
			__memcpy(dst + dpos, numstr, len);
			dpos += len;
			len = __i2strh(numl,1, (unsigned char*)numstr);
			__memcpy(dst + dpos, numstr, len);
			dpos += len;
		}
		else if (format[spos] == '%' && format[spos + 1] == 'S') {
			wchar_t* wstr = (wchar_t*)*params;
			params++;
			int tmpstrlen = 2*__wcslen(wstr);
			spos += 2;
			__wcscpy((wchar_t*)dst + dpos, (wchar_t*)wstr);
			dpos += tmpstrlen;
		}
		else {
			dst[dpos] = format[spos];
			dpos++;
			spos++;
		}
	}
	dst[dpos] = 0;
	dst[dpos+1] = 0;

	return dpos;
}

int __strlwr(char * str) {
	int len = __strlen(str);
	for (int i = 0; i < len; i++)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] += 0x20;
		}
	}
	return len;
}

int __strupr(char * str) {
	int len = __strlen(str);
	for (int i = 0; i < len; i++)
	{
		if (str[i] >= 'a' && str[i] <= 'z')
		{
			str[i] -= 0x20;
		}
	}
	return len;
}


int lower2upper(char *data,int len) {
	int k = 0;
	for (int i = 0;i < len; i ++)
	{
		if (data[i] >= 'a' && data[i] <= 'z')
		{
			data[i] -= 0x20;
			k++;
		}
	}

	return k;
}

int upper2lower(char *data, int len) {
	int k = 0;
	for (int i = 0; i < len; i++)
	{
		if (data[i] >= 'A' && data[i] <= 'Z')
		{
			data[i] += 0x20;
			k++;
		}
	}

	return k;
}


//bswap oprd1,oprd1:reg
DWORD __ntohl(DWORD v) {

	DWORD result = v;

	__asm {
		mov eax, result
		bswap eax
		mov result,eax
	}

	return result;
}

WORD __ntohs(WORD v) {

	WORD result = 0;

	__asm {
		mov ax, v
		xchg ah,al
		mov result, ax
	}

	return result;
}





//xadd oprd1,oprd2,�Ƚ��������������ٽ�����֮���͸���һ����,oprd1:mem or reg,oprd2:reg
DWORD __lockInc(DWORD *v) {
	DWORD old = 0;
	__asm {
		mov eax, 1
		lock xadd [v],eax
		mov old,eax
	}
	return old;
}


void __initSpinlock(DWORD * v) {
	*v = 0;
}


/*
��ʽ �� bts dword ptr [ecx],0
[ecx] ָ����ڴ�ĵ�0λ��ֵ�� CF λ �� ���ҽ�[ecx]�ĵ�0λ��Ϊ1
*/
DWORD __enterSpinlock(DWORD * v) {
	__asm {
		__enterSpinLockLoop:
		lock bts[v], 0
		jnc __getSpinLock
		pause
		jmp __enterSpinLockLoop
		__getSpinLock :
	}
}


DWORD __leaveSpinlock(DWORD * v) {
	__asm {
		lock btr[v], 0
		jnc __leaveSpinLockError
		ret
		__leaveSpinLockError :
	}
	char szout[1024];
	//__printf(szout,"__leaveSpinLock errpr\r\n");
}


extern "C"  __declspec(dllexport) int __spinlockEntry(void* lockv) {
	__asm {
		__spinlock_xchg:
		mov eax, 1
		lock xchg[lockv], eax
		cmp eax, 0
		jnz __spinlock_xchg
	}
	return TRUE;
}


extern "C"  __declspec(dllexport) int __spinlockLeave(void* lockv) {
	DWORD result = 0;
	__asm {
		mov eax, 0
		lock xchg[lockv], eax
		mov[result], eax
	}
	return result;
}

//cmpxchg oprd1,oprd2  oprd1:mem or reg,oprd2:reg
//CMPXCHG r/m,r
//CMPXCHG r/m, r 
// ���ۼ���AL/AX/EAX/RAX�е�ֵ���ײ�������Ŀ�Ĳ��������Ƚ�
// �����ȣ���2��������Դ����������ֵװ�ص��ײ�������zf��1��������ȣ� �ײ�������ֵװ�ص�AL/AX/EAX/RAX����zf��0
DWORD __enterLock(DWORD * lockvalue) {
	DWORD result = 0;

	__asm {
		__waitZeroValue:
		mov eax, 0
		mov edx, 1
		lock cmpxchg[lockvalue], edx
		jz __entryFree
		mov result,eax
		nop
		pause
		jmp __waitZeroValue
		__entryFree :
	}
	
	return result;
}


DWORD __leaveLock(DWORD * lockvalue) {
	DWORD result = 0;

	__asm {
		__leavelockLoop:
		mov eax, 1
		mov edx, 0
		lock cmpxchg[lockvalue], edx
		jz _over
		mov result,eax
		pause	
		jmp __leavelockLoop
		_over :
	}
	
	return result;
}








int getCpuType(char* name) {

	__asm {
		mov edi, name
		mov eax, 9
		int 80h
	}

	return 0;
}

int getCpuInfo(char* name) {

	__asm {
		mov edi, name
		mov eax, 12
		int 80h
	}

	return 0;
}




int __shutdownSystem() {

	__asm {
		mov ax, 2001h;
		mov dx, 1004h;
		out dx, ax;    //д�� 2001h  ���˿� 1004h    ʵ�ֱ����ػ�
	}

	doPowerOff();

	outportw(0x4004, 0x3400);

	for (int bdf = 0x80000008; bdf <= 0x80fff808; bdf += 0x100)			//offset 8,read class type,vender type
	{
		outportd(0xcf8, bdf);
		DWORD v = inportd(0xcfc);
		if (v && v != 0xffffffff)
		{
			int r = 4 + (v & 0xfffe);
			int d = inportw(r) | 0x3c00;
			outportw(r, d);

			int r2 = 30 + (v & 0xfffe);
			int d2 = inportw(r2) & 0xffef;
			outportw(r2, d2);
		}
	}
	return 0;
}

//puѰַλ�ڵ�һλ��ʼffff:0000,��Ѱַλ�ڵ�һλ��ʱ��0��
//���⵽��ǰ��ַ0040:0072λ�Ƿ�Ϊ1234h,�����1234hʱ���Ͳ���Ҫ����ڴ棬�������1234h������Ҫ����ڴ棬�ͻ�����
int __reset() {

	doReboot();

#if 0
	outportb(0x92, 0x01);
	outportb(0x64, 0xFE);
	outportb(0xcf9, 0x04);
	outportb(0xcf9, 0x06);
#endif
	__asm {

		mov al, 4
		mov dx, 0cf9h
		out dx, al

		mov al, 1
		out 92h, al
	}
}



int __sleep(int millisecs) {
	__asm {
		mov eax, 6
		lea edi, millisecs
		int 80h
	}
}
