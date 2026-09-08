#include "GoUI.h"
#include <string>
#include <functional>
#include <random>
#include <algorithm>
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <cstdio>
#include "..\MyGo\Resource.h"

GoUI::GoUI(int boardSize)
	: nana::form(nana::API::make_center(580, 700)),
	core(boardSize),
	drawer(*this),
	boardSize(boardSize),
	padX(0),
	padY(0),
	cellSize(20),
	bottomReserved(90),
	lblStatus(*this),
	btnNewGame(*this),
	btnUndo(*this),
	btnPass(*this),
	btnResign(*this),
	btnExport(*this),
	btnImport(*this),
	cmbDifficulty(*this),
	aiThinking(false),
	hoverRow(-1),
	hoverCol(-1)
{
	this->caption("围棋");

	// 控件位置
	lblStatus.move(nana::rectangle(10, this->size().height - bottomReserved + 10, 300, 25));
	lblStatus.caption("黑棋走");
	lblStatus.bgcolor(nana::color(210, 180, 140));
	lblStatus.fgcolor(nana::color(0, 0, 0));

	cmbDifficulty.move(nana::rectangle(428, this->size().height - 30, 120, 25));
	cmbDifficulty.push_back("简单 (稳健)");
	cmbDifficulty.push_back("中等 (蒙特卡洛)");
	cmbDifficulty.push_back("困难 (混合算法)");
	cmbDifficulty.push_back("双人模式");
	cmbDifficulty.option(0);

	// 底部单行按钮：新游戏 → 导出 → 导入 → 悔棋 → 停一手 → 认输 → 难度(combox)
	btnNewGame.move(nana::rectangle(8, this->size().height - 30, 66, 25));
	btnNewGame.caption("新游戏");
	btnExport.move(nana::rectangle(78, this->size().height - 30, 66, 25));
	btnExport.caption("导出棋谱");
	btnImport.move(nana::rectangle(148, this->size().height - 30, 66, 25));
	btnImport.caption("导入棋谱");
	btnUndo.move(nana::rectangle(218, this->size().height - 30, 66, 25));
	btnUndo.caption("悔棋");
	btnPass.move(nana::rectangle(288, this->size().height - 30, 66, 25));
	btnPass.caption("停一手");
	btnResign.move(nana::rectangle(358, this->size().height - 30, 66, 25));
	btnResign.caption("认输");

	// 事件绑定
	btnNewGame.events().click([this]() { onNewGame(); });
	btnUndo.events().click([this]() { onUndo(); });
	btnPass.events().click([this]() { onPass(); });
	btnResign.events().click([this]() {
		std::string msg = core.isBlackTurn() ? "黑棋认输，白棋获胜" : "白棋认输，黑棋获胜";
		nana::msgbox("认输") << msg;
		core.reset();
		nana::API::refresh_window(*this);
		updateStatus();
	});
	btnExport.events().click([this]() { onExport(); });
	btnImport.events().click([this]() { onImport(); });

	drawer.draw([this](nana::paint::graphics& graph) { onDraw(graph); });
	this->events().click(std::bind(&GoUI::onMouseClickEvent, this, std::placeholders::_1));
	this->events().mouse_move([this](const nana::arg_mouse& arg) {
		onMouseMove(arg.pos.x, arg.pos.y);
	});

	// 快捷键：Ctrl+Z 悔棋、Ctrl+N 新游戏、空格 停一手
	// 注意：Nana 的 key_press 只派发给当前有焦点的控件，点过按钮后焦点就随按钮走了，
	// 因此窗体与所有子控件都绑定同一处理函数，保证快捷键始终生效。
	auto bindKeys = [this](nana::widget& w) {
		w.events().key_press([this](const nana::arg_keyboard& arg) { onKeyPress(arg); });
	};
	bindKeys(*this);
	bindKeys(lblStatus);
	bindKeys(cmbDifficulty);
	bindKeys(btnNewGame);
	bindKeys(btnUndo);
	bindKeys(btnPass);
	bindKeys(btnResign);
	bindKeys(btnExport);
	bindKeys(btnImport);

	this->events().resized([this](const nana::arg_resized& arg) {
		int w = arg.width, h = arg.height;
		lblStatus.move(nana::rectangle(10, h - bottomReserved + 10, 300, 25));
		cmbDifficulty.move(nana::rectangle(428, h - 30, 120, 25));
		btnNewGame.move(nana::rectangle(8, h - 30, 66, 25));
		btnExport.move(nana::rectangle(78, h - 30, 66, 25));
		btnImport.move(nana::rectangle(148, h - 30, 66, 25));
		btnUndo.move(nana::rectangle(218, h - 30, 66, 25));
		btnPass.move(nana::rectangle(288, h - 30, 66, 25));
		btnResign.move(nana::rectangle(358, h - 30, 66, 25));
		nana::API::refresh_window(*this);
	});

	loadConfig();   // 恢复上次保存的设置（难度、窗口大小）
	updateStatus();

	// 关闭窗口时保存设置
	this->events().unload([this](const nana::arg_unload&) { saveConfig(); });
}

