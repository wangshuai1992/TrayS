//////////////////////////////////////////////////////////////////////////////// 进程网速排行（悬停窗口“上传 / 下载”页）
//数据来自 ETW 的 Microsoft-Windows-Kernel-Network 收发事件（和资源监视器同源），需要管理员权限。
//每收发一次就有一条事件，网速越高越费 CPU，所以只在悬停窗口开着、并且切到上传 / 下载页时运行。
//走本机代理的流量（程序 → 回环 → 代理 → 外网）：代理连外网的流量按各程序和它之间的回环流量比例分回给程序，代理只留剩下的；
//回环那头的监听方自己没有外网流量（进程间通信）时不算网速。
//TUN 代理（加载了 wintun.dll 或是常见代理核心）：程序经虚拟网卡的流量已经记在程序上，从代理身上扣掉。
#include <evntrace.h>
#include <evntcons.h>
#ifndef EVENT_TRACE_USE_MS_FLUSH_TIMER
#define EVENT_TRACE_USE_MS_FLUSH_TIMER 0x00000010//FlushTimer 按毫秒（Win8 起）
#endif

#define NET_ROWS 5
struct NETTOP { DWORD pid; ULONG64 rate; WCHAR szExe[36]; };
NETTOP netTop[2][NET_ROWS];//[0] 上传 [1] 下载，字节/秒，从高到低
int nNetTop[2];
int netState = 0;//0 没运行 1 刚开始还没数据 2 有数据 -1 没有管理员权限 -2 其它错误（netErr）
ULONG netErr = 0;
BOOL ProcNameByPid(DWORD pid, WCHAR* sz);//TrayS.cpp，用进程快照查名字

