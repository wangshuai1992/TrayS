// Fluent.h —— 方案 A（Fluent 原生）界面：主题色、Direct2D 绘制工具、悬停窗口
// 只被 TrayS.cpp 包含（TrayS.h 里定义了全局变量，不能被多个编译单元包含）
#pragma once
#include <d2d1.h>
#include <dwrite.h>

#define WM_FLY_DATA (WM_USER + 17)//数据线程每秒通知悬停窗口重绘
void FBarHoverUpdate();//状态条悬停高亮（在文件末尾）

//////////////////////////////////////////////////////////////////////////////// 主题
BOOL fDark = FALSE;//深色（跟随“Windows 模式”，任务栏和它弹出的浮层用这个）
DWORD fAccentL = 0x005FB8, fAccentD = 0x60CDFF;//浅色模式用 Dark1，深色模式用 Light2，和 Win11 控件一致
void FluentLoadTheme()
{
	fDark = GetSystemUsesLightTheme() != 1;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		BYTE p[32];
		DWORD cb = sizeof(p), type;
		if (RegQueryValueEx(hKey, L"AccentPalette", NULL, &type, p, &cb) == ERROR_SUCCESS && cb >= 32)
		{
			fAccentD = (p[4] << 16) | (p[5] << 8) | p[6];//Light2
			fAccentL = (p[16] << 16) | (p[17] << 8) | p[18];//Dark1
		}
		RegCloseKey(hKey);
	}
}
inline DWORD FAccent() { return fDark ? fAccentD : fAccentL; }
inline DWORD FT1() { return fDark ? 0xFFFFFF : 0x1A1A1A; }//正文
inline DWORD FT2() { return fDark ? 0xC5C5C5 : 0x5D5D5D; }//次要
inline DWORD FT3() { return fDark ? 0x9A9A9A : 0x8B8B8B; }//辅助
inline DWORD FDown() { return fDark ? 0x3987E5 : 0x2A78D6; }//下载（与上传一起过了色盲区分校验）
inline DWORD FUp() { return fDark ? 0xD95926 : 0xEB6834; }//上传
#define F_WARN 0xFAB219
#define F_CRIT 0xD03B3B
inline DWORD FWarnText() { return fDark ? 0xFCE100 : 0x9D5D00; }
inline DWORD FCritText() { return fDark ? 0xFF99A4 : 0xC42B1C; }
inline D2D1_COLOR_F FC(DWORD rgb, float a = 1.f)
{
	D2D1_COLOR_F c = { ((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f, a };
	return c;
}

//////////////////////////////////////////////////////////////////////////////// Direct2D / DirectWrite
ID2D1Factory* fD2D = NULL;
IDWriteFactory* fDW = NULL;
WCHAR fUiFont[32] = L"Segoe UI";
WCHAR fIconFont[32] = L"Segoe MDL2 Assets";
BOOL FluentInit()
{
	if (fD2D && fDW)
		return TRUE;
	HMODULE h1 = LoadLibrary(L"d2d1.dll"), h2 = LoadLibrary(L"dwrite.dll");
	if (!h1 || !h2)
		return FALSE;
	typedef HRESULT(WINAPI* PFN_D2D)(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS*, void**);
	typedef HRESULT(WINAPI* PFN_DW)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);
	PFN_D2D pD2D = (PFN_D2D)GetProcAddress(h1, "D2D1CreateFactory");
	PFN_DW pDW = (PFN_DW)GetProcAddress(h2, "DWriteCreateFactory");
	if (!pD2D || !pDW || FAILED(pD2D(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), NULL, (void**)&fD2D)))
		return FALSE;
	if (FAILED(pDW(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&fDW)))
		return FALSE;
	IDWriteFontCollection* fc = NULL;
	if (SUCCEEDED(fDW->GetSystemFontCollection(&fc, FALSE)))
	{
		UINT32 idx;
		BOOL ex = FALSE;
		if (SUCCEEDED(fc->FindFamilyName(L"Segoe UI Variable Text", &idx, &ex)) && ex)
			lstrcpy(fUiFont, L"Segoe UI Variable Text");
		ex = FALSE;
		if (SUCCEEDED(fc->FindFamilyName(L"Segoe Fluent Icons", &idx, &ex)) && ex)
			lstrcpy(fIconFont, L"Segoe Fluent Icons");
		fc->Release();
	}
	return TRUE;
}
//icon: 0 界面字体 1 图标字体 2 状态条字体 3 状态条用的图标字体（2、3 跟着状态条字号变，改设置时清掉）
WCHAR fSbFont[LF_FACESIZE] = L"Segoe UI";
struct FTF { float size; int weight; int icon; IDWriteTextFormat* tf; };
FTF fTF[48];
int nTF = 0;
void FFormatFlushBar()
{
	int n = 0;
	for (int i = 0; i < nTF; i++)
	{
		if (fTF[i].icon >= 2)
			fTF[i].tf->Release();
		else
			fTF[n++] = fTF[i];
	}
	nTF = n;
}
IDWriteTextFormat* FFormat(float size, int weight, int icon)
{
	for (int i = 0; i < nTF; i++)
		if (fTF[i].size == size && fTF[i].weight == weight && fTF[i].icon == icon)
			return fTF[i].tf;
	IDWriteTextFormat* tf = NULL;
	if (nTF >= 48 || FAILED(fDW->CreateTextFormat((icon & 1) ? fIconFont : icon == 2 ? fSbFont : fUiFont, NULL, (DWRITE_FONT_WEIGHT)weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"zh-CN", &tf)))
		return NULL;
	tf->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
	tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	fTF[nTF].size = size;
	fTF[nTF].weight = weight;
	fTF[nTF].icon = icon;
	fTF[nTF].tf = tf;
	nTF++;
	return tf;
}
//绘制到 32 位预乘 alpha 位图，再用 UpdateLayeredWindow 显示。软件渲染：图小、1 秒一帧，不值得唤醒显卡（双显卡笔记本上会唤醒独显）
struct FSURF { HDC dc; HBITMAP bmp, old; void* bits; int w, h; float dpi; ID2D1DCRenderTarget* rt; ID2D1SolidColorBrush* br; };
void FSurfFree(FSURF* s)
{
	if (s->br) s->br->Release();
	if (s->rt) s->rt->Release();
	if (s->dc)
	{
		if (s->old) SelectObject(s->dc, s->old);
		DeleteDC(s->dc);
	}
	if (s->bmp) DeleteObject(s->bmp);
	memset(s, 0, sizeof(FSURF));
}
BOOL FSurfEnsure(FSURF* s, int w, int h, float dpi)
{
	if (s->rt && s->w == w && s->h == h && s->dpi == dpi)
		return TRUE;
	FSurfFree(s);
	BITMAPINFO bi;
	memset(&bi, 0, sizeof bi);
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = w;
	bi.bmiHeader.biHeight = -h;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	s->dc = CreateCompatibleDC(NULL);
	if (s->dc)
		s->bmp = CreateDIBSection(s->dc, &bi, DIB_RGB_COLORS, &s->bits, NULL, 0);
	if (!s->bmp)
	{
		FSurfFree(s);
		return FALSE;
	}
	s->old = (HBITMAP)SelectObject(s->dc, s->bmp);
	D2D1_RENDER_TARGET_PROPERTIES p = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);
	RECT rc = { 0, 0, w, h };
	if (FAILED(fD2D->CreateDCRenderTarget(&p, &s->rt)) || FAILED(s->rt->BindDC(s->dc, &rc)) ||
		FAILED(s->rt->CreateSolidColorBrush(FC(0), &s->br)))
	{
		FSurfFree(s);
		return FALSE;
	}
	s->rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	s->w = w;
	s->h = h;
	s->dpi = dpi;
	return TRUE;
}
//当前绘制目标（单线程，界面线程上用）
ID2D1RenderTarget* fRT = NULL;
ID2D1SolidColorBrush* fBr = NULL;
inline D2D1_RECT_F FR(float x, float y, float w, float h) { D2D1_RECT_F r = { x, y, x + w, y + h }; return r; }
void FFill(float x, float y, float w, float h, DWORD rgb, float a, float r = 0)
{
	fBr->SetColor(FC(rgb, a));
	if (r > 0)
		fRT->FillRoundedRectangle(D2D1::RoundedRect(FR(x, y, w, h), r, r), fBr);
	else
		fRT->FillRectangle(FR(x, y, w, h), fBr);
}
void FStroke(float x, float y, float w, float h, DWORD rgb, float a, float r, float sw = 1.f)
{
	fBr->SetColor(FC(rgb, a));
	float o = sw / 2;
	fRT->DrawRoundedRectangle(D2D1::RoundedRect(FR(x + o, y + o, w - sw, h - sw), r, r), fBr, sw);
}
void FLine(float x1, float y1, float x2, float y2, DWORD rgb, float a, float sw = 1.f)
{
	fBr->SetColor(FC(rgb, a));
	fRT->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), fBr, sw);
}
//align: 0 左 1 中 2 右
void FText(LPCWSTR s, float x, float y, float w, float h, float size, int weight, DWORD rgb, float a = 1.f, int align = 0, int icon = 0)
{
	IDWriteTextFormat* tf = FFormat(size, weight, icon);
	if (!tf)
		return;
	tf->SetTextAlignment(align == 0 ? DWRITE_TEXT_ALIGNMENT_LEADING : align == 1 ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_TRAILING);
	fBr->SetColor(FC(rgb, a));
	fRT->DrawText(s, lstrlen(s), tf, FR(x, y, w, h), fBr, D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
float FTextW(LPCWSTR s, float size, int weight, int icon = 0)
{
	IDWriteTextFormat* tf = FFormat(size, weight, icon);
	IDWriteTextLayout* tl = NULL;
	float w = 0;
	if (tf && SUCCEEDED(fDW->CreateTextLayout(s, lstrlen(s), tf, 2000.f, 100.f, &tl)))
	{
		DWRITE_TEXT_METRICS m;
		if (SUCCEEDED(tl->GetMetrics(&m)))
			w = m.widthIncludingTrailingWhitespace;
		tl->Release();
	}
	return w;
}
//平滑曲线（Catmull-Rom 转三次贝塞尔），bArea 时闭合到底边做面积
ID2D1PathGeometry* FCurve(const float* v, int n, float x, float y, float w, float h, float vmax, BOOL bArea)
{
	ID2D1PathGeometry* g = NULL;
	ID2D1GeometrySink* sk = NULL;
	if (n < 2 || FAILED(fD2D->CreatePathGeometry(&g)) || FAILED(g->Open(&sk)))
	{
		if (g) g->Release();
		return NULL;
	}
	D2D1_POINT_2F p[64];
	if (n > 64) n = 64;
	for (int i = 0; i < n; i++)
	{
		float t = v[i] > vmax ? vmax : (v[i] < 0 ? 0 : v[i]);
		p[i].x = x + w * i / (n - 1);
		p[i].y = y + h - t / vmax * h;
	}
	sk->BeginFigure(bArea ? D2D1::Point2F(x, y + h) : p[0], bArea ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
	if (bArea)
		sk->AddLine(p[0]);
	for (int i = 0; i < n - 1; i++)
	{
		D2D1_POINT_2F p0 = p[i > 0 ? i - 1 : i], p1 = p[i], p2 = p[i + 1], p3 = p[i + 2 < n ? i + 2 : i + 1];
		D2D1_BEZIER_SEGMENT b;
		b.point1 = D2D1::Point2F(p1.x + (p2.x - p0.x) / 6, p1.y + (p2.y - p0.y) / 6);
		b.point2 = D2D1::Point2F(p2.x - (p3.x - p1.x) / 6, p2.y - (p3.y - p1.y) / 6);
		b.point3 = p2;
		//防止贝塞尔控制点越过底边/顶边产生“下探”
		if (b.point1.y > y + h) b.point1.y = y + h;
		if (b.point2.y > y + h) b.point2.y = y + h;
		if (b.point1.y < y) b.point1.y = y;
		if (b.point2.y < y) b.point2.y = y;
		sk->AddBezier(b);
	}
	if (bArea)
		sk->AddLine(D2D1::Point2F(x + w, y + h));
	sk->EndFigure(bArea ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
	sk->Close();
	sk->Release();
	return g;
}
void FDrawSeries(const float* v, int n, float x, float y, float w, float h, float vmax, DWORD rgb, BOOL bArea, float sw)
{
	if (bArea)
	{
		ID2D1PathGeometry* ga = FCurve(v, n, x, y, w, h, vmax, TRUE);
		if (ga)
		{
			D2D1_GRADIENT_STOP st[2] = { { 0.f, FC(rgb, fDark ? .30f : .20f) }, { 1.f, FC(rgb, 0.f) } };
			ID2D1GradientStopCollection* sc = NULL;
			ID2D1LinearGradientBrush* lb = NULL;
			if (SUCCEEDED(fRT->CreateGradientStopCollection(st, 2, &sc)) &&
				SUCCEEDED(fRT->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, y), D2D1::Point2F(0, y + h)), sc, &lb)))
			{
				fRT->FillGeometry(ga, lb);
				lb->Release();
			}
			if (sc) sc->Release();
			ga->Release();
		}
	}
	ID2D1PathGeometry* gl = FCurve(v, n, x, y, w, h, vmax, FALSE);
	if (gl)
	{
		fBr->SetColor(FC(rgb));
		fRT->DrawGeometry(gl, fBr, sw);
		gl->Release();
	}
}
//////////////////////////////////////////////////////////////////////////////// 历史数据（数据线程每秒写，界面线程读）
#define HIST_N 60
float fHistCpu[HIST_N];
float fHistDown[HIST_N];
float fHistUp[HIST_N];
void FHistPush(float cpu, float down, float up)
{
	for (int i = 0; i < HIST_N - 1; i++)
	{
		fHistCpu[i] = fHistCpu[i + 1];
		fHistDown[i] = fHistDown[i + 1];
		fHistUp[i] = fHistUp[i + 1];
	}
	fHistCpu[HIST_N - 1] = cpu;
	fHistDown[HIST_N - 1] = down;
	fHistUp[HIST_N - 1] = up;
}
//速率文字：数字和单位分开（单位用次要色）。bBit 时按 bit 显示；minU 最小单位（1 至少 K，2 至少 M，同“流量单位”设置）
void FRateStr(ULONG64 b, BOOL bBit, WCHAR* num, WCHAR* unit, int minU = 0)
{
	ULONG64 v = bBit ? b * 8 : b;
	ULONG64 k = bBit ? 1000 : 1024;
	const WCHAR* u[5] = { L"B/s", L"KB/s", L"MB/s", L"GB/s", L"TB/s" };
	const WCHAR* ub[5] = { L"b/s", L"Kb/s", L"Mb/s", L"Gb/s", L"Tb/s" };
	int i = 0;
	ULONG64 x100 = v * 100;
	while (i < 4 && (v >= k || i < minU))
	{
		x100 = v * 100 / k;
		v /= k;
		i++;
	}
	if (i == 0)
		wsprintf(num, L"%d", (int)v);
	else if (x100 >= 10000)
		wsprintf(num, L"%d", (int)(x100 / 100));
	else if (x100 >= 1000)
		wsprintf(num, L"%d.%d", (int)(x100 / 100), (int)(x100 / 10 % 10));
	else
		wsprintf(num, L"%d.%.2d", (int)(x100 / 100), (int)(x100 % 100));
	lstrcpy(unit, bBit ? ub[i] : u[i]);
}
//GB 一位小数
void FGB(ULONG64 b, WCHAR* sz)
{
	ULONG64 x = b * 10 / 1073741824;
	wsprintf(sz, L"%d.%d", (int)(x / 10), (int)(x % 10));
}

//////////////////////////////////////////////////////////////////////////////// 悬停窗口
#define FLY_W 368.f//卡片宽（DIP）
#define FLY_M 20.f//四周留给阴影的边
#define FLY_ROWS 5
//卡片里各块的尺寸（DIP），静态底图和内容共用
#define FLY_P 14.f//卡片内边距
#define FLY_HEAD 44.f//标题区
#define FLY_TH 116.f//指标卡高
#define FLY_NH 150.f//网络卡高
#define FLY_PH (40.f + FLY_ROWS * 32 + 8)//进程卡高
#define FLY_GAP 8.f
#define FLY_FOOT 48.f
#define FLY_H (FLY_P + FLY_HEAD + FLY_TH * 2 + FLY_GAP * 2 + FLY_NH + FLY_GAP + FLY_PH + FLY_GAP + FLY_FOOT)
enum {
	FH_NONE = 0, FH_CPU, FH_MEM, FH_TEMP, FH_DISK0, FH_DISK1, FH_DISK2, FH_NET, FH_NETCHIP, FH_SEG0,
	FH_ROW0 = FH_SEG0 + 4, FH_ROWDIR0 = FH_ROW0 + FLY_ROWS, FH_ROWKILL0 = FH_ROWDIR0 + FLY_ROWS, FH_GEAR = FH_ROWKILL0 + FLY_ROWS, FH_POWER, FH_MAX
};
struct FLY
{
	FSURF s;
	float sc;//DPI 缩放
	float H;//卡片高（DIP）
	int x, y;//窗口最终位置（像素）
	int slide;//动画位移方向：1 从下往上，-1 从上往下
	int hover;
	int tab;//进程排行：0 CPU 1 内存 2 上传 3 下载
	int anim;//0 静止 1 打开中 2 关闭中
	LARGE_INTEGER t0;
	int leave;//鼠标离开计数（100ms 一次）
	D2D1_RECT_F hit[FH_MAX];
	DWORD pid[FLY_ROWS];
	WCHAR drive[3][4];
	float alpha;//当前不透明度 0..1
	float offset;//当前位移 DIP
	ID2D1Bitmap* bg;//静态底图（阴影、卡片、各块底色、标题、底栏），每次打开时画一次
} fly;
DWORD fMaxMhz = 0;
void FlyCardRect(float* x, float* y) { *x = FLY_M; *y = FLY_M; }
inline BOOL FHit(int id, float x, float y) { D2D1_RECT_F& r = fly.hit[id]; return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }
void FHitSet(int id, float x, float y, float w, float h) { fly.hit[id] = FR(x, y, w, h); }
//块的底色在静态底图里，这里只画悬停时叠加的一层
void FTile(float x, float y, float w, float h, BOOL hover)
{
	if (hover)
		FFill(x, y, w, h, fDark ? 0xFFFFFF : 0x000000, fDark ? .04f : .025f, 6);
}
void FTileBase(float x, float y, float w, float h)
{
	FFill(x, y, w, h, 0xFFFFFF, fDark ? .055f : .72f, 6);
	FStroke(x, y, w, h, fDark ? 0xFFFFFF : 0x000000, fDark ? .07f : .06f, 6);
}
//静态底图：软件渲染下大面积圆角填充很贵（阴影 12 层 + 卡片 + 6 块约占一帧的 2/3），只在打开时画一次
void FlyPaintBg()
{
	ID2D1BitmapRenderTarget* brt = NULL;
	ID2D1SolidColorBrush* br = NULL;
	if (FAILED(fly.s.rt->CreateCompatibleRenderTarget(&brt)))
		return;
	if (FAILED(brt->CreateSolidColorBrush(FC(0), &br)))
	{
		brt->Release();
		return;
	}
	fRT = brt;
	fBr = br;
	brt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	brt->BeginDraw();
	brt->Clear(FC(0, 0));
	float cx = FLY_M, cy = FLY_M, W = FLY_W, H = fly.H;
	//阴影：多层叠加近似模糊，偏下
	for (int i = 1; i <= 12; i++)
	{
		float d = i * 1.35f;
		FFill(cx - d, cy - d + 7, W + d * 2, H + d * 2, 0, (fDark ? .055f : .03f) * (1.f - i / 13.f), 8 + d);
	}
	FFill(cx, cy, W, H, fDark ? 0x2C2C2C : 0xF9F9FA, 1, 8);
	FStroke(cx, cy, W, H, 0x000000, fDark ? .45f : .10f, 8);
	if (fDark)
		FStroke(cx + 1, cy + 1, W - 2, H - 2, 0xFFFFFF, .05f, 7);
	float x = cx + FLY_P, y = cy + FLY_P, iw = W - FLY_P * 2, tw = (iw - FLY_GAP) / 2;
	FText(L"系统状态", x + 2, y, 200, 20, 15, 600, FT1());
	y += FLY_HEAD;
	FTileBase(x, y, tw, FLY_TH);
	FTileBase(x + tw + FLY_GAP, y, tw, FLY_TH);
	y += FLY_TH + FLY_GAP;
	FTileBase(x, y, tw, FLY_TH);
	FTileBase(x + tw + FLY_GAP, y, tw, FLY_TH);
	y += FLY_TH + FLY_GAP;
	FTileBase(x, y, iw, FLY_NH);
	y += FLY_NH + FLY_GAP;
	FTileBase(x, y, iw, FLY_PH);
	//底栏
	float fy = cy + H - FLY_FOOT;
	brt->PushAxisAlignedClip(FR(cx, fy, W, FLY_FOOT), D2D1_ANTIALIAS_MODE_ALIASED);
	br->SetColor(FC(fDark ? 0x202020 : 0xEEEEF0, fDark ? 1.f : .7f));
	brt->FillRoundedRectangle(D2D1::RoundedRect(FR(cx + 0.5f, fy, W - 1, FLY_FOOT - .5f), 7.5f, 7.5f), br);
	brt->PopAxisAlignedClip();
	FLine(cx + 1, fy + 0.5f, cx + W - 1, fy + 0.5f, 0x000000, fDark ? .35f : .07f);
	float lx = cx + 16, ly = fy + 16;//小标志
	FFill(lx, ly, 16, 16, fAccentL, 1, 4);
	FFill(lx + 4, ly + 9, 2, 4, 0xFFFFFF, 1, 1);
	FFill(lx + 7, ly + 5, 2, 8, 0xFFFFFF, 1, 1);
	FFill(lx + 10, ly + 7, 2, 6, 0xFFFFFF, 1, 1);
	FText(L"TrayS", lx + 24, ly, 80, 16, 12, 400, FT2());
	brt->EndDraw();
	brt->GetBitmap(&fly.bg);
	br->Release();
	brt->Release();
	fRT = NULL;
	fBr = NULL;
}
//标题行：图标 + 文字 + 右侧小字
void FTileHead(float x, float y, float w, LPCWSTR glyph, LPCWSTR title, LPCWSTR right)
{
	FText(glyph, x, y, 16, 16, 13, 400, FT2(), 1, 0, TRUE);
	FText(title, x + 20, y, w - 20, 16, 12, 400, FT2());
	if (right && right[0])
		FText(right, x, y, w, 16, 11.5f, 400, FT3(), 1, 2);
}
//大号数字 + 单位
void FBig(float x, float y, LPCWSTR num, LPCWSTR unit, DWORD rgb)
{
	FText(num, x, y, 120, 32, 26, 600, rgb);
	float nw = FTextW(num, 26, 600);
	FText(unit, x + nw + 2, y + 6, 40, 26, 13, 600, FT2());
}
void FBar(float x, float y, float w, float h, float pct, DWORD rgb, float a = 1.f)
{
	FFill(x, y, w, h, fDark ? 0xFFFFFF : 0x000000, fDark ? .12f : .08f, h / 2);
	if (pct > 0)
	{
		float fw = w * (pct > 1 ? 1 : pct);
		if (fw < h) fw = h;
		FFill(x, y, fw, h, rgb, a, h / 2);
	}
}
void FIconBtn(float x, float y, float s, LPCWSTR glyph, int id, DWORD rgb, BOOL danger = FALSE)
{
	BOOL hv = fly.hover == id;
	if (hv)
		FFill(x, y, s, s, danger ? F_CRIT : (fDark ? 0xFFFFFF : 0x000000), danger ? .14f : (fDark ? .08f : .06f), 4);
	FText(glyph, x, y, s, s, s > 28 ? 15.f : 13.f, 400, hv && danger ? FCritText() : rgb, 1, 1, TRUE);
	FHitSet(id, x, y, s, s);
}
void FlyPaint()
{
	FSURF* s = &fly.s;
	if (!fly.bg)
		FlyPaintBg();
	fRT = s->rt;
	fBr = s->br;
	memset(fly.hit, 0, sizeof(fly.hit));
	fRT->BeginDraw();
	fRT->Clear(FC(0, 0));
	float cx = FLY_M, cy = FLY_M, W = FLY_W, H = fly.H;
	if (fly.bg)
		fRT->DrawBitmap(fly.bg, FR(0, 0, s->w * 96.f / s->dpi, s->h * 96.f / s->dpi), 1.f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
	WCHAR sz[96], s2[48];
	float x = cx + FLY_P, y = cy + FLY_P, iw = W - FLY_P * 2;
	//标题（“系统状态”在底图里）
	ULONG64 up = GetTickCount64() / 60000;
	if (up >= 1440)
		wsprintf(sz, L"已开机 %d 天 %d 小时", (int)(up / 1440), (int)(up / 60 % 24));
	else
		wsprintf(sz, L"已开机 %d 小时 %d 分钟", (int)(up / 60), (int)(up % 60));
	FText(sz, x + 2, y + 21, 240, 16, 12, 400, FT2());
	y += FLY_HEAD;
	//四张指标卡
	float tw = (iw - FLY_GAP) / 2, th = FLY_TH;
	//CPU
	{
		BOOL hv = fly.hover == FH_CPU;
		FTile(x, y, tw, th, hv);
		FHitSet(FH_CPU, x, y, tw, th);
		FTileHead(x + 12, y + 11, tw - 24, L"\xE950", L"CPU", NULL);
		wsprintf(sz, L"%d", (int)iCPU);
		FBig(x + 12, y + 30, sz, L"%", iCPU > TraySave.dNumValues[5] ? FCritText() : iCPU > TraySave.dNumValues[4] ? FWarnText() : FT1());
		float mx = 25;
		for (int i = 0; i < HIST_N; i++)
			while (fHistCpu[i] > mx && mx < 100)
				mx = mx < 50 ? 50.f : 100.f;
		FDrawSeries(fHistCpu, HIST_N, x + 12, y + 66, tw - 24, 26, mx, FAccent(), TRUE, 1.6f);
		if (fMaxMhz)
			wsprintf(sz, L"%d 线程 · 最高 %d.%.2d GHz", dNumProcessor, fMaxMhz / 1000, fMaxMhz % 1000 / 10);
		else
			wsprintf(sz, L"%d 线程", dNumProcessor);
		FText(sz, x + 12, y + 95, tw - 24, 14, 11.5f, 400, FT2());
	}
	//内存
	{
		float tx = x + tw + FLY_GAP;
		BOOL hv = fly.hover == FH_MEM;
		FTile(tx, y, tw, th, hv);
		FHitSet(FH_MEM, tx, y, tw, th);
		FTileHead(tx + 12, y + 11, tw - 24, L"\xEEA0", L"内存", NULL);
		wsprintf(sz, L"%d", MemoryStatusEx.dwMemoryLoad);
		FBig(tx + 12, y + 30, sz, L"%", MemoryStatusEx.dwMemoryLoad > TraySave.dNumValues[7] ? FCritText() : MemoryStatusEx.dwMemoryLoad > TraySave.dNumValues[6] ? FWarnText() : FT1());
		WCHAR a[16], b[16];
		FGB(MemoryStatusEx.ullTotalPhys - MemoryStatusEx.ullAvailPhys, a);
		FGB(MemoryStatusEx.ullTotalPhys, b);
		wsprintf(sz, L"已用 %s / %s GB", a, b);
		FText(sz, tx + 12, y + 64, tw - 24, 14, 11.5f, 400, FT2());
		FBar(tx + 12, y + 83, tw - 24, 4, MemoryStatusEx.dwMemoryLoad / 100.f, FAccent());
		FGB(MemoryStatusEx.ullTotalPageFile - MemoryStatusEx.ullAvailPageFile, a);
		FGB(MemoryStatusEx.ullTotalPageFile, b);
		wsprintf(sz, L"%s / %s GB", a, b);
		FText(L"虚拟内存", tx + 12, y + 92, tw - 24, 14, 11, 400, FT2());
		FText(sz, tx + 12, y + 92, tw - 24, 14, 11, 400, FT2(), 1, 2);
		float vp = MemoryStatusEx.ullTotalPageFile ? (float)(MemoryStatusEx.ullTotalPageFile - MemoryStatusEx.ullAvailPageFile) / MemoryStatusEx.ullTotalPageFile : 0;
		FBar(tx + 12, y + 108, tw - 24, 3, vp, fDark ? 0x8AA6D0 : 0x6B8FC4);
	}
	y += th + FLY_GAP;
	//温度
	{
		FTileHead(x + 12, y + 11, tw - 24, L"\xE9CA", L"温度", NULL);
		int warn = (int)TraySave.dNumValues[2], crit = (int)TraySave.dNumValues[3];
		BOOL hasCpu = TraySave.bMonitorTemperature && (bRing0 || hOHMA) && TrayData->iTemperature1 > 0;
		BOOL hasGpu = TraySave.bMonitorTemperature && (hNVDLL || hATIDLL || hOHMA);
		int t = hasCpu ? TrayData->iTemperature1 : (hasGpu ? TrayData->iTemperature2 : 0);
		if (t > 0 && !(bNvGpuSleeping && !hasCpu))
		{
			wsprintf(sz, L"%d", t);
			FBig(x + 12, y + 30, sz, L"°C", t > crit ? FCritText() : t > warn ? FWarnText() : FT1());
		}
		else
			FBig(x + 12, y + 30, L"--", L"°C", FT3());
		if (!TraySave.bMonitorTemperature)
			lstrcpy(sz, L"未开启温度监控");
		else if (hasCpu && hasGpu)
		{
			if (bNvGpuSleeping)
				lstrcpy(sz, L"处理器 · 显卡休眠");
			else
				wsprintf(sz, L"处理器 · 显卡 %d°C", TrayData->iTemperature2);
		}
		else
			lstrcpy(sz, hasCpu ? L"处理器" : L"显卡");
		FText(sz, x + 12, y + 64, tw - 24, 14, 11.5f, 400, FT2());
		float smax = (float)(crit + 20 > 110 ? crit + 20 : 110);
		float bx = x + 12, bw = tw - 24, by = y + 86;
		float pw = warn / smax, pc = crit / smax;
		FFill(bx, by, bw * pw, 4, fDark ? 0xFFFFFF : 0x000000, fDark ? .14f : .10f, 2);
		FFill(bx + bw * pw, by, bw * (pc - pw), 4, F_WARN, .65f);
		FFill(bx + bw * pc, by, bw * (1 - pc), 4, F_CRIT, .65f, 2);
		if (t > 0)
		{
			float kx = bx + bw * (t > smax ? 1.f : t / smax);
			FFill(kx - 1.5f, by - 3, 3, 10, FT1(), 1, 1.5f);
		}
		FText(L"0°", bx, by + 8, 30, 14, 10.5f, 400, FT3());
		wsprintf(sz, L"%d°", warn);
		FText(sz, bx + bw * pw - 20, by + 8, 40, 14, 10.5f, 400, FT3(), 1, 1);
		wsprintf(sz, L"%d°", crit);
		FText(sz, bx + bw * pc - 20, by + 8, 40, 14, 10.5f, 400, FT3(), 1, 1);
	}
	//磁盘：只看本地固定磁盘（网络盘断开时 GetDiskFreeSpaceEx 会卡住界面线程）
	{
		float tx = x + tw + FLY_GAP;
		WCHAR wDrive[MAX_PATH];
		DWORD dwLen = GetLogicalDriveStrings(MAX_PATH, wDrive);
		int n = 0, nAll = 0;
		WCHAR warnDrv[4] = { 0 };
		ULONG64 warnFree = 0, dFree[3], dTot[3];
		BOOL dWarn[3];
		for (DWORD i = 0; dwLen && i < dwLen / 4; i++)
		{
			LPWSTR d = wDrive + i * 4;
			if (GetDriveType(d) != DRIVE_FIXED)
				continue;
			ULARGE_INTEGER fa, tot, fr;
			if (!GetDiskFreeSpaceEx(d, &fa, &tot, &fr) || tot.QuadPart == 0)
				continue;
			nAll++;
			BOOL bWarn = fr.QuadPart < tot.QuadPart / 10;
			if (bWarn && !warnDrv[0])
			{
				warnDrv[0] = d[0];
				warnDrv[1] = L':';
				warnFree = fr.QuadPart;
			}
			if (n >= 3)
				continue;
			fly.drive[n][0] = d[0];
			fly.drive[n][1] = L':';
			fly.drive[n][2] = L'\\';
			fly.drive[n][3] = 0;
			dFree[n] = fr.QuadPart;
			dTot[n] = tot.QuadPart;
			dWarn[n] = bWarn;
			n++;
		}
		//只有一块盘时多一行可用/总量，免得卡片空着
		float dy = y + 34, rh = nAll == 1 ? 40.f : 20.f;
		for (int i = 0; i < n; i++, dy += rh)
		{
			int pct = (int)((dTot[i] - dFree[i]) * 100 / dTot[i]);
			if (fly.hover == FH_DISK0 + i)
				FFill(tx + 6, dy - 3, tw - 12, rh, fDark ? 0xFFFFFF : 0x000000, .05f, 4);
			FHitSet(FH_DISK0 + i, tx + 6, dy - 3, tw - 12, rh);
			s2[0] = fly.drive[i][0];
			s2[1] = L':';
			s2[2] = 0;
			FText(s2, tx + 12, dy, 22, 14, 11.5f, 400, FT1());
			FBar(tx + 34, dy + 5, tw - 90, 4, pct / 100.f, dWarn[i] ? F_WARN : FAccent());
			wsprintf(sz, L"%d%%", pct);
			FText(sz, tx + 12, dy, tw - 24, 14, 11.5f, 600, FT1(), 1, 2);
			if (nAll == 1)
			{
				WCHAR a[16], b[16];
				FGB(dFree[i], a);
				FGB(dTot[i], b);
				wsprintf(sz, L"可用 %s GB，共 %s GB", a, b);
				FText(sz, tx + 12, dy + 18, tw - 24, 14, 11, 400, FT2());
			}
		}
		wsprintf(sz, nAll > 3 ? L"共 %d 个" : L"", nAll);
		FTileHead(tx + 12, y + 11, tw - 24, L"\xEDA2", L"磁盘", sz);
		if (warnDrv[0])
		{
			WCHAR g[16];
			FGB(warnFree, g);
			wsprintf(sz, L"%s 可用空间不足 %s GB", warnDrv, g);
			FText(L"\xE7BA", tx + 12, y + 96, 14, 14, 11, 400, fDark ? F_WARN : 0xC88400, 1, 0, TRUE);
			FText(sz, tx + 28, y + 96, tw - 40, 14, 11, 400, FWarnText());
		}
	}
	y += th + FLY_GAP;
	//网络
	{
		float nh = FLY_NH;
		BOOL hv = fly.hover == FH_NET;
		FTile(x, y, iw, nh, hv);
		FHitSet(FH_NET, x, y, iw, nh);
		BOOL bBit = HIWORD(TraySave.iUnit);
		int sel = -1;
		for (int i = 0; i < nTraffic; i++)
			if (traffic[i].AdapterName && (TraySave.AdpterName[0] == 0 ? traffic[i].in_bytes > 0 : lstrcmpA(traffic[i].AdapterName, TraySave.AdpterName) == 0))
			{
				sel = i;
				if (TraySave.AdpterName[0] || traffic[i].in_byte > 0)
					break;
			}
		if (TraySave.AdpterName[0] == 0)
			lstrcpy(sz, L"全部网卡");
		else if (sel >= 0 && traffic[sel].FriendlyName)
			wsprintf(sz, L"%s · %s", traffic[sel].FriendlyName, traffic[sel].IP4);
		else
			lstrcpy(sz, L"选择网卡");
		FTileHead(x + 12, y + 11, iw - 24, L"\xE839", L"网络", NULL);
		float chipW = FTextW(sz, 11.5f, 400) + 26;
		BOOL hvc = fly.hover == FH_NETCHIP;
		if (hvc)
			FFill(x + iw - 12 - chipW + 4, y + 7, chipW, 24, fDark ? 0xFFFFFF : 0x000000, .06f, 4);
		FText(sz, x + iw - 12 - chipW + 10, y + 11, chipW - 26, 16, 11.5f, 400, FT2());
		FText(L"\xE70D", x + iw - 12 - 14, y + 11, 12, 16, 9, 400, FT2(), 1, 1, TRUE);
		FHitSet(FH_NETCHIP, x + iw - 12 - chipW + 4, y + 7, chipW, 24);
		//图例
		WCHAR n1[16], u1[8];
		float lx = x + 12, ly = y + 34;
		FFill(lx, ly + 7, 12, 2, FDown(), 1, 1);
		FText(L"下载", lx + 18, ly, 40, 16, 12, 400, FT2());
		FRateStr(TrayData->s_in_byte, bBit, n1, u1);
		wsprintf(sz, L"%s %s", n1, u1);
		float lw = FTextW(L"下载", 12, 400);
		FText(sz, lx + 18 + lw + 6, ly, 120, 16, 12, 600, FT1());
		lx += 18 + lw + 6 + FTextW(sz, 12, 600) + 18;
		FFill(lx, ly + 7, 12, 2, FUp(), 1, 1);
		FText(L"上传", lx + 18, ly, 40, 16, 12, 400, FT2());
		FRateStr(TrayData->s_out_byte, bBit, n1, u1);
		wsprintf(sz, L"%s %s", n1, u1);
		FText(sz, lx + 18 + lw + 6, ly, 120, 16, 12, 600, FT1());
		//曲线：纵轴取“好看”的上限
		float mx = 1;
		for (int i = 0; i < HIST_N; i++)
		{
			if (fHistDown[i] > mx) mx = fHistDown[i];
			if (fHistUp[i] > mx) mx = fHistUp[i];
		}
		double nice = 1024;
		const float steps[3] = { 1.f, 2.f, 5.f };
		for (int e = 0; e < 40; e++)
		{
			double base = 1024;
			for (int k = 0; k < e / 3; k++) base *= 10;
			nice = base * steps[e % 3];
			if (nice >= mx * 1.15)
				break;
		}
		float gx = x + 12, gy = y + 60, gw = iw - 24 - 46, gh = 64;
		for (int i = 0; i <= 2; i++)
		{
			float ly2 = gy + gh - gh * i / 2;
			FLine(gx, ly2, gx + gw, ly2, fDark ? 0xFFFFFF : 0x000000, i ? (fDark ? .08f : .07f) : (fDark ? .16f : .14f));
			if (i)
			{
				FRateStr((ULONG64)(nice * i / 2), bBit, n1, u1);
				wsprintf(sz, L"%s %s", n1, u1);
			}
			else
				lstrcpy(sz, L"0");
			FText(sz, gx + gw + 4, ly2 - 8, 42, 16, 10, 400, FT3(), 1, 2);
		}
		FDrawSeries(fHistDown, HIST_N, gx, gy, gw, gh, (float)nice, FDown(), TRUE, 2.f);
		FDrawSeries(fHistUp, HIST_N, gx, gy, gw, gh, (float)nice, FUp(), FALSE, 2.f);
		FText(L"60 秒前", gx, gy + gh + 5, 80, 14, 10.5f, 400, FT3());
		FText(L"现在", gx, gy + gh + 5, gw, 14, 10.5f, 400, FT3(), 1, 2);
		y += nh + FLY_GAP;
	}
	//进程
	{
		float ph = FLY_PH;
		FText(L"占用最高的进程", x + 12, y + 11, 200, 18, 12.5f, 600, FT1());
		float sw = 168, sx = x + iw - 12 - sw;
		FFill(sx, y + 9, sw, 24, fDark ? 0xFFFFFF : 0x000000, fDark ? .06f : .05f, 5);
		float segW = (sw - 4) / 4;
		const WCHAR* segName[4] = { L"CPU", L"内存", L"上传", L"下载" };
		for (int i = 0; i < 4; i++)
		{
			BOOL on = i == fly.tab;
			float bx2 = sx + 2 + segW * i;
			if (on)
			{
				FFill(bx2, y + 11, segW, 20, fDark ? 0x454545 : 0xFFFFFF, 1, 4);
				if (!fDark)
					FStroke(bx2, y + 11, segW, 20, 0x000000, .08f, 4);
			}
			else if (fly.hover == FH_SEG0 + i)
				FFill(bx2, y + 11, segW, 20, fDark ? 0xFFFFFF : 0x000000, .05f, 4);
			FText(segName[i], bx2, y + 11, segW, 20, 11.5f, on ? 600 : 400, on ? FT1() : FT2(), 1, 1);
			FHitSet(FH_SEG0 + i, bx2, y + 11, segW, 20);
		}
		//拷一份进程数据，数据线程会同时更新
		WCHAR nm[FLY_ROWS][37];
		DWORD pids[FLY_ROWS];
		float val[FLY_ROWS];
		SIZE_T mem[FLY_ROWS];//内存页：字节；上传 / 下载页：字节/秒
		int nRow = 0, tab = fly.tab;
		const WCHAR* wait = NULL;//没有可列的数据时显示的话
		WCHAR szWait[48];
		if (tab == 0 && !bProcCpuReady)//刚打开还没有第二次采样，CPU 数值都是 0，先不列
			wait = L"正在统计…";
		else if (tab >= 2 && netState != 2)
		{
			if (netState == -1)
				wait = L"需要以管理员身份运行 TrayS";
			else if (netState == -2)
			{
				wsprintf(szWait, L"无法统计网络（错误 %d）", (int)netErr);
				wait = szWait;
			}
			else
				wait = L"正在统计…";
		}
		EnterCriticalSection(&csProcess);
		for (int i = 0; i < FLY_ROWS && !wait; i++)
		{
			if (tab >= 2)
			{
				if (i >= nNetTop[tab - 2])
					break;
				lstrcpyn(nm[nRow], netTop[tab - 2][i].szExe, 37);
				pids[nRow] = netTop[tab - 2][i].pid;
				val[nRow] = 0;
				mem[nRow] = (SIZE_T)netTop[tab - 2][i].rate;
				nRow++;
				continue;
			}
			const WCHAR* e = tab ? (ppmu[i] ? ppmu[i]->szExe : NULL) : (ppcu[i] ? ppcu[i]->szExe : NULL);
			if (!e || !e[0])
				continue;
			lstrcpyn(nm[nRow], e, 37);
			pids[nRow] = tab ? ppmu[i]->dwProcessID : ppcu[i]->dwProcessID;
			val[nRow] = tab ? 0 : ppcu[i]->fCpuUsage;
			mem[nRow] = tab ? ppmu[i]->dwMemUsage : 0;
			nRow++;
		}
		LeaveCriticalSection(&csProcess);
		if (!wait && tab >= 2 && nRow == 0)
			wait = L"暂无网络活动";
		float vmax = 0.01f;
		for (int i = 0; i < nRow; i++)
			if ((tab ? (float)mem[i] : val[i]) > vmax)
				vmax = tab ? (float)mem[i] : val[i];
		const DWORD pal[8] = { 0xE8A33D, 0x3B82D6, 0xD9A520, 0x6B7280, 0x7C5CC4, 0x1B9E7A, 0xD9534F, 0x334155 };
		float ry = y + 40;
		if (wait)
			FText(wait, x, ry, iw, FLY_ROWS * 32.f, 12, 400, FT2(), 1, 1);
		float valW = tab >= 2 ? 66.f : 52.f;//速率要宽一些（“1023 KB/s”）
		for (int i = 0; i < FLY_ROWS; i++, ry += 32)
		{
			fly.pid[i] = i < nRow ? pids[i] : 0;
			if (i >= nRow)
				continue;
			BOOL hv = fly.hover == FH_ROW0 + i || fly.hover == FH_ROWDIR0 + i || fly.hover == FH_ROWKILL0 + i;
			if (hv)
				FFill(x + 6, ry, iw - 12, 32, fDark ? 0xFFFFFF : 0x000000, fDark ? .06f : .045f, 4);
			FHitSet(FH_ROW0 + i, x + 6, ry, iw - 12, 32);
			DWORD hsh = 0;
			for (const WCHAR* p = nm[i]; *p; p++)
				hsh = hsh * 31 + (*p | 0x20);
			FFill(x + 12, ry + 7, 18, 18, pal[hsh % 8], 1, 5);
			s2[0] = (nm[i][0] >= L'a' && nm[i][0] <= L'z') ? nm[i][0] - 32 : nm[i][0];
			s2[1] = 0;
			FText(s2, x + 12, ry + 7, 18, 18, 10, 700, 0xFFFFFF, 1, 1);
			if (tab >= 2)
			{
				WCHAR n[16], u[8];
				FRateStr(mem[i], HIWORD(TraySave.iUnit), n, u);
				wsprintf(sz, L"%s %s", n, u);
			}
			else if (tab == 1)
			{
				if (mem[i] >= 1048576000)
					wsprintf(sz, L"%d.%.2d GB", (int)(mem[i] * 100 / 1073741824 / 100), (int)(mem[i] * 100 / 1073741824 % 100));
				else
					wsprintf(sz, L"%d MB", (int)(mem[i] / 1048576));
			}
			else
			{
				int c = (int)(val[i] * 10);
				wsprintf(sz, L"%d.%d%%", c / 10, c % 10);
			}
			float nameW = iw - 12 - 30 - (valW + 98);
			if (hv)
			{
				wsprintf(s2, L"PID %d", pids[i]);
				float w1 = FTextW(nm[i], 12.5f, 400);
				if (w1 > nameW - 60) w1 = nameW - 60;
				FText(nm[i], x + 38, ry, w1, 32, 12.5f, 400, FT1());
				FText(s2, x + 38 + w1 + 6, ry, 80, 32, 11, 400, FT3());
				FIconBtn(x + iw - 12 - (valW + 4) - 76, ry + 4, 24, L"\xE838", FH_ROWDIR0 + i, FT2());
				FIconBtn(x + iw - 12 - (valW + 4) - 48, ry + 4, 24, L"\xE711", FH_ROWKILL0 + i, FT2(), TRUE);
			}
			else
			{
				FText(nm[i], x + 38, ry, nameW, 32, 12.5f, 400, FT1());
				float v = tab ? (float)mem[i] : val[i];
				FBar(x + iw - 12 - (valW + 4) - 76, ry + 14.5f, 70, 3, v / vmax, fDark ? 0x6E9BD8 : 0x7AA7E0);
			}
			FText(sz, x + iw - 12 - valW, ry, valW, 32, 12.5f, 600, FT1(), 1, 2);
		}
		y += ph;
	}
	//底栏（底色、标志在底图里）
	{
		float fy = cy + H - FLY_FOOT;
		FIconBtn(cx + W - 16 - 32 - 4 - 32, fy + 8, 32, L"\xE713", FH_GEAR, FT1());
		FIconBtn(cx + W - 16 - 32, fy + 8, 32, L"\xE7E8", FH_POWER, FT1());
	}
	fRT->EndDraw();
	fRT = NULL;
	fBr = NULL;
}
void FlyPresent(HWND h)
{
	POINT ptDst = { fly.x, fly.y + (int)(fly.offset * fly.sc) }, ptSrc = { 0, 0 };
	SIZE sz = { fly.s.w, fly.s.h };
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)(fly.alpha * 255 + .5f), AC_SRC_ALPHA };
	UpdateLayeredWindow(h, NULL, &ptDst, &sz, fly.s.dc, &ptSrc, 0, &bf, ULW_ALPHA);
}
float FlyElapsed()
{
	LARGE_INTEGER t, f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
	return (float)(t.QuadPart - fly.t0.QuadPart) * 1000.f / f.QuadPart;
}
#define FLY_OPEN_MS 200.f
#define FLY_CLOSE_MS 130.f
#define FLY_SLIDE 14.f
void FlyAnimStep(HWND h)
{
	float t = FlyElapsed();
	if (fly.anim == 1)
	{
		float p = t / FLY_OPEN_MS;
		if (p >= 1)
		{
			p = 1;
			fly.anim = 0;
			KillTimer(h, 1);
		}
		float e = 1 - (1 - p) * (1 - p) * (1 - p);//ease-out cubic
		fly.alpha = e;
		fly.offset = (1 - e) * FLY_SLIDE * fly.slide;
		FlyPresent(h);
	}
	else if (fly.anim == 2)
	{
		float p = t / FLY_CLOSE_MS;
		if (p >= 1)
		{
			KillTimer(h, 1);
			fly.anim = 0;
			DestroyWindow(h);
			return;
		}
		fly.alpha = 1 - p * p;
		fly.offset = p * p * 6 * fly.slide;
		FlyPresent(h);
	}
}
void FlyClose(HWND h)
{
	if (fly.anim == 2 || !IsWindow(h))
		return;
	fly.anim = 2;
	QueryPerformanceCounter(&fly.t0);
	SetTimer(h, 1, 10, NULL);
	FBarHoverUpdate();
}
//关闭动画中光标又回到状态条：从当前不透明度接着往回打开，不闪
void FlyCancelClose(HWND h)
{
	if (fly.anim != 2)
		return;
	//打开曲线 e = 1 - (1 - p)^3，由当前 alpha 反求进度 p（牛顿法开立方，不用 CRT）
	float r = 1 - fly.alpha, c = r > 0 ? (r + 1) / 2 : 0;
	for (int i = 0; i < 8 && r > 0; i++)
		c -= (c * c * c - r) / (3 * c * c);
	LARGE_INTEGER t, f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
	fly.t0.QuadPart = t.QuadPart - (LONGLONG)((1 - c) * FLY_OPEN_MS * f.QuadPart / 1000);
	fly.anim = 1;
	fly.leave = 0;
	SetTimer(h, 1, 10, NULL);
	FBarHoverUpdate();
}
//打开：hAnchor 是任务栏上的状态条
void FlyOpen(HWND h, HWND hAnchor)
{
	if (!FluentInit())
		return;
	FluentLoadTheme();
	if (fMaxMhz == 0)
	{
		typedef LONG(WINAPI* pfnCNPI)(int, PVOID, ULONG, PVOID, ULONG);
		HMODULE hp = LoadLibrary(L"powrprof.dll");
		pfnCNPI cnpi = hp ? (pfnCNPI)GetProcAddress(hp, "CallNtPowerInformation") : NULL;
		DWORD n = dNumProcessor ? dNumProcessor : 1;
		PROCESSOR_POWER_INFORMATION* pi = (PROCESSOR_POWER_INFORMATION*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PROCESSOR_POWER_INFORMATION) * n);
		if (cnpi && pi && cnpi(11, NULL, 0, pi, sizeof(PROCESSOR_POWER_INFORMATION) * n) == 0)
			fMaxMhz = pi[0].MaxMhz;
		if (pi) HeapFree(GetProcessHeap(), 0, pi);
		if (hp) FreeLibrary(hp);
	}
	UINT dpi = pGetDpiForWindow(hAnchor);
	if (dpi == 0) dpi = 96;
	fly.sc = dpi / 96.f;
	fly.H = FLY_H;
	RECT ar;
	GetWindowRect(hAnchor, &ar);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(MonitorFromWindow(hAnchor, MONITOR_DEFAULTTONEAREST), &mi);
	RECT wa = mi.rcWork;
	float fit = (wa.bottom - wa.top) / (fly.H + 24.f);//屏幕矮（如 1080p 150%）放不下时整体缩小
	if (fly.sc > fit)
		fly.sc = fit;
	if (fly.bg)//主题、DPI 可能变了，底图重画
	{
		fly.bg->Release();
		fly.bg = NULL;
	}
	int w = (int)((FLY_W + FLY_M * 2) * fly.sc + .5f), hh = (int)((fly.H + FLY_M * 2) * fly.sc + .5f);
	if (!FSurfEnsure(&fly.s, w, hh, fly.sc * 96.f))
		return;
	int m = (int)(FLY_M * fly.sc), gap = (int)(12 * fly.sc);
	int cardW = w - m * 2, cardH = hh - m * 2;
	int cx = (ar.left + ar.right) / 2 - cardW / 2;
	if (cx + cardW > wa.right - gap) cx = wa.right - gap - cardW;
	if (cx < wa.left + gap) cx = wa.left + gap;
	int cy;
	if ((ar.top + ar.bottom) / 2 > (mi.rcMonitor.top + mi.rcMonitor.bottom) / 2)
	{
		cy = (ar.top < wa.bottom ? ar.top : wa.bottom) - gap - cardH;
		fly.slide = 1;
	}
	else
	{
		cy = (ar.bottom > wa.top ? ar.bottom : wa.top) + gap;
		fly.slide = -1;
	}
	if (cy < wa.top + gap) cy = wa.top + gap;
	fly.x = cx - m;
	fly.y = cy - m;
	fly.hover = FH_NONE;
	fly.leave = 0;
	FlyPaint();
	fly.alpha = 0;
	fly.offset = FLY_SLIDE * fly.slide;
	FlyPresent(h);
	SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	fly.anim = 1;
	QueryPerformanceCounter(&fly.t0);
	SetTimer(h, 1, 10, NULL);//动画
	SetTimer(h, 2, 100, NULL);//检查鼠标是否离开
	FBarHoverUpdate();
	if (fly.tab >= 2)//上次停在上传 / 下载页
		NetTopStart();
}
void FlyRefresh(HWND h)
{
	if (fly.anim == 2 || !fly.s.rt)
		return;
	FlyPaint();
	FlyPresent(h);
}
int FlyHitTest(HWND h, LPARAM lParam)
{
	float x = GET_X_LPARAM(lParam) / fly.sc, y = GET_Y_LPARAM(lParam) / fly.sc;
	const int order[] = { FH_GEAR, FH_POWER, FH_SEG0, FH_SEG0 + 1, FH_SEG0 + 2, FH_SEG0 + 3, FH_NETCHIP };
	for (int i = 0; i < 7; i++)
		if (FHit(order[i], x, y))
			return order[i];
	for (int i = 0; i < FLY_ROWS; i++)
	{
		if (fly.pid[i] == 0)
			continue;
		if (FHit(FH_ROWDIR0 + i, x, y)) return FH_ROWDIR0 + i;
		if (FHit(FH_ROWKILL0 + i, x, y)) return FH_ROWKILL0 + i;
		if (FHit(FH_ROW0 + i, x, y)) return FH_ROW0 + i;
	}
	for (int i = 0; i < 3; i++)
		if (FHit(FH_DISK0 + i, x, y))
			return FH_DISK0 + i;
	if (FHit(FH_NET, x, y)) return FH_NET;
	if (FHit(FH_CPU, x, y)) return FH_CPU;
	if (FHit(FH_MEM, x, y)) return FH_MEM;
	return FH_NONE;
}
//鼠标移动：悬停目标变了才重绘
void FlyMouseMove(HWND h, LPARAM lParam)
{
	int ht = FlyHitTest(h, lParam);
	TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, h, 0 };
	TrackMouseEvent(&tme);
	if (ht != fly.hover)
	{
		//鼠标在进程行上时冻结进程列表，免得行在光标下面换来换去点错
		BOOL inRow = ht >= FH_ROW0 && ht < FH_GEAR;
		if (!inRow && inTipsProcessX)
			inTipsProcessX = FALSE;
		else if (inRow)
			inTipsProcessX = TRUE;
		fly.hover = ht;
		FlyRefresh(h);
	}
}
void FlyClick(HWND h, LPARAM lParam)
{
	int ht = FlyHitTest(h, lParam);
	BOOL bClose = TRUE;
	if (ht >= FH_SEG0 && ht < FH_SEG0 + 4)
	{
		fly.tab = ht - FH_SEG0;
		if (fly.tab >= 2)//网速排行只在这两页统计
			NetTopStart();
		else
			NetTopStop();
		FlyRefresh(h);
		bClose = FALSE;
	}
	else if (ht >= FH_ROWDIR0 && ht < FH_ROWDIR0 + FLY_ROWS)
		OpenProcessPath(fly.pid[ht - FH_ROWDIR0]);
	else if (ht >= FH_ROWKILL0 && ht < FH_ROWKILL0 + FLY_ROWS)
	{
		HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, fly.pid[ht - FH_ROWKILL0]);
		if (hProc)
		{
			TerminateProcess(hProc, 0);
			CloseHandle(hProc);
		}
		inTipsProcessX = FALSE;
		bClose = FALSE;
	}
	else if (ht >= FH_ROW0 && ht < FH_ROW0 + FLY_ROWS)
		RunProcess(NULL, fly.tab ? szPerfmon : szTaskmgr);
	else if (ht >= FH_DISK0 && ht <= FH_DISK2)
	{
		WCHAR sz[8];
		wsprintf(sz, L"o%s", fly.drive[ht - FH_DISK0]);
		RunProcess(NULL, sz);
	}
	else if (ht == FH_NETCHIP)
	{
		ShowSelectMenu(TRUE);
		bClose = FALSE;
	}
	else if (ht == FH_NET)
		RunProcess(NULL, szNetCpl);
	else if (ht == FH_CPU)
		RunProcess(NULL, szTaskmgr);
	else if (ht == FH_MEM)
		RunProcess(NULL, szPerfmon);
	else if (ht == FH_GEAR)
	{
		bSetting = TRUE;
		PostMessage(hMain, WM_TRAYS, 0, 0);
	}
	else if (ht == FH_POWER)
	{
		bRealClose = TRUE;
		TrayData->bExit = TRUE;
		PostMessage(hMain, WM_CLOSE, 0, 0);
	}
	else
		bClose = FALSE;
	if (bClose)
		FlyClose(h);
}
//100ms 一次：鼠标离开浮层和状态条 300ms 后关闭；弹出菜单打开时不关
void FlyLeaveCheck(HWND h)
{
	POINT pt;
	GetCursorPos(&pt);
	RECT rc = { fly.x + (int)(FLY_M * fly.sc), fly.y + (int)(FLY_M * fly.sc), fly.x + fly.s.w - (int)(FLY_M * fly.sc), fly.y + fly.s.h - (int)(FLY_M * fly.sc) };
	RECT br;
	GetWindowRect(hTaskBar, &br);
	if (PtInRect(&rc, pt) || PtInRect(&br, pt) || FindWindow(L"#32768", NULL))
		fly.leave = 0;
	else if (++fly.leave >= 3)
		FlyClose(h);
}
INT_PTR FlyProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TIMER:
		if (wParam == 1)
			FlyAnimStep(h);
		else if (wParam == 2)
			FlyLeaveCheck(h);
		return TRUE;
	case WM_FLY_DATA:
		FlyRefresh(h);
		return TRUE;
	case WM_MOUSEMOVE:
		FlyMouseMove(h, lParam);
		return TRUE;
	case WM_MOUSELEAVE:
		if (fly.hover != FH_NONE)
		{
			fly.hover = FH_NONE;
			inTipsProcessX = FALSE;
			FlyRefresh(h);
		}
		return TRUE;
	case WM_LBUTTONUP:
		FlyClick(h, lParam);
		return TRUE;
	case WM_SETCURSOR:
		SetCursor(LoadCursor(NULL, fly.hover != FH_NONE ? IDC_HAND : IDC_ARROW));
		SetWindowLongPtr(h, DWLP_MSGRESULT, TRUE);
		return TRUE;
	case WM_DESTROY:
		KillTimer(h, 1);
		KillTimer(h, 2);
		NetTopStop();
		inTipsProcessX = FALSE;
		fly.anim = 0;
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////// 状态条
//画在任务栏的子窗口上：先铺满取到的任务栏颜色（颜色键透明）或清成透明（每像素 alpha），文字用灰度抗锯齿
//坐标就是像素（渲染目标 96 DPI），字号按设置里的字体大小换算
ID2D1DCRenderTarget* fSbRT = NULL;
ID2D1SolidColorBrush* fSbBr = NULL;
BOOL fSbOn = FALSE;//SetWH 时决定：Direct2D 可用就用新样式，否则用原来的 GDI 绘制
float fSbV = 12, fSbL = 11, fSbU = 10.5f;//数值 / 标签 / 单位字号
float fSbPitch = 18;//两行显示时的行距
float fSbArrowW = 0, fSbNumW = 0, fSbUnitW = 0, fSbGlyphW = 0;
BOOL fSbDark = TRUE;
float fSbHover = 0;//悬停高亮 0..1，渐变
int fSbHoverTo = 0;
ULONGLONG fSbHoverT = 0;
int fSbDwell = 0;//Win11 轮询：光标在状态条上停了多久（ms）
inline DWORD FRgb(COLORREF c) { return (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c); }
inline DWORD FSbT1() { return fSbDark ? 0xFFFFFF : 0x1A1A1A; }
inline DWORD FSbT2() { return fSbDark ? 0xA8A8A8 : 0x6B6B6B; }
inline float FMax(float a, float b) { return a > b ? a : b; }
//三档颜色是否改过默认值。g: 0 流量/硬盘读写 1 占用/温度
BOOL FSbCustom(int g)
{
	const COLORREF def[2][3] = { { RGB(128, 128, 128), RGB(255, 255, 255), RGB(255, 0, 0) }, { RGB(0, 168, 0), RGB(255, 128, 0), RGB(255, 0, 0) } };
	for (int i = 0; i < 3; i++)
		if (TraySave.cMonitorColor[(g ? 4 : 1) + i] != def[g][i])
			return TRUE;
	return FALSE;
}
//设置里“数字按自定义颜色显示”：关时数字用正文色，开时用三档颜色；超阈值的底色块和警示图标都有。
//没设过（旧配置）时，改过三档颜色的算打开，保持原来按颜色显示
BOOL FNumColor()
{
	if (TraySave.iNumColor < 0)
		TraySave.iNumColor = FSbCustom(0) || FSbCustom(1) ? 1 : 0;
	return TraySave.iNumColor;
}
inline int FLevel(DWORD v, DWORD warn, DWORD crit) { return v > crit ? 2 : v > warn ? 1 : 0; }
void FSbLabel(LPCWSTR src, WCHAR* dst)//设置里的标签去掉结尾的冒号
{
	lstrcpyn(dst, src, 8);
	int n = lstrlen(dst);
	while (n > 0 && (dst[n - 1] == L':' || dst[n - 1] == L'\xFF1A' || dst[n - 1] == L' '))
		dst[--n] = 0;
}
//阴影模式（透明任务栏、浮动）：原来的做法是四周描一圈再往下一点，这里照做
void FSbText(LPCWSTR s, float x, float y, float w, float h, float size, int weight, DWORD rgb, int align, int icon = 2)
{
	if (bShadow)
	{
		const float o[5][2] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 }, { 0, 2 } };
		for (int i = 0; i < 5; i++)
			FText(s, x + o[i][0], y + o[i][1], w, h, size, weight, FRgb(bColor), 1, align, icon);
	}
	FText(s, x, y, w, h, size, weight, rgb, 1, align, icon);
}
//指标格：左标签，右数值。超阈值：预警琥珀色底，警告红底 + 警示图标（不只靠颜色）
void FSbMetric(float x, float y, float w, float h, LPCWSTR label, LPCWSTR val, int level, int g)
{
	float sc = iDPI / 96.f;
	if (TraySave.iMonitorSimple == 0 && label && label[0])
		FSbText(label, x, y, w, h, fSbL, 400, FSbT2(), 0);
	DWORD tc = FNumColor() ? FRgb(TraySave.cMonitorColor[(g ? 4 : 1) + level]) : FSbT1();
	if (level > 0)
	{
		float vw = FTextW(val, fSbV, 600, 2);
		float gw = level == 2 ? fSbGlyphW + 3 * sc : 0;
		float pw = vw + gw + 8 * sc, ph = fSbV * 1.4f;
		float px = x + w + 4 * sc - pw, py = y + (h - ph) / 2;
		FFill(px, py, pw, ph, level == 2 ? F_CRIT : F_WARN, level == 2 ? (fSbDark ? .32f : .20f) : (fSbDark ? .30f : .32f), 4 * sc);
		if (level == 2)
			FSbText(L"\xE7BA", px + 4 * sc, y, fSbGlyphW, h, fSbV * .85f, 400, fSbDark ? 0xFF6B6B : 0xC42B1C, 0, 3);
	}
	FSbText(val, x, y, w, h, fSbV, 600, tc, 2);
}
//速率格：左边箭头（硬盘是文字），右边数字 + 单位，单位列宽固定，数字右对齐不跳动
//kind: 0 上传 1 下载 2 读 3 写
void FSbRate(float x, float y, float w, float h, int kind, ULONG64 v, int level, BOOL bBit, int minU)
{
	float sc = iDPI / 96.f;
	int mode = TraySave.iMonitorSimple;
	WCHAR n[16], u[8], l[8];
	FRateStr(v, bBit, n, u, minU);
	float ux = x + w - fSbUnitW;
	if (level > 0)//超阈值：数字和单位垫底色块，同指标格
	{
		float vw = FTextW(n, fSbV, 600, 2), uw = FTextW(u, fSbU, 400, 2), ph = fSbV * 1.4f;
		float px = ux - 3 * sc - vw - 4 * sc;
		FFill(px, y + (h - ph) / 2, ux + uw + 4 * sc - px, ph, level == 2 ? F_CRIT : F_WARN, level == 2 ? (fSbDark ? .32f : .20f) : (fSbDark ? .30f : .32f), 4 * sc);
	}
	if (level == 2 && mode < 2)//警告：左边的箭头 / 标签换成警示图标（不只靠颜色，也不用加宽）
		FSbText(L"\xE7BA", x, y, fSbGlyphW, h, fSbV * .85f, 400, fSbDark ? 0xFF6B6B : 0xC42B1C, 0, 3);
	else if (kind <= 1)
	{
		if (mode < 2)
			FSbText(kind == 0 ? L"\xE74A" : L"\xE74B", x, y, fSbArrowW, h, fSbV * .8f, 400,
				kind == 0 ? (fSbDark ? 0xFF8A5C : 0xEB6834) : (fSbDark ? 0x6AA9FF : 0x2A78D6), 0, 3);
	}
	else if (mode < 2)
	{
		if (mode == 0)
			FSbLabel(kind == 2 ? TraySave.szDiskReadSec : TraySave.szDiskWriteSec, l);
		else
			lstrcpy(l, kind == 2 ? L"R" : L"W");
		FSbText(l, x, y, w, h, fSbL, 400, FSbT2(), 0);
	}
	DWORD tc = FNumColor() ? FRgb(TraySave.cMonitorColor[1 + level]) : FSbT1();
	FSbText(u, ux, y, fSbUnitW, h, fSbU, 400, FSbT2(), 0);
	FSbText(n, x, y, ux - x - 3 * sc, h, fSbV, 600, tc, 2);
}
//量宽度，覆盖 SetWH 里按 GDI 字体算出的 wTraffic 等（右键菜单的点击区域也用这些）
void FBarSetWH()
{
	fSbOn = FluentInit();
	if (!fSbOn)
		return;
	//字体没改过（微软雅黑）就用 Segoe UI Variable，中文自动回落到微软雅黑 UI；改过就用选的字体
	if (lstrcmpi(TraySave.TraybarFont.lfFaceName, L"微软雅黑") == 0 || lstrcmpi(TraySave.TraybarFont.lfFaceName, L"Microsoft YaHei") == 0)
		lstrcpyn(fSbFont, fUiFont, LF_FACESIZE);
	else
		lstrcpyn(fSbFont, TraySave.TraybarFont.lfFaceName, LF_FACESIZE);
	FFormatFlushBar();
	float sc = iDPI / 96.f;
	LONG lh = TraySave.TraybarFont.lfHeight;
	float px = lh < 0 ? (float)-lh : lh * .8f;//正数是含行距的高度
	fSbV = px * 12 / 14;//默认 14 像素 → 数值 12、标签 11、单位 10.5（设计稿的比例）
	fSbL = fSbV * 11 / 12;
	fSbU = fSbV * 10.5f / 12;
	fSbPitch = (float)(int)(fSbV * 1.45f + .5f);
	BOOL bBit = HIWORD(TraySave.iUnit);
	fSbGlyphW = FTextW(L"\xE7BA", fSbV * .85f, 400, 3);
	fSbArrowW = fSbV * .9f;
	fSbNumW = FTextW(L"8.88", fSbV, 600, 2);
	fSbUnitW = FTextW(bBit ? L"Mb/s" : L"MB/s", fSbU, 400, 2);
	int mode = TraySave.iMonitorSimple;
	wSpace = (int)(12 * sc + .5f);
	wHeight = (int)fSbPitch;
	if (!TraySave.bMonitorFloat && IsWindow(hTray))//停靠在横向任务栏上：高度向任务栏按钮（40）看齐，任务栏太矮时压缩行距
	{
		RECT tr;
		GetWindowRect(hTray, &tr);
		int th = tr.bottom - tr.top;
		if (tr.right - tr.left > th)
		{
			int want = (int)(40 * sc);
			if (want > th - (int)(8 * sc))
				want = th - (int)(8 * sc);
			if (want / 2 > wHeight)
				wHeight = want / 2;
			if (wHeight * 2 > th)
				wHeight = th / 2;
		}
	}
	WCHAR a[8], b[8], c[8];
	float lab;
	mWidth = 0;
	mHeight = 0;
	if (TraySave.bMonitorTraffic)
	{
		wTraffic = (int)((mode < 2 ? fSbArrowW + 4 * sc : 0) + fSbNumW + 3 * sc + fSbUnitW + .999f) + wSpace;
		mWidth += wTraffic;
		mHeight += wHeight * 2;
	}
	else
		wTraffic = 0;
	float gv = fSbGlyphW + 3 * sc + 4 * sc;//警告时数值前面多出的图标和底色块边距
	if (TraySave.bMonitorUsage)
	{
		FSbLabel(TraySave.szUsageCPU, a);
		FSbLabel(TraySave.szUsageMEM, b);
		lab = mode == 0 ? FMax(FTextW(a, fSbL, 400, 2), FTextW(b, fSbL, 400, 2)) + 6 * sc : 0;
		float v = mode == 2 ? FMax(FTextW(L"100", fSbV, 600, 2), FTextW(L"88", fSbV, 600, 2) + gv) : FMax(FTextW(L"100%", fSbV, 600, 2), FTextW(L"88%", fSbV, 600, 2) + gv);
		wUsage = (int)(lab + v + .999f) + wSpace;
		mWidth += wUsage;
		mHeight += wHeight * 2;
	}
	else
		wUsage = 0;
	if (TraySave.bMonitorTemperature)
	{
		FSbLabel(TraySave.szTemperatureCPU, a);
		FSbLabel(TraySave.szTemperatureGPU, b);
		FSbLabel(TraySave.szDiskName, c);
		lab = mode == 0 ? FMax(FMax(FTextW(a, fSbL, 400, 2), FTextW(b, fSbL, 400, 2)), TraySave.bMonitorDisk ? FTextW(c, fSbL, 400, 2) : 0) + 6 * sc : 0;
		float v = mode == 2 ? FMax(FTextW(L"100", fSbV, 600, 2), FTextW(L"88", fSbV, 600, 2) + gv) : FMax(FTextW(L"100\x00B0", fSbV, 600, 2), FTextW(L"88\x00B0", fSbV, 600, 2) + gv);
		wTemperature = (int)(lab + v + .999f) + wSpace;
		mWidth += wTemperature;
		mHeight += bRing0 ? wHeight * 2 : wHeight;
	}
	else
		wTemperature = 0;
	if (TraySave.bMonitorDisk)
	{
		FSbLabel(TraySave.szDiskReadSec, a);
		FSbLabel(TraySave.szDiskWriteSec, b);
		lab = mode == 0 ? FMax(FTextW(a, fSbL, 400, 2), FTextW(b, fSbL, 400, 2)) + 4 * sc : mode == 1 ? FTextW(L"W", fSbL, 400, 2) + 4 * sc : 0;
		wDisk = (int)(lab + fSbNumW + 3 * sc + fSbUnitW + .999f) + wSpace;
		if (hOHMA && TraySave.bMonitorTemperature)//硬盘温度和占用（宽度同温度列）
		{
			if (!(TraySave.bMonitorFloatVRow && TraySave.bMonitorFloat))
				wDisk += wTemperature;
			mHeight += wHeight * 2;
		}
		mWidth += wDisk;
		mHeight += wHeight * 2;
	}
	else
		wDisk = 0;
	if (TraySave.bMonitorTime)
	{
		wTime = (int)(FMax(FTextW(L"88:88:88", fSbV, 600, 2), FTextW(L"88/88 周三", fSbL, 400, 2)) + .999f) + wSpace;
		mWidth += wTime;
		mHeight += wHeight * 2;
	}
	else
		wTime = 0;
}
//画状态条。返回 FALSE 时由原来的 GDI 代码画
BOOL FBarPaint(HDC mdc, RECT rc, COLORREF cBg)
{
	if (!fSbOn)
		return FALSE;
	if (!fSbRT)
	{
		D2D1_RENDER_TARGET_PROPERTIES p = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.f, 96.f);
		if (FAILED(fD2D->CreateDCRenderTarget(&p, &fSbRT)))
			return FALSE;
		if (FAILED(fSbRT->CreateSolidColorBrush(FC(0), &fSbBr)))
		{
			fSbRT->Release();
			fSbRT = NULL;
			return FALSE;
		}
		fSbRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	}
	if (FAILED(fSbRT->BindDC(mdc, &rc)))
		return FALSE;
	fRT = fSbRT;
	fBr = fSbBr;
	DWORD bg = FRgb(cBg);
	fSbDark = GetRValue(cBg) * 299 + GetGValue(cBg) * 587 + GetBValue(cBg) * 114 < 128000;
	float sc = iDPI / 96.f, W = (float)(rc.right - rc.left), H = (float)(rc.bottom - rc.top);
	fRT->BeginDraw();
	//0,0,1：Win10 任务栏风格 / 浮动时窗口靠每像素 alpha 透出底下（原来 GDI 画出来 alpha 是 0），这里清成全透明
	fRT->Clear(cBg == RGB(0, 0, 1) ? FC(0, 0) : FC(bg));
	if (fSbHover > 0)
		FFill(0, 0, W, H, fSbDark ? 0xFFFFFF : 0x000000, (fSbDark ? .075f : .055f) * fSbHover, 4 * sc);
	int mode = TraySave.iMonitorSimple;
	BOOL bBit = HIWORD(TraySave.iUnit);
	int minU = LOWORD(TraySave.iUnit);
	float inset = wSpace / 2.f, colX = 0, vy = 0;
	float p = (float)wHeight < fSbPitch ? (float)wHeight : fSbPitch, top = H / 2 - p;
	float x = 0, y = 0, w = 0, h = 0;
	WCHAR sz[24], l[8];
	//取格子：横排按列（两行，或居中一行），竖排一行一格往下排
	auto cell = [&](float colW, int row, int nRows)
	{
		if (VTray)
		{
			x = inset;
			w = W - inset * 2;
			y = vy;
			h = (float)wHeight;
			vy += wHeight;
		}
		else
		{
			x = colX + inset;
			w = colW - inset * 2;
			h = p;
			y = nRows == 1 ? (H - p) / 2 : top + row * p;
		}
	};
	if (TraySave.bMonitorTraffic)
	{
		BOOL ud = TraySave.bMonitorTrafficUpDown;
		for (int r = 0; r < 2; r++)
		{
			BOOL up = (r == 0) == (ud != FALSE);
			ULONG64 v = up ? TrayData->s_out_byte : TrayData->s_in_byte;
			cell((float)wTraffic, r, 2);
			FSbRate(x, y, w, h, up ? 0 : 1, v, v < TraySave.dNumValues[0] ? 0 : v < TraySave.dNumValues[1] ? 1 : 2, bBit, minU);
		}
		colX += wTraffic;
	}
	if (TraySave.bMonitorUsage)
	{
		cell((float)wUsage, 0, 2);
		FSbLabel(TraySave.szUsageCPU, l);
		wsprintf(sz, mode == 2 ? L"%d" : L"%d%%", (int)iCPU);
		FSbMetric(x, y, w, h, l, sz, FLevel((DWORD)iCPU, TraySave.dNumValues[4], TraySave.dNumValues[5]), 1);
		cell((float)wUsage, 1, 2);
		FSbLabel(TraySave.szUsageMEM, l);
		wsprintf(sz, mode == 2 ? L"%d" : L"%d%%", MemoryStatusEx.dwMemoryLoad);
		FSbMetric(x, y, w, h, l, sz, FLevel(MemoryStatusEx.dwMemoryLoad, TraySave.dNumValues[6], TraySave.dNumValues[7]), 1);
		colX += wUsage;
	}
	if (TraySave.bMonitorTemperature)
	{
		BOOL bGpuDll = hATIDLL != NULL || hNVDLL != NULL;
		if (bRing0)//第一行 CPU 温度；有独显但读不到 CPU 温度时换成硬盘占用（同原来）
		{
			cell((float)wTemperature, 0, 2);
			if (bGpuDll && TrayData->iTemperature1 == 0 && TraySave.bMonitorDisk && !hOHMA)
			{
				FSbLabel(TraySave.szDiskName, l);
				wsprintf(sz, mode == 2 ? L"%d" : L"%d%%", TrayData->disktime);
				FSbMetric(x, y, w, h, l, sz, FLevel(TrayData->disktime, TraySave.dNumValues[6], TraySave.dNumValues[7]), 1);
			}
			else
			{
				FSbLabel(TraySave.szTemperatureCPU, l);
				wsprintf(sz, mode == 2 ? L"%d" : L"%d\x00B0", TrayData->iTemperature1);
				FSbMetric(x, y, w, h, l, sz, FLevel(TrayData->iTemperature1, TraySave.dNumValues[2], TraySave.dNumValues[3]), 1);
			}
		}
		cell((float)wTemperature, 1, bRing0 ? 2 : 1);
		if (!bGpuDll && TraySave.bMonitorDisk && !hOHMA)//没有独显时显示硬盘占用（同原来）
		{
			FSbLabel(TraySave.szDiskName, l);
			wsprintf(sz, mode == 2 ? L"%d" : L"%d%%", TrayData->disktime);
			FSbMetric(x, y, w, h, l, sz, FLevel(TrayData->disktime, TraySave.dNumValues[6], TraySave.dNumValues[7]), 1);
		}
		else
		{
			FSbLabel(TraySave.szTemperatureGPU, l);
			if (bNvGpuSleeping)//独显休眠，不去唤醒它
				lstrcpy(sz, L"--");
			else
				wsprintf(sz, mode == 2 ? L"%d" : L"%d\x00B0", TrayData->iTemperature2);
			FSbMetric(x, y, w, h, l, sz, bNvGpuSleeping ? 0 : FLevel(TrayData->iTemperature2, TraySave.dNumValues[2], TraySave.dNumValues[3]), 1);
		}
		colX += wTemperature;
	}
	if (TraySave.bMonitorDisk)
	{
		float rw = (float)wDisk;
		if (hOHMA && TraySave.bMonitorTemperature)//硬盘温度、硬盘占用
		{
			FSbLabel(TraySave.szDiskName, l);
			cell((float)wTemperature, 0, 2);
			wsprintf(sz, mode == 2 ? L"%d" : L"%d\x00B0", TrayData->iHddTemperature);
			FSbMetric(x, y, w, h, l, sz, FLevel(TrayData->iHddTemperature, TraySave.dNumValues[2], TraySave.dNumValues[3]), 1);
			cell((float)wTemperature, 1, 2);
			wsprintf(sz, mode == 2 ? L"%d" : L"%d%%", TrayData->disktime);
			FSbMetric(x, y, w, h, l, sz, FLevel(TrayData->disktime, TraySave.dNumValues[6], TraySave.dNumValues[7]), 1);
			if (!VTray)
			{
				colX += wTemperature;
				rw -= wTemperature;
			}
		}
		BOOL ud = TraySave.bMonitorTrafficUpDown;
		for (int r = 0; r < 2; r++)
		{
			BOOL rd = (r == 0) == (ud != FALSE);
			double v = rd ? TrayData->diskreadbyte : TrayData->diskwritebyte;
			double mb = v / 1048576;
			cell(rw, r, 2);
			FSbRate(x, y, w, h, rd ? 2 : 3, (ULONG64)v, mb < TraySave.dNumValues2[0] ? 0 : mb < TraySave.dNumValues2[1] ? 1 : 2, FALSE, 2);
		}
		colX += rw;
	}
	if (TraySave.bMonitorTime)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		const WCHAR* wk[7] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };
		cell((float)wTime, 0, 2);
		wsprintf(sz, L"%d/%d 周%s", st.wMonth, st.wDay, wk[st.wDayOfWeek]);
		FSbText(sz, x, y, w, h, fSbL, 400, FSbT2(), 1);
		cell((float)wTime, 1, 2);
		wsprintf(sz, L"%.2d:%.2d:%.2d", st.wHour, st.wMinute, st.wSecond);
		FSbText(sz, x, y, w, h, fSbV, 600, FSbT1(), 1);
	}
	HRESULT hr = fRT->EndDraw();
	fRT = NULL;
	fBr = NULL;
	if (FAILED(hr))
	{
		fSbBr->Release();
		fSbRT->Release();
		fSbBr = NULL;
		fSbRT = NULL;
		return FALSE;
	}
	return TRUE;
}
//悬停高亮：光标在状态条上、或悬停窗口开着时亮起，渐变 100ms 亮、160ms 灭
void FBarHoverUpdate()
{
	if (!IsWindow(hTaskBar))
		return;
	POINT pt;
	GetCursorPos(&pt);
	RECT r;
	GetWindowRect(hTaskBar, &r);
	int to = PtInRect(&r, pt) || (IsWindowVisible(hTaskTips) && fly.anim != 2);
	if (to != fSbHoverTo)
	{
		fSbHoverTo = to;
		fSbHoverT = GetTickCount64();
		SetTimer(hTaskBar, 7, 15, NULL);
	}
}
void FBarHoverStep()//状态条定时器 7
{
	ULONGLONG t = GetTickCount64();
	float step = (t - fSbHoverT) / (fSbHoverTo ? 100.f : 160.f);
	fSbHoverT = t;
	fSbHover += fSbHoverTo ? step : -step;
	if (fSbHover >= 1 || fSbHover <= 0)
	{
		fSbHover = fSbHover >= 1 ? 1.f : 0.f;
		KillTimer(hTaskBar, 7);
	}
	InvalidateRect(hTaskBar, NULL, TRUE);
}
//Win11 上任务栏子窗口收不到鼠标消息，100ms 轮询一次光标：停留 300ms 打开悬停窗口（同 Win10 的 TrackMouseEvent）
void FBarPoll()//状态条定时器 8
{
	FBarHoverUpdate();
	POINT pt;
	GetCursorPos(&pt);
	RECT r;
	GetWindowRect(hTaskBar, &r);
	if (PtInRect(&r, pt) && !KEYDOWN(VK_LBUTTON) && !bFullScreen)
	{
		fSbDwell += 100;
		if (fSbDwell >= 300 && (!IsWindowVisible(hTaskTips) || fly.anim == 2))
			SendMessage(hTaskBar, WM_MOUSEHOVER, 0, 0);
	}
	else
		fSbDwell = 0;
}
