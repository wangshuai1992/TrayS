// FluentSet.h —— 方案 A 设置窗口：左侧导航分页，右侧卡片
// 沿用 IDD_SETTING 里原来的控件和 SettingProc 的读写逻辑，这里只改布局和绘制：
// 复选框画成开关、单选按钮画成分段按钮、编辑框去掉边框由窗口画 Win11 输入框，都用 NM_CUSTOMDRAW，选中状态仍在原控件里
// 只被 TrayS.cpp 包含，放在 Fluent.h 之后
#pragma once

enum { FR_HEAD = 1, FR_TOGGLE, FR_SEG, FR_SEG3, FR_BTN, FR_SWATCH, FR_SLIDER, FR_EDIT, FR_GRID2, FR_EDIT5, FR_NOTE, FR_LINK, FR_ABOUT };
struct FROW { BYTE page; BYTE kind; LPCWSTR icon; LPCWSTR title; LPCWSTR desc; int id[5]; };
#define FSET_PAGES 9
const WCHAR* fSetPageName[FSET_PAGES] = { L"状态条", L"位置", L"悬停窗口", L"任务栏外观", L"预警与提醒", L"网络与硬盘", L"文字与字体", L"常规", L"关于" };
const WCHAR fSetPageIcon[FSET_PAGES] = { 0xE9D2, 0xE718, 0xE7C4, 0xE790, 0xEA8F, 0xE968, 0xE8D2, 0xE713, 0xE946 };
WCHAR fSetFontDesc[64];//“状态条字体”一行的说明，绘制时更新
const FROW fSetRows[] = {
	{ 0, FR_HEAD, 0, L"显示内容" },
	{ 0, FR_TOGGLE, L"\xE7F4", L"在任务栏上显示状态条", L"关闭后只保留托盘图标", { IDC_CHECK_MONITOR } },
	{ 0, FR_TOGGLE, L"\xE839", L"网速", L"上传和下载速度", { IDC_CHECK_TRAFFIC } },
	{ 0, FR_TOGGLE, L"\xE950", L"CPU 和内存占用", NULL, { IDC_CHECK_USAGE } },
	{ 0, FR_TOGGLE, 0, L"准确占用", L"用性能计数器计算，数值和任务管理器一致", { IDC_CHECK_MONITOR_PDH } },
	{ 0, FR_TOGGLE, L"\xE9CA", L"温度", L"CPU 和显卡温度；显卡休眠时显示 --，不会唤醒独显", { IDC_CHECK_TEMPERATURE } },
	{ 0, FR_TOGGLE, L"\xEDA2", L"硬盘读写", NULL, { IDC_CHECK_DISK } },
	{ 0, FR_TOGGLE, L"\xE823", L"星期和时间", NULL, { IDC_CHECK_MONITOR_TIME } },
	{ 0, FR_HEAD, 0, L"样式" },
	{ 0, FR_SEG3, L"\xE8A9", L"显示方式", L"简约：去掉文字标签；极简：只保留数字", { IDC_CHECK_MONITOR_SIMPLE } },
	{ 0, FR_TOGGLE, L"\xE8CB", L"上传显示在上面", NULL, { IDC_CHECK_MONITOR_UPDOWN } },

	{ 1, FR_HEAD, 0, L"在任务栏上" },
	{ 1, FR_TOGGLE, 0, L"放在任务栏左侧", L"竖放的任务栏放在顶部", { IDC_CHECK_MONITOR_LEFT } },
	{ 1, FR_TOGGLE, 0, L"靠近托盘图标", NULL, { IDC_CHECK_MONITOR_NEAR } },
	{ 1, FR_TOGGLE, 0, L"保持置顶", L"被其它窗口挡住时重新放到最上层", { IDC_CHECK_TOPMOST } },
	{ 1, FR_HEAD, 0, L"浮动" },
	{ 1, FR_TOGGLE, 0, L"浮动显示", L"不放在任务栏里，可以拖到屏幕任意位置", { IDC_CHECK_MONITOR_FLOAT } },
	{ 1, FR_TOGGLE, 0, L"浮动时竖排", NULL, { IDC_CHECK_MONITOR_FLOAT_VROW } },
	{ 1, FR_HEAD, 0, L"外观" },
	{ 1, FR_TOGGLE, 0, L"鼠标穿透", L"状态条不响应鼠标", { IDC_CHECK_TRANSPARENT } },
	{ 1, FR_TOGGLE, 0, L"背景融合", L"文字半透明，和背景融在一起", { IDC_CHECK_FUSE } },
	{ 1, FR_SWATCH, 0, L"背景颜色", L"默认跟随任务栏", { IDC_BUTTON_COLOR_BACKGROUND } },

	{ 2, FR_TOGGLE, L"\xE7C4", L"鼠标停在状态条上时显示详细信息", L"CPU、内存、温度、磁盘、网络曲线和占用最高的进程", { IDC_CHECK_TIPS } },
	{ 2, FR_NOTE, 0, NULL, L"停留 0.3 秒打开，移开后自动关闭。点各块可以打开任务管理器、资源监视器、网络连接；鼠标放在进程行上可以打开所在文件夹或结束进程。" },

	{ 3, FR_TOGGLE, L"\xE790", L"任务栏风格", L"修改系统任务栏的透明效果和颜色", { IDC_CHECK_TRAY_STYLE } },
	{ 3, FR_HEAD, 0, L"效果" },
	{ 3, FR_SEG, 0, L"设置哪种情况", L"没有、有窗口最大化时分开设置", { IDC_RADIO_NORMAL, IDC_RADIO_MAXIMIZE } },
	{ 3, FR_SEG, 0, L"效果", NULL, { IDC_RADIO_DEFAULT, IDC_RADIO_TRANSPARENT, IDC_RADIO_BLURBEHIND, IDC_RADIO_ACRYLIC } },
	{ 3, FR_SWATCH, 0, L"颜色", NULL, { IDC_BUTTON_COLOR } },
	{ 3, FR_SLIDER, 0, L"颜色浓度", NULL, { IDC_SLIDER_ALPHA_B } },
	{ 3, FR_SLIDER, 0, L"任务栏不透明度", NULL, { IDC_SLIDER_ALPHA } },
	{ 3, FR_HEAD, 0, L"任务栏图标" },
	{ 3, FR_SEG, 0, L"图标位置", L"Win11 上不可用", { IDC_RADIO_LEFT, IDC_RADIO_CENTER, IDC_RADIO_RIGHT } },
	{ 3, FR_NOTE, 0, NULL, L"Win10 全部可用；Win11 不支持玻璃和亚克力；Win8 只支持玻璃；Win7 不支持任务栏风格。" },

	{ 4, FR_HEAD, 0, L"阈值", L"超过预警值数字加琥珀色底，超过警告值加红底和警示图标", { 1 } },
	{ 4, FR_GRID2, 0, L"网速", L"MB/s", { IDC_EDIT1, IDC_EDIT2 } },
	{ 4, FR_GRID2, 0, L"温度", L"°C", { IDC_EDIT3, IDC_EDIT4 } },
	{ 4, FR_GRID2, 0, L"CPU 占用", L"%", { IDC_EDIT5, IDC_EDIT6 } },
	{ 4, FR_GRID2, 0, L"内存占用", L"%", { IDC_EDIT7, IDC_EDIT8 } },
	{ 4, FR_GRID2, 0, L"硬盘读写", L"MB/s", { IDC_EDIT24, IDC_EDIT25 } },
	{ 4, FR_HEAD, 0, L"声音提醒" },
	{ 4, FR_TOGGLE, 0, L"超过下面的值时响铃", L"填 0 表示这一项不响铃；网速和硬盘的单位是 MB/s", { IDC_CHECK_SOUND } },
	{ 4, FR_EDIT5, 0, NULL, NULL, { IDC_EDIT9, IDC_EDIT10, IDC_EDIT11, IDC_EDIT12, IDC_EDIT26 } },
	{ 4, FR_HEAD, 0, L"数字颜色" },
	{ 4, FR_SWATCH, 0, L"网速和硬盘", L"正常 / 预警 / 警告；保持默认时按新样式显示", { IDC_BUTTON_COLOR_TRAFFIC_LOW, IDC_BUTTON_COLOR_TRAFFIC_MEDIUM, IDC_BUTTON_COLOR_TRAFFIC_HIGH } },
	{ 4, FR_SWATCH, 0, L"占用和温度", NULL, { IDC_BUTTON_COLOR_LOW, IDC_BUTTON_COLOR_MEDUIM, IDC_BUTTON_COLOR_HIGH } },

	{ 5, FR_BTN, L"\xE839", L"网卡", L"状态条和悬停窗口显示哪块网卡的速度", { IDC_BUTTON_SELECT_NET } },
	{ 5, FR_BTN, L"\xEDA2", L"硬盘", L"硬盘读写显示哪块硬盘", { IDC_BUTTON_SELECT_DISK } },
	{ 5, FR_SEG, 0, L"速率单位", NULL, { IDC_RADIO_BYTE, IDC_RADIO_BIT } },
	{ 5, FR_SEG, 0, L"最小单位", L"例如选 MB 时，网速再小也按 MB/s 显示", { IDC_RADIO_AUTO, IDC_RADIO_KB, IDC_RADIO_MB } },

	{ 6, FR_BTN, L"\xE8D2", L"状态条字体", fSetFontDesc, { IDC_BUTTON_FONT } },
	{ 6, FR_HEAD, 0, L"标签文字", L"去掉结尾的冒号后作为标签显示；网速用箭头表示，单位固定为 % 和 °" },
	{ 6, FR_EDIT, 0, L"上传 / 下载", NULL, { IDC_EDIT14, IDC_EDIT15 } },
	{ 6, FR_EDIT, 0, L"CPU 温度 / 单位", NULL, { IDC_EDIT16, IDC_EDIT18 } },
	{ 6, FR_EDIT, 0, L"显卡温度 / 单位", NULL, { IDC_EDIT17, IDC_EDIT19 } },
	{ 6, FR_EDIT, 0, L"CPU 占用 / 单位", NULL, { IDC_EDIT20, IDC_EDIT22 } },
	{ 6, FR_EDIT, 0, L"内存占用 / 单位", NULL, { IDC_EDIT21, IDC_EDIT23 } },
	{ 6, FR_EDIT, 0, L"硬盘读 / 写 / 名称", NULL, { IDC_EDIT27, IDC_EDIT28, IDC_EDIT29 } },

	{ 7, FR_TOGGLE, 0, L"开机启动", NULL, { IDC_CHECK_AUTORUN } },
	{ 7, FR_TOGGLE, 0, L"托盘图标", L"在通知区域显示 TrayS 图标", { IDC_CHECK_TRAYICON } },
	{ 7, FR_TOGGLE, 0, L"系统时钟显示秒和星期", NULL, { IDC_CHECK_TIME } },
	{ 7, FR_EDIT, 0, L"刷新间隔（毫秒）", L"检查状态条位置、刷新任务栏效果的间隔", { IDC_EDIT_TIME } },
	{ 7, FR_HEAD, 0, L"其它" },
	{ 7, FR_BTN, 0, L"恢复默认设置", L"删除保存的设置，TrayS 会退出，下次启动时用默认值", { IDC_RESTORE_DEFAULT } },
	{ 7, FR_BTN, 0, L"退出 TrayS", NULL, { IDC_CLOSE } },

	{ 8, FR_ABOUT },
	{ 8, FR_LINK, 0, L"项目主页", L"原作者 cgbsmy", { IDC_SYSLINK } },
	{ 8, FR_LINK, 0, L"发布帖", NULL, { IDC_SYSLINK_52PJ } },
};
#define FSET_NROWS (sizeof(fSetRows) / sizeof(fSetRows[0]))