void GoUI::loadConfig() {
	const std::wstring ini = configPath();
	int diff = ::GetPrivateProfileIntW(L"Settings", L"Difficulty", -1, ini.c_str());
	if (diff >= 0 && diff <= 3) cmbDifficulty.option(diff);
	int w = ::GetPrivateProfileIntW(L"Settings", L"Width", 0, ini.c_str());
	int h = ::GetPrivateProfileIntW(L"Settings", L"Height", 0, ini.c_str());
	if (w >= 600 && h >= 700) this->size(nana::size(w, h));
}

void GoUI::saveConfig() {
	const std::wstring ini = configPath();
	wchar_t buf[32];
	_itow_s(cmbDifficulty.option(), buf, 10);
	::WritePrivateProfileStringW(L"Settings", L"Difficulty", buf, ini.c_str());
	_itow_s(this->size().width, buf, 10);
	::WritePrivateProfileStringW(L"Settings", L"Width", buf, ini.c_str());
	_itow_s(this->size().height, buf, 10);
	::WritePrivateProfileStringW(L"Settings", L"Height", buf, ini.c_str());
}

std::wstring GoUI::configPath() const {
	wchar_t full[MAX_PATH];
	::GetModuleFileNameW(NULL, full, MAX_PATH);
	std::wstring p = full;
	// 把 .exe 后缀替换为 .ini，得到与程序同名的配置文件
	size_t ext = p.rfind(L".exe");
	if (ext != std::wstring::npos) p.replace(ext, 4, L".ini");
	return p;
}

void GoUI::run() {
	this->show();

	// 使用 exe 内嵌资源图标，同时设置标题栏(ICON_SMALL)和任务栏(ICON_BIG)
	HICON hIcon = ::LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP));
	if (hIcon) {
		HWND hwnd = reinterpret_cast<HWND>(this->native_handle());
		::SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
		::SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
	}

	nana::exec();
}

void GoUI::onMouseClickEvent(const nana::arg_click& arg) {
	if (arg.mouse_args)
		onMouseClick(arg.mouse_args->pos.x, arg.mouse_args->pos.y);
}

