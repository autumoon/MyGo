#ifndef GOUI_H
#define GOUI_H

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/combox.hpp>
#include "../Core/GoCore.h"

class GoUI : public nana::form {
public:
	GoUI(int boardSize = 9);
	void run();

private:
	GoCore core;
	nana::drawing drawer;
	int boardSize;
	int padX;
	int padY;
	int cellSize;
	int bottomReserved;

	nana::label lblStatus;
	nana::button btnNewGame;
	nana::button btnUndo;
	nana::button btnPass;
	nana::button btnResign;
	nana::button btnExport;    // 导出棋谱
	nana::button btnImport;    // 导入棋谱
	nana::combox cmbDifficulty;
	bool aiThinking;        // AI 思考中标志
	int hoverRow;           // 鼠标悬停预览的棋子位置（-1 表示无）
	int hoverCol;

	std::string colLabel(int col) const;   // 列字母

	void onDraw(nana::paint::graphics& graph);
	void onMouseClick(int x, int y);
	void onMouseClickEvent(const nana::arg_click& arg);
	void onMouseMove(int x, int y);
	void onKeyPress(const nana::arg_keyboard& arg);   // 快捷键统一处理
	void updateStatus();
	void onNewGame();                  // 新游戏
	void onUndo();                     // 悔棋
	void onPass();                     // 停一手
	void onExport();                   // 导出棋谱（SGF）
	void onImport();                   // 导入棋谱（SGF）
	void onGameOver();
	void playSound(const wchar_t* wavFile);   // 播放 wav 音效（exe 同目录的 res 子目录）
	void loadConfig();                        // 从与exe同名的ini读取设置
	void saveConfig();                        // 保存设置到ini
	std::wstring configPath() const;          // 得到ini完整路径
	void doRandomMove();
	void doSimpleMove();               // 净发展评估落子（简单难度）
	void doAIMove();                   // 根据当前难度选择调用具体 AI
	void doHybridMove();   // 混合算法（启发式+蒙特卡洛）
	void doHeuristicMove();
	void doMonteCarloMove(int simulations);  // 蒙特卡洛模拟
};
#endif