//要画的东西（布局时算好，WM_PAINT 里画）
enum { PI_CARD = 1, PI_SEP, PI_TEXT, PI_ICON, PI_FRAME, PI_TRACK, PI_LOGO };
struct FPI { BYTE kind; BYTE role; WORD id; float x, y, w, h; float size; int weight; int align; LPCWSTR s; };
struct FSET
{
	HWND h;
	int page, hoverNav;
	float sc;
	BOOL dark;
	HFONT font;
	HBRUSH brCard, brTrack, brInput, brInputFocus;
	ID2D1DCRenderTarget* rt;
	ID2D1SolidColorBrush* br;
	FPI pi[160];
	int npi;
	WNDPROC seg3Proc;
	WNDPROC sliderProc;
	BOOL bTrackLeave;
} fs;
//颜色（0xRRGGBB）
inline DWORD FSBg() { return fs.dark ? 0x202020 : 0xF3F3F3; }
inline DWORD FSCard() { return fs.dark ? 0x2B2B2B : 0xFBFBFB; }
inline DWORD FSTrack() { return fs.dark ? 0x363636 : 0xEDEDED; }
inline DWORD FSInput() { return fs.dark ? 0x323232 : 0xFFFFFF; }
inline DWORD FSInputFocus() { return fs.dark ? 0x1F1F1F : 0xFFFFFF; }
inline DWORD FST1() { return fs.dark ? 0xFFFFFF : 0x1A1A1A; }
inline DWORD FST2() { return fs.dark ? 0xC5C5C5 : 0x5D5D5D; }
inline DWORD FSAccent() { return fs.dark ? fAccentD : fAccentL; }
inline COLORREF FSRef(DWORD rgb) { return RGB((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255); }
#define FS(v) ((v) * fs.sc)

void FSetAdd(BYTE kind, BYTE role, int id, float x, float y, float w, float h, LPCWSTR s = NULL, float size = 0, int weight = 400, int align = 0)
{
	if (fs.npi >= 160)
		return;
	FPI& p = fs.pi[fs.npi++];
	p.kind = kind;
	p.role = role;
	p.id = (WORD)id;
	p.x = x;
	p.y = y;
	p.w = w;
	p.h = h;
	p.s = s;
	p.size = size;
	p.weight = weight;
	p.align = align;
}
BOOL FSetOnPage(int id)//控件在当前页上？
{
	for (int i = 0; i < (int)FSET_NROWS; i++)
		if (fSetRows[i].page == fs.page)
			for (int k = 0; k < 5; k++)
				if (fSetRows[i].id[k] == id && fSetRows[i].kind != FR_HEAD)
					return TRUE;
	return FALSE;
}
void FSetPlace(int id, float x, float y, float w, float h)
{
	HWND c = GetDlgItem(fs.h, id);
	if (c)
		SetWindowPos(c, NULL, (int)(x + .5f), (int)(y + .5f), (int)(w + .5f), (int)(h + .5f), SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}
float FSetTextW(LPCWSTR s, float size, int weight) { return FTextW(s, FS(size), weight); }
//分段按钮每段的宽度
float FSetSegW(int id)
{
	WCHAR t[32];
	GetDlgItemText(fs.h, id, t, 32);
	return FSetTextW(t, 14, 600) + FS(28);
}
//排版当前页：放控件、记下要画的卡片和文字
#define FSET_W 860.f
#define FSET_H 670.f
#define FSET_NAV 268.f
void FSetLayout()
{
	fs.npi = 0;
	RECT rc;
	GetClientRect(fs.h, &rc);
	float W = (float)rc.right;
	float cx = FS(FSET_NAV), cw = W - cx - FS(28);
	//隐藏不在这一页的控件
	for (HWND c = GetWindow(fs.h, GW_CHILD); c; c = GetWindow(c, GW_HWNDNEXT))
	{
		int id = GetDlgCtrlID(c);
		if (!FSetOnPage(id))
			ShowWindow(c, SW_HIDE);
	}
	FSetAdd(PI_TEXT, 1, 0, cx + FS(4), FS(18), cw, FS(40), fSetPageName[fs.page], FS(26), 600);
	float y = FS(76);
	float cardY = -1;
	int nInCard = 0;
	for (int i = 0; i <= (int)FSET_NROWS; i++)
	{
		const FROW* r = i < (int)FSET_NROWS ? &fSetRows[i] : NULL;
		if (r && r->page != fs.page)
			continue;
		BOOL endCard = !r || r->kind == FR_HEAD || r->kind == FR_NOTE || r->kind == FR_ABOUT;
		if (endCard && cardY >= 0)//收尾上一张卡片
		{
			FSetAdd(PI_CARD, 0, 0, cx, cardY, cw, y - cardY);
			cardY = -1;
			y += FS(8);
		}
		if (!r)
			break;
		if (r->kind == FR_HEAD)
		{
			y += FS(8);
			FSetAdd(PI_TEXT, 1, 0, cx + FS(2), y, cw, FS(22), r->title, FS(14), 600);
			if (r->desc)
			{
				FSetAdd(PI_TEXT, 2, 0, cx + FS(2), y + FS(22), cw, FS(18), r->desc, FS(12));
				y += FS(18);
			}
			y += FS(26);
			if (r->id[0] == 1)//阈值表的列标题
			{
				float ex = cx + cw - FS(16) - FS(64);
				FSetAdd(PI_TEXT, 2, 0, ex - FS(64) - FS(12), y - FS(20), FS(64), FS(18), L"预警", FS(12), 400, 1);
				FSetAdd(PI_TEXT, 2, 0, ex, y - FS(20), FS(64), FS(18), L"警告", FS(12), 400, 1);
			}
			continue;
		}
		if (r->kind == FR_NOTE)
		{
			FSetAdd(PI_TEXT, 3, 0, cx + FS(4), y + FS(2), cw - FS(8), FS(40), r->desc, FS(12));
			y += FS(46);
			continue;
		}
		if (r->kind == FR_ABOUT)
		{
			FSetAdd(PI_CARD, 0, 0, cx, y, cw, FS(96));
			FSetAdd(PI_LOGO, 0, 0, cx + FS(20), y + FS(20), FS(56), FS(56));
			FSetAdd(PI_TEXT, 1, 0, cx + FS(92), y + FS(22), cw, FS(28), L"TrayS", FS(20), 600);
			FSetAdd(PI_TEXT, 2, 0, cx + FS(92), y + FS(52), cw, FS(20), L"任务栏系统监控：网速、CPU、内存、温度、硬盘", FS(12));
			y += FS(96 + 8);
			continue;
		}
		if (cardY < 0)
		{
			cardY = y;
			nInCard = 0;
		}
		else
			FSetAdd(PI_SEP, 0, 0, cx + FS(1), y, cw - FS(2), FS(1));
		nInCard++;
		float rh = r->kind == FR_GRID2 ? FS(42) : r->kind == FR_EDIT5 ? FS(52) : r->desc ? FS(56) : FS(44);
		float tx = cx + FS(16);
		if (r->icon)
		{
			FSetAdd(PI_ICON, 0, 0, tx, y, FS(20), rh, r->icon, FS(17));
			tx += FS(36);
		}
		if (r->title)
		{
			if (r->desc && r->kind != FR_GRID2)
			{
				FSetAdd(PI_TEXT, 1, 0, tx, y + rh / 2 - FS(19), cw, FS(20), r->title, FS(14));
				FSetAdd(PI_TEXT, 2, 0, tx, y + rh / 2 + FS(1), cw - (tx - cx) - FS(r->kind == FR_TOGGLE ? 130.f : 230.f), FS(18), r->desc, FS(12));
			}
			else
				FSetAdd(PI_TEXT, 1, 0, tx, y, cw, rh, r->title, FS(14));
		}
		float right = cx + cw - FS(16);
		float mid = y + rh / 2;
		switch (r->kind)
		{
		case FR_TOGGLE:
			FSetPlace(r->id[0], right - FS(96), mid - FS(16), FS(96), FS(32));
			break;
		case FR_SEG:
		{
			int n = 0;
			float tw = 0;
			while (n < 5 && r->id[n])
				tw += FSetSegW(r->id[n++]);
			float x = right - tw - FS(4);
			FSetAdd(PI_TRACK, 0, 0, x - FS(2), mid - FS(17), tw + FS(4), FS(34));
			for (int k = 0; k < n; k++)
			{
				float sw = FSetSegW(r->id[k]);
				FSetPlace(r->id[k], x, mid - FS(15), sw, FS(30));
				x += sw;
			}
			break;
		}
		case FR_SEG3:
		{
			float tw = FSetTextW(L"完整", 14, 600) * 3 + FS(28 * 3);
			FSetAdd(PI_TRACK, 0, 0, right - tw - FS(2), mid - FS(17), tw + FS(4), FS(34));
			FSetPlace(r->id[0], right - tw, mid - FS(15), tw, FS(30));
			break;
		}
		case FR_BTN:
		case FR_LINK:
		{
			WCHAR t[48];
			GetDlgItemText(fs.h, r->id[0], t, 48);
			float bw = r->kind == FR_LINK ? FSetTextW(t, 14, 400) + FS(8) : FMax(FSetTextW(t, 14, 400) + FS(40), FS(104));
			if (r->kind == FR_LINK)
			{
				int n = lstrlen(t);//SysLink 的文字带 <a></a>，量宽度时去掉
				bw = FSetTextW(t, 14, 400) - (n > 7 ? FSetTextW(L"<a></a>", 14, 400) : 0) + FS(12);
			}
			FSetPlace(r->id[0], right - bw, mid - FS(16), bw, FS(32));
			break;
		}
		case FR_SWATCH:
		{
			int n = 0;
			while (n < 5 && r->id[n])
				n++;
			float x = right - n * FS(64) - (n - 1) * FS(8);
			for (int k = 0; k < n; k++, x += FS(72))
				FSetPlace(r->id[k], x, mid - FS(15), FS(64), FS(30));
			break;
		}
		case FR_SLIDER:
			FSetPlace(r->id[0], right - FS(240), mid - FS(16), FS(240), FS(32));
			break;
		case FR_GRID2:
		case FR_EDIT:
		{
			int n = 0;
			while (n < 5 && r->id[n])
				n++;
			float ew = r->kind == FR_GRID2 ? FS(64) : FS(88);
			float x = right - n * ew - (n - 1) * FS(12);
			if (r->kind == FR_GRID2 && r->desc)//单位写在两列前面
				FSetAdd(PI_TEXT, 2, 0, x - FS(80), y, FS(68), rh, r->desc, FS(12), 400, 2);
			for (int k = 0; k < n; k++, x += ew + FS(12))
			{
				FSetAdd(PI_FRAME, 0, r->id[k], x, mid - FS(16), ew, FS(32));
				float eh = FS(20);
				FSetPlace(r->id[k], x + FS(10), mid - eh / 2, ew - FS(20), eh);
			}
			break;
		}
		case FR_EDIT5:
		{
			const WCHAR* lb[5] = { L"网速", L"温度", L"CPU", L"内存", L"硬盘" };
			float gw = (cw - FS(32)) / 5, x = cx + FS(16);
			for (int k = 0; k < 5; k++, x += gw)
			{
				FSetAdd(PI_TEXT, 2, 0, x, y, FS(36), rh, lb[k], FS(12));
				float ex = x + FS(36), ew = gw - FS(36) - FS(14);
				FSetAdd(PI_FRAME, 0, r->id[k], ex, mid - FS(16), ew, FS(32));
				FSetPlace(r->id[k], ex + FS(8), mid - FS(10), ew - FS(16), FS(20));
			}
			break;
		}
		}
		y += rh;
	}
}
//绘制目标：绑到给定 DC 的一块区域上，坐标从区域左上角算
BOOL FSetBegin(HDC hdc, const RECT* rc, DWORD bg)
{
	if (!fs.rt)
	{
		D2D1_RENDER_TARGET_PROPERTIES p = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.f, 96.f);
		if (FAILED(fD2D->CreateDCRenderTarget(&p, &fs.rt)))
			return FALSE;
		if (FAILED(fs.rt->CreateSolidColorBrush(FC(0), &fs.br)))
		{
			fs.rt->Release();
			fs.rt = NULL;
			return FALSE;
		}
	}
	if (FAILED(fs.rt->BindDC(hdc, rc)))
		return FALSE;
	fRT = fs.rt;
	fBr = fs.br;
	fRT->BeginDraw();
	fRT->Clear(FC(bg));
	return TRUE;
}
void FSetEnd()
{
	if (FAILED(fRT->EndDraw()))
	{
		fs.br->Release();
		fs.rt->Release();
		fs.br = NULL;
		fs.rt = NULL;
	}
	fRT = NULL;
	fBr = NULL;
}
void FSetLogo(float x, float y, float s)
{
	FFill(x, y, s, s, fAccentL, 1, s * .25f);
	float u = s / 16;
	FFill(x + 4 * u, y + 9 * u, 2 * u, 4 * u, 0xFFFFFF, 1, u);
	FFill(x + 7 * u, y + 5 * u, 2 * u, 8 * u, 0xFFFFFF, 1, u);
	FFill(x + 10 * u, y + 7 * u, 2 * u, 6 * u, 0xFFFFFF, 1, u);
}
//导航项的位置
void FSetNavRect(int i, float* y, float* h) { *y = FS(96 + i * 40.f); *h = FS(36); }
void FSetPaint(HDC hdc)
{
	RECT rc;
	GetClientRect(fs.h, &rc);
	BOOL bDefFont = lstrcmpi(TraySave.TraybarFont.lfFaceName, L"微软雅黑") == 0 || lstrcmpi(TraySave.TraybarFont.lfFaceName, L"Microsoft YaHei") == 0;
	int nFont = TraySave.TraybarFontSize < 0 ? -TraySave.TraybarFontSize : TraySave.TraybarFontSize;
	wsprintf(fSetFontDesc, L"%s · %d 像素", bDefFont ? L"默认（Segoe UI Variable + 微软雅黑 UI）" : TraySave.TraybarFont.lfFaceName, nFont);
	if (!FSetBegin(hdc, &rc, FSBg()))
		return;
	//导航
	FSetLogo(FS(24), FS(24), FS(44));
	FText(L"TrayS", FS(80), FS(24), FS(180), FS(24), FS(16), 600, FST1());
	FText(L"设置", FS(80), FS(48), FS(180), FS(20), FS(12), 400, FST2());
	for (int i = 0; i < FSET_PAGES; i++)
	{
		float ny, nh;
		FSetNavRect(i, &ny, &nh);
		if (i == fs.page || i == fs.hoverNav)
			FFill(FS(12), ny, FS(FSET_NAV - 28), nh, fs.dark ? 0xFFFFFF : 0x000000, i == fs.page ? (fs.dark ? .065f : .055f) : (fs.dark ? .04f : .035f), FS(5));
		if (i == fs.page)
			FFill(FS(12), ny + nh / 2 - FS(8), FS(3), FS(16), FSAccent(), 1, FS(1.5f));
		WCHAR g[2] = { fSetPageIcon[i], 0 };
		FText(g, FS(28), ny, FS(20), nh, FS(16), 400, FST1(), 1, 0, 1);
		FText(fSetPageName[i], FS(60), ny, FS(190), nh, FS(14), 400, FST1());
	}
	//当前页：先画卡片底，再画上面的文字
	for (int pass = 0; pass < 2; pass++)
	for (int i = 0; i < fs.npi; i++)
	{
		FPI& p = fs.pi[i];
		if ((p.kind == PI_CARD) != (pass == 0))
			continue;
		switch (p.kind)
		{
		case PI_CARD:
			FFill(p.x, p.y, p.w, p.h, FSCard(), 1, FS(6));
			FStroke(p.x, p.y, p.w, p.h, fs.dark ? 0x000000 : 0x000000, fs.dark ? .22f : .07f, FS(6));
			break;
		case PI_SEP:
			FFill(p.x, p.y, p.w, FS(1) < 1 ? 1 : FS(1), fs.dark ? 0x000000 : 0x000000, fs.dark ? .22f : .06f);
			break;
		case PI_TEXT:
			if (p.role == 3)//说明文字，可以换行
			{
				IDWriteTextFormat* tf = FFormat(p.size, 400, 0);
				if (tf)
				{
					tf->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
					tf->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
					tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
					fBr->SetColor(FC(FST2()));
					fRT->DrawText(p.s, lstrlen(p.s), tf, FR(p.x, p.y, p.w, p.h), fBr);
					tf->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
					tf->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
				}
			}
			else
				FText(p.s, p.x, p.y, p.w, p.h, p.size, p.weight, p.role == 1 ? FST1() : FST2(), 1, p.align);
			break;
		case PI_ICON:
			FText(p.s, p.x, p.y, p.w, p.h, p.size, 400, FST1(), 1, 1, 1);
			break;
		case PI_TRACK:
			FFill(p.x, p.y, p.w, p.h, FSTrack(), 1, FS(5));
			break;
		case PI_FRAME:
		{
			BOOL focus = GetFocus() == GetDlgItem(fs.h, p.id);
			FFill(p.x, p.y, p.w, p.h, focus ? FSInputFocus() : FSInput(), 1, FS(4));
			FStroke(p.x, p.y, p.w, p.h, fs.dark ? 0xFFFFFF : 0x000000, fs.dark ? .08f : .07f, FS(4));
			//底边：平时一条细线，获得焦点时 2 像素强调色
			fRT->PushAxisAlignedClip(FR(p.x, p.y + p.h - FS(2), p.w, FS(2)), D2D1_ANTIALIAS_MODE_ALIASED);
			if (focus)
				FFill(p.x, p.y + p.h - FS(4), p.w, FS(4), FSAccent(), 1, FS(4));
			else
				FFill(p.x, p.y + p.h - FS(1), p.w, FS(1), fs.dark ? 0xFFFFFF : 0x000000, fs.dark ? .45f : .40f);
			fRT->PopAxisAlignedClip();
			break;
		}
		case PI_LOGO:
			FSetLogo(p.x, p.y, p.w);
			break;
		}
	}
	FSetEnd();
}
//开关：左边“开/关”，右边 40×20 的拨动开关
void FSetDrawToggle(HWND c, HDC hdc, RECT rc, UINT st)
{
	if (!FSetBegin(hdc, &rc, FSCard()))
		return;
	float w = (float)(rc.right - rc.left), h = (float)(rc.bottom - rc.top);
	BOOL on = SendMessage(c, BM_GETCHECK, 0, 0) == BST_CHECKED;
	BOOL hot = (st & CDIS_HOT) != 0, dis = (st & CDIS_DISABLED) != 0;
	float a = dis ? .4f : 1.f;
	float sw = FS(40), sh = FS(20), sx = w - sw - FS(1), sy = (h - sh) / 2;
	if (on)
	{
		FFill(sx, sy, sw, sh, FSAccent(), a, sh / 2);
		float r = hot ? FS(7) : FS(6);
		FFill(sx + sw - FS(10) - r, sy + sh / 2 - r, r * 2, r * 2, fs.dark ? 0x000000 : 0xFFFFFF, a, r);
	}
	else
	{
		FFill(sx, sy, sw, sh, fs.dark ? 0xFFFFFF : 0x000000, hot ? .06f : .02f, sh / 2);
		FStroke(sx, sy, sw, sh, fs.dark ? 0xCFCFCF : 0x6E6E6E, a, sh / 2);
		float r = hot ? FS(7) : FS(5);
		FFill(sx + FS(10) - r, sy + sh / 2 - r, r * 2, r * 2, fs.dark ? 0xCFCFCF : 0x5D5D5D, a, r);
	}
	if ((st & CDIS_FOCUS) && (st & CDIS_SHOWKEYBOARDCUES))//只在用键盘操作时显示焦点框
		FStroke(sx - FS(3), sy - FS(3), sw + FS(6), sh + FS(6), FST1(), .9f, sh / 2 + FS(3), FS(1.5f));
	FText(on ? L"开" : L"关", 0, 0, sx - FS(12), h, FS(14), 400, FST1(), a, 2);
	FSetEnd();
}
//分段按钮的一段（单选按钮）；sel 选中，seg3 时画三段
void FSetDrawSegPart(float x, float y, float w, float h, LPCWSTR t, BOOL sel, BOOL hot, BOOL dis)
{
	float a = dis ? .4f : 1.f;
	if (sel)
	{
		FFill(x + FS(1), y + FS(1), w - FS(2), h - FS(2), fs.dark ? 0x4A4A4A : 0xFFFFFF, a, FS(4));
		FStroke(x + FS(1), y + FS(1), w - FS(2), h - FS(2), 0x000000, fs.dark ? .25f : .08f, FS(4));
	}
	else if (hot && !dis)
		FFill(x + FS(1), y + FS(1), w - FS(2), h - FS(2), fs.dark ? 0xFFFFFF : 0x000000, .04f, FS(4));
	FText(t, x, y, w, h, FS(14), sel ? 600 : 400, sel ? FST1() : FST2(), a, 1);
}
void FSetDrawRadio(HWND c, HDC hdc, RECT rc, UINT st)
{
	if (!FSetBegin(hdc, &rc, FSTrack()))
		return;
	WCHAR t[32];
	GetWindowText(c, t, 32);
	FSetDrawSegPart(0, 0, (float)(rc.right - rc.left), (float)(rc.bottom - rc.top), t,
		SendMessage(c, BM_GETCHECK, 0, 0) == BST_CHECKED, (st & CDIS_HOT) != 0, (st & CDIS_DISABLED) != 0);
	FSetEnd();
}
POINT fSeg3Hot = { -1, -1 };
void FSetDrawSeg3(HWND c, HDC hdc, RECT rc, UINT st)
{
	if (!FSetBegin(hdc, &rc, FSTrack()))
		return;
	const WCHAR* t[3] = { L"完整", L"简约", L"极简" };
	int v = (int)SendMessage(c, BM_GETCHECK, 0, 0);//0 未选 1 选中 2 不确定，对应 iMonitorSimple
	float w = (float)(rc.right - rc.left) / 3, h = (float)(rc.bottom - rc.top);
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(c, &pt);
	for (int i = 0; i < 3; i++)
		FSetDrawSegPart(w * i, 0, w, h, t[i], v == i, (st & CDIS_HOT) && pt.x >= w * i && pt.x < w * (i + 1), (st & CDIS_DISABLED) != 0);
	FSetEnd();
}
//“简约显示”原来是三态复选框，点一下循环切换；这里改成按点的位置选
LRESULT CALLBACK FSetSeg3Proc(HWND c, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_LBUTTONUP || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
	{
		if (msg == WM_LBUTTONUP)
		{
			RECT rc;
			GetClientRect(c, &rc);
			int i = GET_X_LPARAM(lParam) * 3 / (rc.right > 0 ? rc.right : 1);
			if (i >= 0 && i < 3 && i != (int)SendMessage(c, BM_GETCHECK, 0, 0))
			{
				SendMessage(c, BM_SETCHECK, i, 0);
				SendMessage(GetParent(c), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(c), BN_CLICKED), (LPARAM)c);
			}
		}
		return 0;
	}
	if (msg == WM_MOUSEMOVE)
		InvalidateRect(c, NULL, FALSE);//悬停的那一段跟着光标
	return CallWindowProc(fs.seg3Proc, c, msg, wParam, lParam);
}
//滑块：画的圆比控件自己的滑块区域大，控件拖动时只重画旧/新滑块那一小块，会留残影；每次都整块重画
LRESULT CALLBACK FSetSliderProc(HWND c, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_PAINT)
		InvalidateRect(c, NULL, FALSE);
	return CallWindowProc(fs.sliderProc, c, msg, wParam, lParam);
}
void FSetDrawButton(HWND c, HDC hdc, RECT rc, UINT st)
{
	if (!FSetBegin(hdc, &rc, FSCard()))
		return;
	float w = (float)(rc.right - rc.left), h = (float)(rc.bottom - rc.top);
	BOOL hot = (st & CDIS_HOT) != 0, down = (st & CDIS_SELECTED) != 0, dis = (st & CDIS_DISABLED) != 0;
	DWORD f = fs.dark ? (down ? 0x272727 : hot ? 0x323232 : 0x2D2D2D) : (down ? 0xF5F5F5 : hot ? 0xF6F6F6 : 0xFEFEFE);
	FFill(0, 0, w, h, f, 1, FS(4));
	FStroke(0, 0, w, h, fs.dark ? 0xFFFFFF : 0x000000, fs.dark ? .09f : .09f, FS(4));
	if (!fs.dark && !down)//浅色按钮底边略深
	{
		fRT->PushAxisAlignedClip(FR(0, h - FS(2), w, FS(2)), D2D1_ANTIALIAS_MODE_ALIASED);
		FStroke(0, 0, w, h, 0x000000, .12f, FS(4));
		fRT->PopAxisAlignedClip();
	}
	WCHAR t[48];
	GetWindowText(c, t, 48);
	FText(t, 0, 0, w, h, FS(14), 400, down ? FST2() : FST1(), dis ? .4f : 1.f, 1);
	if ((st & CDIS_FOCUS) && (st & CDIS_SHOWKEYBOARDCUES))
		FStroke(0, 0, w, h, FST1(), .9f, FS(4), FS(1.5f));
	FSetEnd();
}
//颜色按钮（ColorButtonProc 的 WM_PAINT 调过来）
void FSetDrawSwatch(HWND c, HDC hdc)
{
	RECT rc;
	GetClientRect(c, &rc);
	if (!FSetBegin(hdc, &rc, FSCard()))
		return;
	float w = (float)rc.right, h = (float)rc.bottom;
	int id = GetDlgCtrlID(c);
	COLORREF cr = id >= IDC_BUTTON_COLOR_BACKGROUND && id <= IDC_BUTTON_COLOR_HIGH ? TraySave.cMonitorColor[id - IDC_BUTTON_COLOR_BACKGROUND] : TraySave.dAlphaColor[iProject] & 0xffffff;
	WCHAR t[16];
	GetWindowText(c, t, 16);
	BOOL bAuto = id == IDC_BUTTON_COLOR_BACKGROUND && (cr == RGB(0, 0, 1) || cr == 0);
	DWORD rgb = bAuto ? FSTrack() : FRgb(cr);
	FFill(0, 0, w, h, rgb, 1, FS(4));
	FStroke(0, 0, w, h, fs.dark ? 0xFFFFFF : 0x000000, .16f, FS(4));
	BOOL light = GetRValue(cr) * 299 + GetGValue(cr) * 587 + GetBValue(cr) * 114 > 150000;
	FText(bAuto ? L"自动" : t, 0, 0, w, h, FS(12), 600, bAuto ? FST2() : light ? 0x1A1A1A : 0xFFFFFF, 1, 1);
	FSetEnd();
}
//滑块：细轨道，已选部分强调色，圆形滑块
LRESULT FSetDrawSlider(LPNMCUSTOMDRAW cd)
{
	if (cd->dwDrawStage == CDDS_PREPAINT)
		return CDRF_NOTIFYITEMDRAW;
	if (cd->dwDrawStage != CDDS_ITEMPREPAINT)
		return CDRF_DODEFAULT;
	if (cd->dwItemSpec == TBCD_TICS)
		return CDRF_SKIPDEFAULT;
	RECT rc;
	GetClientRect(cd->hdr.hwndFrom, &rc);
	if (cd->dwItemSpec == TBCD_CHANNEL)
	{
		RECT th;
		SendMessage(cd->hdr.hwndFrom, TBM_GETTHUMBRECT, 0, (LPARAM)&th);
		if (!FSetBegin(cd->hdc, &rc, FSCard()))
			return CDRF_SKIPDEFAULT;
		float h = (float)rc.bottom, cxm = (th.left + th.right) / 2.f, l = FS(11), r = rc.right - FS(11);
		FFill(l, h / 2 - FS(2), r - l, FS(4), fs.dark ? 0xFFFFFF : 0x000000, fs.dark ? .55f : .45f, FS(2));
		FFill(l, h / 2 - FS(2), cxm - l, FS(4), FSAccent(), 1, FS(2));
		float R = FS(10);
		FFill(cxm - R, h / 2 - R, R * 2, R * 2, fs.dark ? 0x454545 : 0xFFFFFF, 1, R);
		FStroke(cxm - R, h / 2 - R, R * 2, R * 2, 0x000000, fs.dark ? .3f : .12f, R);
		float ri = (cd->uItemState & (CDIS_HOT | CDIS_SELECTED)) ? FS(6.5f) : FS(5);
		FFill(cxm - ri, h / 2 - ri, ri * 2, ri * 2, FSAccent(), 1, ri);
		FSetEnd();
	}
	return CDRF_SKIPDEFAULT;//滑块在画轨道时一起画了
}
void FSetBrushes()
{
	if (fs.brCard) DeleteObject(fs.brCard);
	if (fs.brTrack) DeleteObject(fs.brTrack);
	if (fs.brInput) DeleteObject(fs.brInput);
	if (fs.brInputFocus) DeleteObject(fs.brInputFocus);
	fs.brCard = CreateSolidBrush(FSRef(FSCard()));
	fs.brTrack = CreateSolidBrush(FSRef(FSTrack()));
	fs.brInput = CreateSolidBrush(FSRef(FSInput()));
	fs.brInputFocus = CreateSolidBrush(FSRef(FSInputFocus()));
}
void FSetTheme()
{
	FluentLoadTheme();
	HKEY hk;
	DWORD v = 1, cb = sizeof(v);
	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hk) == ERROR_SUCCESS)
	{
		RegQueryValueEx(hk, L"AppsUseLightTheme", NULL, NULL, (BYTE*)&v, &cb);
		RegCloseKey(hk);
	}
	fs.dark = v == 0;
	FSetBrushes();
	typedef HRESULT(WINAPI* pfnDSWA)(HWND, DWORD, LPCVOID, DWORD);
	HMODULE hd = LoadLibrary(L"dwmapi.dll");
	pfnDSWA dswa = hd ? (pfnDSWA)GetProcAddress(hd, "DwmSetWindowAttribute") : NULL;
	if (dswa)
	{
		BOOL d = fs.dark;
		dswa(fs.h, 20, &d, sizeof(d));//DWMWA_USE_IMMERSIVE_DARK_MODE：标题栏跟着深色
	}
	if (hd)
		FreeLibrary(hd);
}
//打开设置窗口时调用（原来的初始化做完之后）
void FSetInit(HWND h)
{
	if (!FluentInit())
		return;
	fs.h = h;
	fs.page = 0;
	fs.hoverNav = -1;
	fs.npi = 0;
	UINT dpi = pGetDpiForWindow(h);
	fs.sc = (dpi ? dpi : 96) / 96.f;
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi);
	float fit = (mi.rcWork.bottom - mi.rcWork.top) / (FSET_H + 48.f);//屏幕矮放不下时整体缩小
	if (fs.sc > fit)
		fs.sc = fit;
	FSetTheme();
	SetWindowText(h, L"TrayS 设置");
	SetWindowLongPtr(h, GWL_STYLE, GetWindowLongPtr(h, GWL_STYLE) | WS_CLIPCHILDREN | WS_MINIMIZEBOX);
	//模态框样式的系统菜单里没有“最小化”，没有这一项时点标题栏的最小化按钮系统不理会
	InsertMenu(GetSystemMenu(h, FALSE), 0, MF_BYPOSITION | MF_STRING, SC_MINIMIZE, L"最小化(&N)");
	//窗口大小、居中
	RECT r = { 0, 0, (LONG)FS(FSET_W), (LONG)FS(FSET_H) };
	AdjustWindowRectEx(&r, (DWORD)GetWindowLongPtr(h, GWL_STYLE), FALSE, (DWORD)GetWindowLongPtr(h, GWL_EXSTYLE));
	int ww = r.right - r.left, wh = r.bottom - r.top;
	SetWindowPos(h, NULL, (mi.rcWork.left + mi.rcWork.right - ww) / 2, (mi.rcWork.top + mi.rcWork.bottom - wh) / 2, ww, wh, SWP_NOZORDER | SWP_FRAMECHANGED);
	fs.font = CreateFont(-(int)FS(14), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
	//控件文字改成新界面用的说法（这些文字只用来显示）
	const struct { int id; LPCWSTR t; } cap[] = {
		{ IDC_RADIO_NORMAL, L"常规" }, { IDC_RADIO_MAXIMIZE, L"有窗口最大化时" },
		{ IDC_RADIO_CENTER, L"居中" }, { IDC_RADIO_BYTE, L"字节 B/s" }, { IDC_RADIO_BIT, L"比特 b/s" },
		{ IDC_RADIO_AUTO, L"自动" }, { IDC_BUTTON_FONT, L"更改字体" }, { IDC_CLOSE, L"退出" },
		{ IDC_RESTORE_DEFAULT, L"恢复默认" }, { IDC_BUTTON_COLOR_TRAFFIC_LOW, L"正常" }, { IDC_BUTTON_COLOR_LOW, L"正常" },
		{ IDC_BUTTON_COLOR_BACKGROUND, L"背景" }, { IDC_BUTTON_COLOR, L"颜色" },
	};
	for (int i = 0; i < (int)(sizeof(cap) / sizeof(cap[0])); i++)
		SetDlgItemText(h, cap[i].id, cap[i].t);
	for (HWND c = GetWindow(h, GW_CHILD); c; c = GetWindow(c, GW_HWNDNEXT))
	{
		SendMessage(c, WM_SETFONT, (WPARAM)fs.font, FALSE);
		WCHAR cls[32];
		GetClassName(c, cls, 32);
		if (lstrcmpi(cls, L"Edit") == 0)//去掉边框，边框由窗口画；标签和单位分开放了，统一居中
		{
			SetWindowLongPtr(c, GWL_STYLE, (GetWindowLongPtr(c, GWL_STYLE) & ~(WS_BORDER | ES_RIGHT)) | ES_CENTER);
			SetWindowLongPtr(c, GWL_EXSTYLE, GetWindowLongPtr(c, GWL_EXSTYLE) & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
			SetWindowPos(c, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
		else if (lstrcmpi(cls, L"SysLink") == 0)
			SetWindowLongPtr(c, GWL_STYLE, GetWindowLongPtr(c, GWL_STYLE) | 0x0002);//LWS_TRANSPARENT
	}
	HWND s3 = GetDlgItem(h, IDC_CHECK_MONITOR_SIMPLE);
	if (s3)
		fs.seg3Proc = (WNDPROC)SetWindowLongPtr(s3, GWLP_WNDPROC, (LONG_PTR)FSetSeg3Proc);
	const int sliders[2] = { IDC_SLIDER_ALPHA, IDC_SLIDER_ALPHA_B };
	for (int i = 0; i < 2; i++)
	{
		HWND s = GetDlgItem(h, sliders[i]);
		if (s)
			fs.sliderProc = (WNDPROC)SetWindowLongPtr(s, GWLP_WNDPROC, (LONG_PTR)FSetSliderProc);//同一个类，原过程相同
	}
	FSetLayout();
	InvalidateRect(h, NULL, TRUE);
}
void FSetNavHit(int x, int y, int* idx)
{
	*idx = -1;
	if (x < FS(12) || x > FS(FSET_NAV - 16))
		return;
	for (int i = 0; i < FSET_PAGES; i++)
	{
		float ny, nh;
		FSetNavRect(i, &ny, &nh);
		if (y >= ny && y < ny + nh)
			*idx = i;
	}
}
//SettingProc 先调这里，返回 TRUE 表示已处理（结果在 *ret）
BOOL FSetProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam, INT_PTR* ret)
{
	if (!fs.h || fs.h != h)
		return FALSE;
	switch (msg)
	{
	case WM_ERASEBKGND:
		*ret = TRUE;
		return TRUE;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(h, &ps);
		FSetPaint(hdc);
		EndPaint(h, &ps);
		*ret = TRUE;
		return TRUE;
	}
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:
	{
		HDC dc = (HDC)wParam;
		SetBkColor(dc, FSRef(FSCard()));
		SetTextColor(dc, FSRef(FST1()));
		*ret = (INT_PTR)fs.brCard;
		return TRUE;
	}
	case WM_CTLCOLOREDIT:
	{
		HDC dc = (HDC)wParam;
		BOOL focus = GetFocus() == (HWND)lParam;
		SetBkColor(dc, FSRef(focus ? FSInputFocus() : FSInput()));
		SetTextColor(dc, FSRef(FST1()));
		*ret = (INT_PTR)(focus ? fs.brInputFocus : fs.brInput);
		return TRUE;
	}
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS)//输入框底边强调色
		{
			for (int i = 0; i < fs.npi; i++)
				if (fs.pi[i].kind == PI_FRAME && fs.pi[i].id == LOWORD(wParam))
				{
					RECT fr = { (LONG)fs.pi[i].x, (LONG)fs.pi[i].y, (LONG)(fs.pi[i].x + fs.pi[i].w + 1), (LONG)(fs.pi[i].y + fs.pi[i].h + 1) };
					InvalidateRect(h, &fr, FALSE);
				}
			InvalidateRect((HWND)lParam, NULL, TRUE);
		}
		return FALSE;//交给原来的处理
	case WM_NOTIFY:
	{
		LPNMHDR nh = (LPNMHDR)lParam;
		if (nh->code != NM_CUSTOMDRAW)
			return FALSE;
		LPNMCUSTOMDRAW cd = (LPNMCUSTOMDRAW)lParam;
		WCHAR cls[32];
		GetClassName(nh->hwndFrom, cls, 32);
		LRESULT r = CDRF_DODEFAULT;
		if (lstrcmpi(cls, L"msctls_trackbar32") == 0)
			r = FSetDrawSlider(cd);
		else if (lstrcmpi(cls, L"Button") == 0 && cd->dwDrawStage == CDDS_PREPAINT)
		{
			LONG_PTR style = GetWindowLongPtr(nh->hwndFrom, GWL_STYLE) & BS_TYPEMASK;
			if (nh->idFrom == IDC_CHECK_MONITOR_SIMPLE)
				FSetDrawSeg3(nh->hwndFrom, cd->hdc, cd->rc, cd->uItemState);
			else if (style == BS_AUTORADIOBUTTON || style == BS_RADIOBUTTON)
				FSetDrawRadio(nh->hwndFrom, cd->hdc, cd->rc, cd->uItemState);
			else if (style == BS_AUTOCHECKBOX || style == BS_CHECKBOX || style == BS_AUTO3STATE)
				FSetDrawToggle(nh->hwndFrom, cd->hdc, cd->rc, cd->uItemState);
			else
				FSetDrawButton(nh->hwndFrom, cd->hdc, cd->rc, cd->uItemState);
			r = CDRF_SKIPDEFAULT;
		}
		else
			return FALSE;
		SetWindowLongPtr(h, DWLP_MSGRESULT, r);
		*ret = TRUE;
		return TRUE;
	}
	case WM_LBUTTONDOWN:
	{
		int i;
		FSetNavHit(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &i);
		if (i >= 0 && i != fs.page)
		{
			fs.page = i;
			//焦点不能交给对话框：它会转给输入框并全选，隐藏旧页时焦点再一路挤到下一个输入框，看起来右边的字全被选中闪一下
			SetFocus(NULL);
			FSetLayout();
			InvalidateRect(h, NULL, FALSE);
		}
		*ret = TRUE;
		return TRUE;
	}
	case WM_MOUSEMOVE:
	{
		int i;
		FSetNavHit(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &i);
		if (!fs.bTrackLeave)
		{
			TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, h, 0 };
			TrackMouseEvent(&tme);
			fs.bTrackLeave = TRUE;
		}
		if (i != fs.hoverNav)
		{
			fs.hoverNav = i;
			RECT nr = { 0, 0, (LONG)FS(FSET_NAV), (LONG)FS(FSET_H) };
			InvalidateRect(h, &nr, FALSE);
		}
		return FALSE;
	}
	case WM_MOUSELEAVE:
		fs.bTrackLeave = FALSE;
		if (fs.hoverNav != -1)
		{
			fs.hoverNav = -1;
			RECT nr = { 0, 0, (LONG)FS(FSET_NAV), (LONG)FS(FSET_H) };
			InvalidateRect(h, &nr, FALSE);
		}
		return FALSE;
	case WM_SETTINGCHANGE:
		if (lParam && lstrcmpi((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)//切换了深浅色
		{
			FSetTheme();
			RedrawWindow(h, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
		}
		return FALSE;
	case WM_DESTROY:
		if (fs.seg3Proc)//子窗口比父窗口晚销毁，先把窗口过程换回去
			SetWindowLongPtr(GetDlgItem(h, IDC_CHECK_MONITOR_SIMPLE), GWLP_WNDPROC, (LONG_PTR)fs.seg3Proc);
		if (fs.sliderProc)
		{
			SetWindowLongPtr(GetDlgItem(h, IDC_SLIDER_ALPHA), GWLP_WNDPROC, (LONG_PTR)fs.sliderProc);
			SetWindowLongPtr(GetDlgItem(h, IDC_SLIDER_ALPHA_B), GWLP_WNDPROC, (LONG_PTR)fs.sliderProc);
		}
		if (fs.font) DeleteObject(fs.font);
		if (fs.brCard) DeleteObject(fs.brCard);
		if (fs.brTrack) DeleteObject(fs.brTrack);
		if (fs.brInput) DeleteObject(fs.brInput);
		if (fs.brInputFocus) DeleteObject(fs.brInputFocus);
		if (fs.br) fs.br->Release();
		if (fs.rt) fs.rt->Release();
		memset(&fs, 0, sizeof(fs));
		return FALSE;
	}
	return FALSE;
}