void GoUI::onDraw(nana::paint::graphics& graph) {
	graph.rectangle(true, nana::color(210, 180, 140));   // 浅橡木色
	int width = graph.width(), height = graph.height();
	int effectiveHeight = height - bottomReserved;
	int margin = 20;
	int contentW = width - 2 * margin;
	int contentH = effectiveHeight - 2 * margin;
	int maxSize = (contentW < contentH) ? contentW : contentH;
	cellSize = maxSize / boardSize;
	int totalGrid = (boardSize - 1) * cellSize;
	padX = (width - totalGrid) / 2;            // 水平居中
	padY = (effectiveHeight - totalGrid) / 2;  // 垂直居中（避开底部按钮区）

	nana::color black(0, 0, 0), gray(128, 128, 128);

	for (int i = 0; i < boardSize; ++i) {
		int x = padX + i * cellSize;
		int y = padY + i * cellSize;
		graph.line(nana::point(padX, y), nana::point(padX + totalGrid, y), black);
		graph.line(nana::point(x, padY), nana::point(x, padY + totalGrid), black);
	}

	for (int r = 0; r < boardSize; ++r) {
		for (int c = 0; c < boardSize; ++c) {
			Cell cell = core.getCell(r, c);
			if (cell == Cell::Empty) continue;
			int xc = padX + c * cellSize;
			int yc = padY + r * cellSize;
			int radius = cellSize / 2 - 2;
			int diameter = radius * 2;
			nana::color color = (cell == Cell::Black) ? nana::color(0, 0, 0) : nana::color(255, 255, 255);
			nana::rectangle rect(xc - radius, yc - radius, diameter, diameter);
			graph.round_rectangle(rect, radius, radius, gray, true, color);
			graph.round_rectangle(rect, radius, radius, gray, false, nana::color());
		}
	}

	// 鼠标悬停预览：当前走棋方的半透明子（黑=深棕灰，白=浅米灰）
	if (hoverRow >= 0 && hoverCol >= 0 && core.getCell(hoverRow, hoverCol) == Cell::Empty) {
		int px = padX + hoverCol * cellSize;
		int py = padY + hoverRow * cellSize;
		int radius = cellSize / 2 - 2;
		nana::color preview = core.isBlackTurn() ? nana::color(105, 90, 70) : nana::color(215, 205, 185);
		nana::rectangle pr(px - radius, py - radius, radius * 2, radius * 2);
		graph.round_rectangle(pr, radius, radius, preview, true, preview);
	}

	int lastR = core.getLastMoveRow();
	int lastC = core.getLastMoveCol();
	if (lastR != -1 && lastC != -1) {
		int hx = padX + lastC * cellSize;
		int hy = padY + lastR * cellSize;
		int hr = 8;
		nana::rectangle hrect(hx - hr / 2, hy - hr / 2, hr, hr);
		graph.round_rectangle(hrect, hr / 2, hr / 2, nana::color(255, 0, 0), true, nana::color(255, 0, 0));
	}

	// ---------- 绘制坐标 ----------
	nana::color textColor(0, 0, 0);
	for (int i = 0; i < boardSize; ++i) {
		int x = padX + i * cellSize;
		int y = padY + i * cellSize;


		int bottomY = padY + totalGrid + cellSize / 2 + 4;
		std::string colStr = colLabel(i);
		if (!colStr.empty()) {
			graph.string(nana::point(x - 5, bottomY), colStr, textColor);
		}

		// 左侧行数字（从底部向上）
		int rowNum = boardSize - i;
		std::string rowStr = std::to_string(rowNum);
		int leftX = padX - 18;
		graph.string(nana::point(leftX, y - 6), rowStr, textColor);
	}

	if (aiThinking) {
		int hintY = graph.size().height - bottomReserved + 12;
		nana::color hintColor(220, 40, 40);
		graph.string(nana::point(10, hintY), "AI 思考中...", hintColor);
	}
}

void GoUI::updateStatus() {
	std::string difficulty;
	int diff = cmbDifficulty.option();
	switch (diff) {
	case 0: difficulty = "简单"; break;
	case 1: difficulty = "中等"; break;
	case 2: difficulty = "困难"; break;
	case 3: difficulty = "双人"; break;
	default: difficulty = "未知";
	}

	std::string status = (core.isBlackTurn() ? "黑棋走" : "白棋走");
	status += "  |  黑提: " + std::to_string(core.getCapturesBlack()) +
		"  白提: " + std::to_string(core.getCapturesWhite()) +
		"  [" + difficulty + "]";
	lblStatus.caption(status);
	this->caption("围棋吃5子 - " + std::to_string(boardSize) + "路  " +
		(core.isBlackTurn() ? "黑棋" : "白棋") + "走");
}