#define NET_SESSION L"TrayS-NetTop"
static const GUID NET_PROV = { 0x7dd42a49, 0x5329, 0x4832, { 0x8d, 0xfd, 0x43, 0xd9, 0x79, 0x15, 0x3a, 0x88 } };
#define NETP_N 512
#define NETL_N 256
#define NETLSN_N 1024
struct NETP { DWORD pid; ULONG64 up, down; };//进程收发（不含回环）
struct NETL { DWORD cli, srv; ULONG64 up, down; };//回环：客户端 cli 发给 / 收自监听方 srv
struct NETWIN { NETP p[NETP_N]; NETL l[NETL_N]; ULONG64 tunUp, tunDown; };
struct NETW { DWORD pid; double up, down; };
typedef DWORD(WINAPI* pfnGetExtendedTcpTable)(PVOID, PDWORD, BOOL, ULONG, TCP_TABLE_CLASS, ULONG);
struct NETCTX
{
	CRITICAL_SECTION cs;//事件回调和数据线程之间
	BOOL csInit;
	TRACEHANDLE session, trace;
	HANDLE thread;
	NETWIN* win[2];//回调往 win[cur] 里加，数据线程每秒换一次
	int cur;
	WORD lsnPort[2][NETLSN_N];//本机监听端口（升序）和进程，双缓冲
	DWORD lsnPid[2][NETLSN_N];
	int nLsn[2], lsnCur;
	DWORD tun4[2][8];//虚拟网卡（wintun 等）的地址，双缓冲
	BYTE tun6[2][8][16];
	int nTun4[2], nTun6[2], tunCur;
	ULONGLONG tStart, tLast;
	int tick;
	DWORD tunPid[64];//进程是不是 TUN 代理（缓存）
	BYTE tunIs[64];
	int nTunPid;
	NETW w[NETP_N];//本窗口各进程的流量（分配后）
	int nw;
	pfnGetExtendedTcpTable getTcp;
	pfnGetAdaptersAddresses getAdapters;
	BYTE* buf;
	DWORD nBuf;
} net;
struct NETPROPS { EVENT_TRACE_PROPERTIES p; WCHAR name[32]; };
void NetProps(NETPROPS* x, BOOL msFlush)
{
	memset(x, 0, sizeof(*x));
	x->p.Wnode.BufferSize = sizeof(*x);
	x->p.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	x->p.Wnode.ClientContext = 1;//QPC 时间戳
	x->p.LogFileMode = EVENT_TRACE_REAL_TIME_MODE | (msFlush ? EVENT_TRACE_USE_MS_FLUSH_TIMER : 0);
	x->p.FlushTimer = msFlush ? 250 : 1;//250ms 交一次，每秒统计时不会忽多忽少
	x->p.BufferSize = 64;//KB
	x->p.MinimumBuffers = 4;
	x->p.MaximumBuffers = 32;
	x->p.LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
}
//上次异常退出时会话还留着（内核还在往里记事件），启动时停掉
void NetTopCleanup()
{
	NETPROPS x;
	NetProps(&x, FALSE);
	ControlTraceW(0, NET_SESSION, &x.p, EVENT_TRACE_CONTROL_STOP);
}
inline WORD NetPort(DWORD p) { return (WORD)(((p & 0xFF) << 8) | ((p >> 8) & 0xFF)); }//网络字节序
inline DWORD NetHash(DWORD a) { return (a * 2654435761u) >> 7; }
NETP* NetPGet(NETWIN* w, DWORD pid)
{
	DWORD i = NetHash(pid) & (NETP_N - 1);
	for (int k = 0; k < NETP_N; k++, i = (i + 1) & (NETP_N - 1))
	{
		if (w->p[i].pid == pid)
			return &w->p[i];
		if (w->p[i].pid == 0)
		{
			w->p[i].pid = pid;
			return &w->p[i];
		}
	}
	return NULL;
}
NETL* NetLGet(NETWIN* w, DWORD cli, DWORD srv)
{
	DWORD i = NetHash(cli * 31 + srv) & (NETL_N - 1);
	for (int k = 0; k < NETL_N; k++, i = (i + 1) & (NETL_N - 1))
	{
		if (w->l[i].cli == cli && w->l[i].srv == srv)
			return &w->l[i];
		if (w->l[i].cli == 0)
		{
			w->l[i].cli = cli;
			w->l[i].srv = srv;
			return &w->l[i];
		}
	}
	return NULL;
}
DWORD NetLsn(WORD port)//监听这个端口的进程，没有返回 0
{
	const WORD* a = net.lsnPort[net.lsnCur];
	int lo = 0, hi = net.nLsn[net.lsnCur] - 1;
	while (lo <= hi)
	{
		int m = (lo + hi) / 2;
		if (a[m] == port)
			return net.lsnPid[net.lsnCur][m];
		if (a[m] < port)
			lo = m + 1;
		else
			hi = m - 1;
	}
	return 0;
}
BOOL NetIsTunAddr(const BYTE* a, BOOL v6)
{
	int t = net.tunCur;
	if (!v6)
	{
		for (int i = 0; i < net.nTun4[t]; i++)
			if (net.tun4[t][i] == *(const DWORD UNALIGNED*)a)
				return TRUE;
		return FALSE;
	}
	for (int i = 0; i < net.nTun6[t]; i++)
	{
		int k = 0;
		while (k < 16 && net.tun6[t][i][k] == a[k])
			k++;
		if (k == 16)
			return TRUE;
	}
	return FALSE;
}
//事件回调（消费线程）。字段：PID、size、daddr（对端）、saddr（本机）、dport、sport；IPv6 地址各 16 字节
void WINAPI NetOnEvent(PEVENT_RECORD r)
{
	if (r->EventHeader.ProviderId.Data1 != 0x7dd42a49)
		return;
	BOOL v6, send, tcp;
	switch (r->EventHeader.EventDescriptor.Id)
	{
	case 10: v6 = FALSE; send = TRUE; tcp = TRUE; break;
	case 11: v6 = FALSE; send = FALSE; tcp = TRUE; break;
	case 26: v6 = TRUE; send = TRUE; tcp = TRUE; break;
	case 27: v6 = TRUE; send = FALSE; tcp = TRUE; break;
	case 42: v6 = FALSE; send = TRUE; tcp = FALSE; break;
	case 43: v6 = FALSE; send = FALSE; tcp = FALSE; break;
	case 58: v6 = TRUE; send = TRUE; tcp = FALSE; break;
	case 59: v6 = TRUE; send = FALSE; tcp = FALSE; break;
	default: return;
	}
	const BYTE* d = (const BYTE*)r->UserData;
	if (!d || r->UserDataLength < (v6 ? 44 : 20))
		return;
	DWORD pid = *(const DWORD UNALIGNED*)d, size = *(const DWORD UNALIGNED*)(d + 4);
	if (pid == 0 || size == 0)
		return;
	const BYTE* da = d + 8;
	const BYTE* sa = v6 ? d + 24 : d + 12;
	const BYTE* pp = v6 ? d + 40 : d + 16;
	WORD dport = (WORD)((pp[0] << 8) | pp[1]), sport = (WORD)((pp[2] << 8) | pp[3]);
	BOOL lo;
	if (v6)
	{
		int k = 0, z = 0;
		while (k < 16 && da[k] == sa[k])
			k++;
		while (z < 15 && da[z] == 0)
			z++;
		lo = k == 16 || (z == 15 && da[15] == 1);//和本机地址相同，或 ::1
	}
	else
		lo = da[0] == 127 || *(const DWORD UNALIGNED*)da == *(const DWORD UNALIGNED*)sa;
	EnterCriticalSection(&net.cs);
	NETWIN* w = net.win[net.cur];
	if (w)
	{
		if (lo)
		{
			if (tcp && NetLsn(sport) != pid)//监听方那头不记，只记客户端
			{
				DWORD srv = NetLsn(dport);
				if (srv && srv != pid)
				{
					NETL* l = NetLGet(w, pid, srv);
					if (l)
					{
						if (send)
							l->up += size;
						else
							l->down += size;
					}
				}
			}
		}
		else
		{
			NETP* p = NetPGet(w, pid);
			if (p)
			{
				if (send)
					p->up += size;
				else
					p->down += size;
			}
			if (NetIsTunAddr(sa, v6))
			{
				if (send)
					w->tunUp += size;
				else
					w->tunDown += size;
			}
		}
	}
	LeaveCriticalSection(&net.cs);
}
DWORD WINAPI NetThread(LPVOID)
{
	TRACEHANDLE h = net.trace;
	ProcessTrace(&h, 1, NULL, NULL);//会话停掉后返回
	CloseTrace(h);
	return 0;
}
//本机监听的 TCP 端口 → 进程（用来分清回环连接哪头是代理）
void NetRefreshLsn()
{
	if (!net.getTcp)
		return;
	int t = net.lsnCur ^ 1, n = 0;
	WORD* port = net.lsnPort[t];
	DWORD* pid = net.lsnPid[t];
	for (int f = 0; f < 2; f++)
	{
		ULONG af = f ? AF_INET6 : AF_INET;
		DWORD sz = net.nBuf;
		DWORD e = net.buf ? net.getTcp(net.buf, &sz, FALSE, af, TCP_TABLE_OWNER_PID_LISTENER, 0) : ERROR_INSUFFICIENT_BUFFER;
		if (e == ERROR_INSUFFICIENT_BUFFER)
		{
			if (net.buf)
				HeapFree(GetProcessHeap(), 0, net.buf);
			net.nBuf = sz + 8192;
			net.buf = (BYTE*)HeapAlloc(GetProcessHeap(), 0, net.nBuf);
			if (!net.buf)
			{
				net.nBuf = 0;
				return;
			}
			sz = net.nBuf;
			e = net.getTcp(net.buf, &sz, FALSE, af, TCP_TABLE_OWNER_PID_LISTENER, 0);
		}
		if (e != NO_ERROR)
			continue;
		DWORD cnt = *(DWORD*)net.buf;
		for (DWORD i = 0; i < cnt && n < NETLSN_N; i++)
		{
			WORD p;
			DWORD id;
			if (f == 0)
			{
				MIB_TCPROW_OWNER_PID* r = &((MIB_TCPTABLE_OWNER_PID*)net.buf)->table[i];
				p = NetPort(r->dwLocalPort);
				id = r->dwOwningPid;
			}
			else
			{
				MIB_TCP6ROW_OWNER_PID* r = &((MIB_TCP6TABLE_OWNER_PID*)net.buf)->table[i];
				p = NetPort(r->dwLocalPort);
				id = r->dwOwningPid;
			}
			int j = n++;
			while (j > 0 && port[j - 1] > p)
			{
				port[j] = port[j - 1];
				pid[j] = pid[j - 1];
				j--;
			}
			port[j] = p;
			pid[j] = id;
		}
	}
	net.nLsn[t] = n;
	EnterCriticalSection(&net.cs);
	net.lsnCur = t;
	LeaveCriticalSection(&net.cs);
}
//虚拟网卡（IF_TYPE_PROP_VIRTUAL，wintun / WireGuard 都是）的地址
void NetRefreshTun()
{
	if (!net.getAdapters)
		return;
	ULONG sz = 16384;
	BYTE* b = NULL;
	ULONG e = ERROR_BUFFER_OVERFLOW;
	for (int k = 0; k < 3 && e == ERROR_BUFFER_OVERFLOW; k++)
	{
		if (b)
			HeapFree(GetProcessHeap(), 0, b);
		b = (BYTE*)HeapAlloc(GetProcessHeap(), 0, sz);
		if (!b)
			return;
		e = net.getAdapters(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, (PIP_ADAPTER_ADDRESSES)b, &sz);
	}
	int t = net.tunCur ^ 1, n4 = 0, n6 = 0;
	if (e == NO_ERROR)
		for (PIP_ADAPTER_ADDRESSES a = (PIP_ADAPTER_ADDRESSES)b; a; a = a->Next)
		{
			if (a->IfType != IF_TYPE_PROP_VIRTUAL || a->OperStatus != IfOperStatusUp)
				continue;
			for (PIP_ADAPTER_UNICAST_ADDRESS u = a->FirstUnicastAddress; u; u = u->Next)
			{
				if (u->Address.lpSockaddr->sa_family == AF_INET && n4 < 8)
					net.tun4[t][n4++] = ((SOCKADDR_IN*)u->Address.lpSockaddr)->sin_addr.S_un.S_addr;
				else if (u->Address.lpSockaddr->sa_family == AF_INET6 && n6 < 8)
				{
					const BYTE* s = ((SOCKADDR_IN6*)u->Address.lpSockaddr)->sin6_addr.u.Byte;
					for (int k = 0; k < 16; k++)
						net.tun6[t][n6][k] = s[k];
					n6++;
				}
			}
		}
	HeapFree(GetProcessHeap(), 0, b);
	net.nTun4[t] = n4;
	net.nTun6[t] = n6;
	EnterCriticalSection(&net.cs);
	net.tunCur = t;
	LeaveCriticalSection(&net.cs);
}
//是不是 TUN 代理：常见代理核心的名字，或加载了 wintun.dll（有的核心从内存加载 wintun，看不到模块，所以还要靠名字）
BOOL NetIsTunPid(DWORD pid)
{
	for (int i = 0; i < net.nTunPid; i++)
		if (net.tunPid[i] == pid)
			return net.tunIs[i];
	BOOL is = FALSE;
	WCHAR nm[36];
	if (ProcNameByPid(pid, nm))
	{
		const WCHAR* known[] = { L"sing-box", L"xray", L"v2ray", L"mihomo", L"clash", L"verge-mihomo", L"tun2socks", L"hysteria", L"tuic", L"naive", L"nekobox", L"nekoray", L"easytier" };
		for (int k = 0; k < (int)(sizeof(known) / sizeof(known[0])) && !is; k++)
		{
			int n = lstrlen(known[k]);
			is = lstrlen(nm) >= n && CompareStringOrdinal(nm, n, known[k], n, TRUE) == CSTR_EQUAL;
		}
	}
	if (!is)
	{
		HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if (s != INVALID_HANDLE_VALUE)
		{
			MODULEENTRY32W me;
			me.dwSize = sizeof(me);
			for (BOOL ok = Module32FirstW(s, &me); ok && !is; ok = Module32NextW(s, &me))
				is = lstrcmpi(me.szModule, L"wintun.dll") == 0;
			CloseHandle(s);
		}
	}
	int i = net.nTunPid < 64 ? net.nTunPid++ : (int)(pid % 64);
	net.tunPid[i] = pid;
	net.tunIs[i] = (BYTE)is;
	return is;
}
NETW* NetWFind(DWORD pid, BOOL add)
{
	for (int i = 0; i < net.nw; i++)
		if (net.w[i].pid == pid)
			return &net.w[i];
	if (!add || net.nw >= NETP_N)
		return NULL;
	NETW* x = &net.w[net.nw++];
	x->pid = pid;
	x->up = x->down = 0;
	return x;
}
void NetStopSession()
{
	NETPROPS x;
	NetProps(&x, FALSE);
	ControlTraceW(net.session, NULL, &x.p, EVENT_TRACE_CONTROL_STOP);
	net.session = 0;
}
BOOL NetTopStartLocked()
{
	if (net.session)
		return TRUE;
	if (!net.csInit)
	{
		InitializeCriticalSection(&net.cs);
		net.csInit = TRUE;
	}
	if (!net.getTcp)
	{
		HMODULE h = LoadLibrary(L"iphlpapi.dll");
		if (h)
		{
			net.getTcp = (pfnGetExtendedTcpTable)GetProcAddress(h, "GetExtendedTcpTable");
			net.getAdapters = (pfnGetAdaptersAddresses)GetProcAddress(h, "GetAdaptersAddresses");
		}
	}
	for (int k = 0; k < 2; k++)
	{
		if (!net.win[k])
			net.win[k] = (NETWIN*)HeapAlloc(GetProcessHeap(), 0, sizeof(NETWIN));
		if (!net.win[k])
		{
			netState = -2;
			netErr = ERROR_NOT_ENOUGH_MEMORY;
			return FALSE;
		}
		memset(net.win[k], 0, sizeof(NETWIN));
	}
	net.cur = 0;
	net.tick = 0;
	net.nTunPid = 0;
	NetRefreshLsn();
	NetRefreshTun();
	NETPROPS x;
	NetProps(&x, TRUE);
	ULONG e = StartTraceW(&net.session, NET_SESSION, &x.p);
	if (e == ERROR_ALREADY_EXISTS)
	{
		NetTopCleanup();
		NetProps(&x, TRUE);
		e = StartTraceW(&net.session, NET_SESSION, &x.p);
	}
	if (e == ERROR_INVALID_PARAMETER)//Win7 不支持毫秒刷新
	{
		NetProps(&x, FALSE);
		e = StartTraceW(&net.session, NET_SESSION, &x.p);
	}
	if (e == ERROR_SUCCESS)
	{
		//只要收发数据的 8 种事件，其它（如“代为拷贝数据”）在内核里就丢掉（Win8.1 起支持，不支持就全收、回调里过滤）
		USHORT ids[10] = { 1, 8, 10, 11, 26, 27, 42, 43, 58, 59 };//FilterIn=1、Count=8、事件号
		EVENT_FILTER_DESCRIPTOR fd;
		fd.Ptr = (ULONGLONG)(ULONG_PTR)ids;
		fd.Size = sizeof(ids);
		fd.Type = EVENT_FILTER_TYPE_EVENT_ID;
		ENABLE_TRACE_PARAMETERS ep;
		memset(&ep, 0, sizeof(ep));
		ep.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
		ep.EnableFilterDesc = &fd;
		ep.FilterDescCount = 1;
		e = EnableTraceEx2(net.session, &NET_PROV, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0x30, 0, 0, &ep);//0x30：IPv4 | IPv6
		if (e != ERROR_SUCCESS)
			e = EnableTraceEx2(net.session, &NET_PROV, EVENT_CONTROL_CODE_ENABLE_PROVIDER, TRACE_LEVEL_VERBOSE, 0x30, 0, 0, NULL);
		if (e == ERROR_SUCCESS)
		{
			EVENT_TRACE_LOGFILEW lf;
			memset(&lf, 0, sizeof(lf));
			lf.LoggerName = (LPWSTR)NET_SESSION;
			lf.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
			lf.EventRecordCallback = NetOnEvent;
			net.trace = OpenTraceW(&lf);
			if (net.trace == INVALID_PROCESSTRACE_HANDLE)
				e = GetLastError() ? GetLastError() : ERROR_INVALID_HANDLE;
			else
			{
				net.thread = CreateThread(NULL, 0, NetThread, NULL, 0, NULL);
				if (!net.thread)
				{
					e = GetLastError();
					CloseTrace(net.trace);
				}
			}
		}
		if (e != ERROR_SUCCESS)
			NetStopSession();
	}
	if (e != ERROR_SUCCESS)
	{
		net.session = 0;
		netErr = e;
		netState = e == ERROR_ACCESS_DENIED ? -1 : -2;
		return FALSE;
	}
	net.tStart = net.tLast = GetTickCount64();
	nNetTop[0] = nNetTop[1] = 0;
	netState = 1;
	return TRUE;
}
//切到上传 / 下载页、或开着这两页时打开悬停窗口（界面线程）
BOOL NetTopStart()
{
	EnterCriticalSection(&csProcess);
	BOOL ok = NetTopStartLocked();
	LeaveCriticalSection(&csProcess);
	return ok;
}
//切走、或悬停窗口关掉（界面线程）
void NetTopStop()
{
	EnterCriticalSection(&csProcess);
	if (net.session)
	{
		NetStopSession();
		BOOL done = TRUE;
		if (net.thread)
		{
			done = WaitForSingleObject(net.thread, 2000) == WAIT_OBJECT_0;
			CloseHandle(net.thread);
			net.thread = NULL;
		}
		if (done)//消费线程退出了，回调不会再用缓冲区
			for (int k = 0; k < 2; k++)
			{
				HeapFree(GetProcessHeap(), 0, net.win[k]);
				net.win[k] = NULL;
			}
		if (net.buf)
		{
			HeapFree(GetProcessHeap(), 0, net.buf);
			net.buf = NULL;
			net.nBuf = 0;
		}
	}
	if (netState > 0)
		netState = 0;
	nNetTop[0] = nNetTop[1] = 0;
	LeaveCriticalSection(&csProcess);
}
//数据线程每秒一次（已在 csProcess 里）：换窗口，分配代理流量，排出前五
void NetTopTick()
{
	if (!net.session || !net.win[0] || !net.win[1])
		return;
	ULONGLONG now = GetTickCount64();
	if (now - net.tLast < 300)
		return;
	NetRefreshLsn();
	if (++net.tick % 5 == 0)
		NetRefreshTun();
	EnterCriticalSection(&net.cs);
	NETWIN* w = net.win[net.cur];
	net.cur ^= 1;
	LeaveCriticalSection(&net.cs);
	double ms = (double)(now - net.tLast);
	net.tLast = now;
	net.nw = 0;
	for (int i = 0; i < NETP_N; i++)
		if (w->p[i].pid)
		{
			NETW* x = &net.w[net.nw++];
			x->pid = w->p[i].pid;
			x->up = (double)w->p[i].up;
			x->down = (double)w->p[i].down;
		}
	//代理：外网流量按回环流量比例分给客户端；代理串联（如 sing-box → xray）时多轮往里传
	DWORD srvs[64];
	int ns = 0;
	for (int i = 0; i < NETL_N; i++)
		if (w->l[i].cli)
		{
			int k = 0;
			while (k < ns && srvs[k] != w->l[i].srv)
				k++;
			if (k == ns && ns < 64)
				srvs[ns++] = w->l[i].srv;
		}
	for (int pass = 0; pass < 3; pass++)
		for (int s = 0; s < ns; s++)
		{
			NETW* S = NetWFind(srvs[s], FALSE);
			if (!S || (S->up <= 0 && S->down <= 0))
				continue;
			double su = 0, sd = 0;
			for (int i = 0; i < NETL_N; i++)
				if (w->l[i].cli && w->l[i].srv == srvs[s])
				{
					su += (double)w->l[i].up;
					sd += (double)w->l[i].down;
				}
			double du = S->up < su ? S->up : su, dd = S->down < sd ? S->down : sd;
			if (du <= 0 && dd <= 0)
				continue;
			for (int i = 0; i < NETL_N; i++)
				if (w->l[i].cli && w->l[i].srv == srvs[s])
				{
					NETW* C = NetWFind(w->l[i].cli, TRUE);
					if (!C)
						continue;
					double gu = su > 0 ? du * (double)w->l[i].up / su : 0, gd = sd > 0 ? dd * (double)w->l[i].down / sd : 0;
					C->up += gu;
					C->down += gd;
					w->l[i].up -= (ULONG64)gu;//这部分回环流量已经分过了
					w->l[i].down -= (ULONG64)gd;
				}
			S->up -= du;
			S->down -= dd;
		}
	//TUN：程序经虚拟网卡的流量已经记在程序上，代理身上扣掉（只查流量够大的，免得每秒查很多进程的模块）
	double tu = (double)w->tunUp, td = (double)w->tunDown;
	if (tu + td > 0)
		for (int i = 0; i < net.nw; i++)
		{
			NETW* x = &net.w[i];
			if (x->up + x->down >= (tu + td) / 4 && NetIsTunPid(x->pid))
			{
				x->up = x->up > tu ? x->up - tu : 0;
				x->down = x->down > td ? x->down - td : 0;
			}
		}
	memset(w, 0, sizeof(NETWIN));
	if (inTipsProcessX)//鼠标在进程行上，列表不动
		return;
	for (int t = 0; t < 2; t++)
	{
		int idx[NET_ROWS], n = 0;
		double rate[NET_ROWS];
		for (int i = 0; i < net.nw; i++)
		{
			double r = (t ? net.w[i].down : net.w[i].up) * 1000 / ms;
			if (r < 1)
				continue;
			int j = n < NET_ROWS ? n++ : NET_ROWS;
			if (j == NET_ROWS && r <= rate[NET_ROWS - 1])
				continue;
			if (j == NET_ROWS)
				j--;
			while (j > 0 && rate[j - 1] < r)
			{
				rate[j] = rate[j - 1];
				idx[j] = idx[j - 1];
				j--;
			}
			rate[j] = r;
			idx[j] = i;
		}
		for (int k = 0; k < n; k++)
		{
			netTop[t][k].pid = net.w[idx[k]].pid;
			netTop[t][k].rate = (ULONG64)rate[k];
			if (!ProcNameByPid(netTop[t][k].pid, netTop[t][k].szExe))
				wsprintf(netTop[t][k].szExe, L"PID %d", netTop[t][k].pid);
		}
		nNetTop[t] = n;
	}
	if (now - net.tStart >= 1200)
		netState = 2;
}