void GoUI::onNewGame() {
	core.reset();
	nana::API::refresh_window(*this);
	updateStatus();
}

void GoUI::onUndo() {
	if (cmbDifficulty.option() == 3) {
		// 双人模式：黑白由玩家轮流下，悔棋只撤当前最后一手
		if (core.undo()) {
			nana::API::refresh_window(*this);
			updateStatus();
		}
		else {
			nana::msgbox("提示") << "没有可以悔的棋";
		}
		return;
	}
	// 正常模式：悔棋需撤回到玩家(黑棋)回合：玩家落子 + AI 应手为两步，必要时连撤两步
	int undone = 0;
	while (undone < 2 && core.undo()) {
		undone++;
		if (core.isBlackTurn()) break;
	}
	if (undone > 0) {
		nana::API::refresh_window(*this);
		updateStatus();
	}
	else {
		nana::msgbox("提示") << "没有可以悔的棋";
	}
}

void GoUI::onPass() {
	core.pass();
	nana::API::refresh_window(*this);
	updateStatus();
	if (core.isGameOver()) onGameOver();
	else {
		if (core.getLastMoveRow() == -1)   // 这一手是 Pass，明确提示（平局时由平局弹框承担）
			nana::msgbox("停一手") << "已停一手（Pass）。双方连续停一手则平局结束本局。";
		if (!core.isBlackTurn() && cmbDifficulty.option() != 3) doAIMove();   // 玩家停一手后 AI 自动应手（双人模式除外）
	}
}
void GoUI::playSound(const wchar_t* wavFile) {
	static std::wstring exeDir;
	if (exeDir.empty()) {
		wchar_t buf[MAX_PATH];
		::GetModuleFileNameW(NULL, buf, MAX_PATH);
		wchar_t* slash = wcsrchr(buf, L'\\');
		if (slash) *(slash + 1) = 0; else wcscpy(buf, L".\\");
		exeDir = buf;
	}
	// 音频统一放在与 exe 同目录的 res 子目录
	std::wstring path = exeDir + L"res\\" + wavFile;
	// 文件不存在则不播放，保证软件无音频也能正常运行
	if (::GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
		return;
	::PlaySoundW(path.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
}

void GoUI::onExport() {
	if (core.getMoveLog().empty()) {
		nana::msgbox("导出棋谱") << "还没有任何落子，无法导出";
		return;
	}
	OPENFILENAMEW ofn;
	::memset(&ofn, 0, sizeof(ofn));
	wchar_t file[MAX_PATH] = L"棋谱.sgf";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = reinterpret_cast<HWND>(this->native_handle());
	ofn.lpstrFilter = L"SGF 棋谱 (*.sgf)\0*.sgf\0所有文件 (*.*)\0*.*\0";
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"sgf";
	if (::GetSaveFileNameW(&ofn)) {
		std::string sgf = core.exportSGF();
		FILE* f = _wfopen(file, L"wb");
		if (!f) {
			nana::msgbox("导出棋谱") << "无法写入文件";
			return;
		}
		fwrite(sgf.data(), 1, sgf.size(), f);
		fclose(f);
		nana::msgbox("导出棋谱") << "棋谱已导出";
	}
}

void GoUI::onImport() {
	OPENFILENAMEW ofn;
	::memset(&ofn, 0, sizeof(ofn));
	wchar_t file[MAX_PATH] = L"棋谱.sgf";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = reinterpret_cast<HWND>(this->native_handle());
	ofn.lpstrFilter = L"SGF 棋谱 (*.sgf)\0*.sgf\0所有文件 (*.*)\0*.*\0";
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L"sgf";
	if (!::GetOpenFileNameW(&ofn)) return;

	FILE* f = _wfopen(file, L"rb");
	if (!f) {
		nana::msgbox("导入棋谱") << "无法打开文件";
		return;
	}
	std::string text;
	char buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
	fclose(f);
	// 跳过 UTF-8 BOM
	if (text.size() >= 3 &&
		(unsigned char)text[0] == 0xEF &&
		(unsigned char)text[1] == 0xBB &&
		(unsigned char)text[2] == 0xBF) {
		text = text.substr(3);
	}

	std::string err;
	if (!core.loadSGF(text, err)) {
		nana::msgbox("导入棋谱失败") << err;
		nana::API::refresh_window(*this);
		updateStatus();
		return;
	}
	nana::API::refresh_window(*this);
	updateStatus();
	// 导入后轮到白（AI）时自动应手（双人模式除外）
	if (!core.isBlackTurn() && cmbDifficulty.option() != 3)
		doAIMove();
}

void GoUI::onGameOver() {
	// 玩家执黑：玩家赢 → 黑中盘胜；电脑（白）赢 → 白中盘胜；无人吃满5子 → 平局
	if (core.getCapturesBlack() >= 5)
		playSound(L"黑中盘胜.wav");
	else if (core.getCapturesWhite() >= 5)
		playSound(L"白中盘胜.wav");
	else
		playSound(L"平局.wav");
	nana::msgbox mb("游戏结束");
	mb << core.getWinner();
	mb.show();
	core.reset();
	nana::API::refresh_window(*this);
	updateStatus();
}


void GoUI::doAIMove() {
	if (core.isGameOver()) return;

	// AI 思考提示：标志 + 文本双保险，RedrawWindow 同步强制重绘立即显示
	// （不改变下棋逻辑，AI 仍在界面线程同步计算）
	aiThinking = true;
	lblStatus.caption("AI 思考中...");
	nana::API::refresh_window(*this);
	::RedrawWindow(reinterpret_cast<HWND>(this->native_handle()), NULL, NULL,
		RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

	int diff = cmbDifficulty.option();
	int capBefore = core.getCapturesBlack() + core.getCapturesWhite();
	switch (diff) {
	case 0: doSimpleMove(); break;
	case 1: doHeuristicMove(); break;
	case 2: doHybridMove(); break;      // 困难 → 混合算法
	default: doSimpleMove(); break;
	}
	if (core.getLastMoveRow() != -1) {   // AI 确实落子（Pass 时 lastMove 为 -1）
		bool captured = (core.getCapturesBlack() + core.getCapturesWhite() > capBefore);
		playSound(captured ? L"capture.wav" : L"place.wav");
	}
	else if (!core.isGameOver()) {       // AI 停一手（Pass）：已无合法落子或全局评估分≤0
		nana::msgbox("停一手") << "AI 选择停一手（Pass）。双方连续停一手则平局结束本局。";
	}
	aiThinking = false;
	nana::API::refresh_window(*this);
	updateStatus();
	if (core.isGameOver()) onGameOver();
}

// ========== AI 算法：随机 ==========
void GoUI::doRandomMove() {
	if (core.isGameOver()) return;
	std::vector<std::pair<int, int>> empty;
	int size = core.getBoardSize();
	for (int r = 0; r < size; ++r)
		for (int c = 0; c < size; ++c)
			if (core.getCell(r, c) == Cell::Empty)
				empty.push_back({ r,c });
	if (empty.empty() || !core.hasLegalMove()) { core.pass(); return; }
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::shuffle(empty.begin(), empty.end(), gen);
	for (auto& p : empty) {
		if (core.placeStone(p.first, p.second)) return;   // 选到非法点自动换一点重试
	}
	core.pass();   // 兜底（hasLegalMove 已保证至少一个合法点，正常走不到）
}

// ========== AI 算法：净发展评估（简单难度）==========
// 用 assessMoveSimple（只算提子/己方气/连接/中心，不追击不逃命）
// 对全部空点打分取最高，行为接近早期未优化的中等算法。
void GoUI::doSimpleMove() {
	if (core.isGameOver()) return;
	if (!core.hasLegalMove()) { core.pass(); return; }
	int size = core.getBoardSize();
	int br = -1, bc = -1, bs = -1000000;
	for (int r = 0; r < size; ++r)
		for (int c = 0; c < size; ++c)
			if (core.getCell(r, c) == Cell::Empty) {
				int s = core.assessMoveSimple(r, c);
				if (s > bs) { bs = s; br = r; bc = c; }
			}
	if (br == -1 || !core.placeStone(br, bc)) core.pass();   // 兜底
}

// ========== AI 算法：蒙特卡洛模拟 ==========
void GoUI::doMonteCarloMove(int simulations) {
	int size = core.getBoardSize();
	std::vector<std::pair<int, int>> empty;
	for (int r = 0; r < size; ++r)
		for (int c = 0; c < size; ++c)
			if (core.getCell(r, c) == Cell::Empty)
				empty.push_back({ r, c });
	if (empty.empty() || !core.hasLegalMove()) {
		core.pass();
		return;
	}

	static std::random_device rd;
	static std::mt19937 gen(rd());   // <-- 添加这行，定义静态随机数生成器

	std::vector<int> wins(empty.size(), 0);
	for (size_t i = 0; i < empty.size(); ++i) {
		for (int s = 0; s < simulations; ++s) {
			GoCore simCore = core;   // 拷贝当前棋盘状态
			simCore.placeStone(empty[i].first, empty[i].second);
			int maxSteps = 30;
			while (maxSteps-- > 0 && !simCore.isGameOver()) {
				std::vector<std::pair<int, int>> simEmpty;
				for (int r = 0; r < size; ++r)
					for (int c = 0; c < size; ++c)
						if (simCore.getCell(r, c) == Cell::Empty)
							simEmpty.push_back({ r, c });
				if (simEmpty.empty()) break;
				std::uniform_int_distribution<int> dis(0, static_cast<int>(simEmpty.size()) - 1);
				int idx = dis(gen);
				simCore.placeStone(simEmpty[idx].first, simEmpty[idx].second);
			}
			// 评估：白棋（AI）提子数多于黑棋则胜
			if (simCore.getCapturesWhite() > simCore.getCapturesBlack())
				wins[i]++;
		}
	}
	int bestIdx = 0;
	for (size_t i = 1; i < wins.size(); ++i)
		if (wins[i] > wins[bestIdx])
			bestIdx = i;
	core.placeStone(empty[bestIdx].first, empty[bestIdx].second);
}

void GoUI::onMouseMove(int x, int y) {
	int col = (x - padX + cellSize / 2) / cellSize;
	int row = (y - padY + cellSize / 2) / cellSize;
	bool inBoard = (row >= 0 && row < boardSize && col >= 0 && col < boardSize);

	// 悬停预览只在玩家可下棋的回合有效：AI 模式=黑棋回合；双人模式=当前走棋方
	bool two = (cmbDifficulty.option() == 3);   // 双人模式
	bool canPreview = two ? true : core.isBlackTurn();
	int newRow, newCol;
	if (inBoard && canPreview && core.getCell(row, col) == Cell::Empty) {
		newRow = row;
		newCol = col;
	}
	else {
		newRow = newCol = -1;
	}

	// 仅在预览格子变化时才重绘，降低开销
	if (newRow != hoverRow || newCol != hoverCol) {
		hoverRow = newRow;
		hoverCol = newCol;
		nana::API::refresh_window(*this);
	}
}

void GoUI::onKeyPress(const nana::arg_keyboard& arg) {
	if (arg.ctrl && (arg.key == L'z' || arg.key == L'Z'))
		onUndo();
	else if (arg.ctrl && (arg.key == L'n' || arg.key == L'N'))
		onNewGame();
	else if (arg.key == L' ')
		onPass();
}

void GoUI::onMouseClick(int x, int y) {
	if (core.isGameOver()) return;
	bool two = (cmbDifficulty.option() == 3);   // 选中的是"双人模式"档
	if (!two && !core.isBlackTurn()) return;     // AI 模式：只有黑棋（玩家）回合才处理；双人模式黑白都可下

	int col = (x - padX + cellSize / 2) / cellSize;
	int row = (y - padY + cellSize / 2) / cellSize;

	if (row < 0 || row >= boardSize || col < 0 || col >= boardSize)
		return;

	int capBefore = core.getCapturesBlack() + core.getCapturesWhite();
	if (core.placeStone(row, col)) {
		hoverRow = hoverCol = -1;   // 落子后清除悬停预览
		bool captured = (core.getCapturesBlack() + core.getCapturesWhite() > capBefore);
		playSound(captured ? L"capture.wav" : L"place.wav");
		nana::API::refresh_window(*this);
		updateStatus();

		if (core.isGameOver()) {
			onGameOver();
			return;
		}

		if (!core.isBlackTurn() && !two) {
			doAIMove();
		}
	}
}

// GoUI.cpp 新增
void GoUI::doHeuristicMove() {
	if (core.isGameOver()) return;
	int size = core.getBoardSize();
	std::vector<std::pair<int, int>> empty;
	for (int r = 0; r < size; ++r)
		for (int c = 0; c < size; ++c)
			if (core.getCell(r, c) == Cell::Empty)
				empty.push_back({ r, c });
	if (empty.empty() || !core.hasLegalMove()) { core.pass(); return; }

	// 找评估分前三候选
	int bestScore = -1000000;
	int bestIdx = 0;
	int secondScore = -1000001;
	int secondIdx = -1;
	int thirdScore = -1000002;
	int thirdIdx = -1;
	for (int i = 0; i < (int)empty.size(); ++i) {
		int score = core.assessMove(empty[i].first, empty[i].second);
		if (score > bestScore) {
			thirdScore = secondScore;  thirdIdx = secondIdx;
			secondScore = bestScore;   secondIdx = bestIdx;
			bestScore = score;         bestIdx = i;
		}
		else if (score > secondScore) {
			thirdScore = secondScore;  thirdIdx = secondIdx;
			secondScore = score;       secondIdx = i;
		}
		else if (score > thirdScore) {
			thirdScore = score;        thirdIdx = i;
		}
	}

	// A. 全盘无正分点：怎么走都亏 → 放弃（收官不乱填）
	if (bestScore <= 0) { core.pass(); return; }

	// B. 次优随机化：这一步"怎么走都差不离"（top2 差距小）时，在 top3 里按
	//    「best − 自身分 +1」为权重加权随机挑一着（拉开差距时概率骤降，
	//    关键手如救援/提子/送死几乎必选最优）。避免每局开局/平稳步完全一样。
	int pickIdx = bestIdx;
	static std::random_device rd;
	static std::mt19937 gen(rd());
	// 关键手（白方存在 1 气濒死块）禁止次优随机，必须走最优（救援/对攻）
	if (secondIdx != -1 && bestScore - secondScore < 15 && !core.whiteInAtari()) {
		int cands[3] = { bestIdx, secondIdx, thirdIdx };
		int weights[3] = { 1, 1, 1 };
		int total = 0;
		for (int k = 0; k < 3; ++k) {
			if (cands[k] < 0) break;
			int w = bestScore - (k == 0 ? bestScore : (k == 1 ? secondScore : thirdScore)) + 1;
			if (w < 1) w = 1;
			weights[k] = w;
			total += w;
		}
		unsigned int rr = (unsigned int)(gen() % (total > 0 ? total : 1));
		int acc = 0;
		for (int k = 0; k < 3; ++k) {
			if (cands[k] < 0) break;
			acc += weights[k];
			if (rr < (unsigned int)acc) { pickIdx = cands[k]; break; }
		}
	}
	if (!core.placeStone(empty[pickIdx].first, empty[pickIdx].second))
		core.pass();   // 兜底：选中的点不可下则停一手
}

std::string GoUI::colLabel(int col) const
{
	static const char* labels = "ABCDEFGHJKLMNOPQRST"; // 19路够用
	if (col < 0 || col >= boardSize) return "";
	return std::string(1, labels[col]);
}

#include <numeric>   // 在文件顶部添加，用于 std::iota

// ... 其他函数 ...

void GoUI::doHybridMove() {
	if (core.isGameOver()) return;
	int size = core.getBoardSize();
	std::vector<std::pair<int, int>> empty;
	for (int r = 0; r < size; ++r)
		for (int c = 0; c < size; ++c)
			if (core.getCell(r, c) == Cell::Empty)
				empty.push_back({ r, c });
	if (empty.empty() || !core.hasLegalMove()) { core.pass(); return; }

	std::vector<int> scores;
	for (auto& p : empty) {
		scores.push_back(core.assessMove(p.first, p.second));
	}

	std::vector<size_t> idx(scores.size());
	std::iota(idx.begin(), idx.end(), 0);
	std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
		return scores[a] > scores[b];
	});

	// 取前 5 个候选（可调整）
	const int topN = std::min(5, (int)empty.size());
	std::vector<std::pair<int, int>> topCandidates;
	for (int i = 0; i < topN; ++i) {
		topCandidates.push_back(empty[idx[i]]);
	}

	// 核心手保护：白方有 1 气濒死块时，只在"能救它"的候选里做 MC
	//（否则模拟胜率噪声可能把救援点翻转成攻击点）
	if (core.whiteInAtari()) {
		std::vector<std::pair<int, int>> savers;
		for (auto& p : topCandidates)
			if (core.weakSaveLevel(p.first, p.second) >= 1) savers.push_back(p);
		if (!savers.empty()) {
			topCandidates = savers;
			// 重建与候选列表对齐的排序索引/分数
			std::vector<int> sc2;
			for (auto& p : topCandidates) sc2.push_back(core.assessMove(p.first, p.second));
			std::vector<size_t> idx2(sc2.size());
			std::iota(idx2.begin(), idx2.end(), 0);
			std::sort(idx2.begin(), idx2.end(), [&](size_t a, size_t b) { return sc2[a] > sc2[b]; });
			scores = sc2;
			idx = idx2;
		}
	}

	// 2. 对候选点进行蒙特卡洛模拟（100次）
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::vector<int> wins(topCandidates.size(), 0);

	for (size_t i = 0; i < topCandidates.size(); ++i) {
		for (int s = 0; s < 100; ++s) {
			GoCore simCore = core;  // 复制当前棋盘
			simCore.placeStone(topCandidates[i].first, topCandidates[i].second);
			int maxSteps = 30;
			while (maxSteps-- > 0 && !simCore.isGameOver()) {
				std::vector<std::pair<int, int>> simEmpty;
				for (int r = 0; r < size; ++r)
					for (int c = 0; c < size; ++c)
						if (simCore.getCell(r, c) == Cell::Empty)
							simEmpty.push_back({ r, c });
				if (simEmpty.empty()) break;
				std::uniform_int_distribution<int> dis(0, static_cast<int>(simEmpty.size()) - 1);
				int idx2 = dis(gen);
				simCore.placeStone(simEmpty[idx2].first, simEmpty[idx2].second);
			}
			// 评估白棋（AI）的提子数是否多于黑棋
			if (simCore.getCapturesWhite() > simCore.getCapturesBlack())
				wins[i]++;
		}
	}

// 3. 综合选点：评估分主导 + 模拟胜率微调
	//    （蒙特卡洛纯随机 playout 噪声大，若权重过高会被"远离战场的虚高胜率点"带偏；
	//    故胜率仅作细调：评估分差 1 点胜过 4 点胜率，只影响评估分接近的点）
	int bestIdx = 0;
	int bestTotal = scores[idx[0]] + wins[0] / 4;
	for (size_t i = 1; i < topCandidates.size(); ++i) {
		int total = scores[idx[i]] + wins[i] / 4;
		if (total > bestTotal) {
			bestTotal = total;
bestIdx = static_cast<int>(i);
		}
	}

	// A. 全盘无正分综合 → 放弃（收官不乱填，避免负分点送子）
	if (bestTotal <= 0) { core.pass(); return; }
	if (!core.placeStone(topCandidates[bestIdx].first, topCandidates[bestIdx].second))
		core.pass();   // 兜底：选中的点不可下则停一手
}

