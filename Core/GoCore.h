// GoCore.h
#ifndef GOCORE_H
#define GOCORE_H

#include <vector>
#include <string>
#include <stack>

enum class Cell {
	Empty,
	Black,
	White
};

class GoCore {
public:
	GoCore(int boardSize = 9);
	~GoCore() = default;

	void reset();
	bool placeStone(int row, int col);   // 落子，成功返回true
	bool pass();                         // 停一手，成功返回true
	bool undo();                         // 悔棋，成功返回true
	Cell getCell(int row, int col) const;
	bool isBlackTurn() const;
	int getCapturesBlack() const;
	int getCapturesWhite() const;
	bool isGameOver() const;
	std::string getWinner() const;
	int getBoardSize() const;
	int getLastMoveRow() const { return lastMoveRow; }
	int getLastMoveCol() const { return lastMoveCol; }
	// 新增公有辅助函数（供 AI 评估使用）
	std::vector<std::pair<int, int>> findGroup(int row, int col) const;
	int countLiberties(const std::vector<std::pair<int, int>>& group) const;
	int assessMove(int row, int col) const;   // 评估空位落子的得分（针对白棋，攻守平衡）
	int assessMoveSimple(int row, int col) const;  // 简化评估（净发展风格：提子/己方气/连接/中心，不追击不逃命）
	bool whiteInAtari() const;            // 盘上是否存在 1 气白块（关键手保护：此时禁次优随机）
	int weakSaveLevel(int row, int col) const;  // 该落点对最弱 1 气白块的补救档位（0无 1缓解 2救活）
	bool hasLegalMove() const;                // 是否存在任一合法落子点（含劫/自杀排除）
	struct MoveRecord {                       // 落子记录（棋谱用）
		int row;          // -1 表示 Pass
		int col;
		bool isBlack;     // true=黑 / false=白
		MoveRecord(int r, int c, bool b) : row(r), col(c), isBlack(b) {}
	};
	const std::vector<MoveRecord>& getMoveLog() const { return moveLog; }
	std::string exportSGF() const;            // 导出 SGF 文本（含对局序列与结果）
	bool loadSGF(const std::string& text, std::string& errMsg);  // 导入 SGF 并重放（失败返回 false）

private:
	// GoCore.h (在 HistoryState 中添加 koPoint)
	struct HistoryState {
		std::vector<std::vector<Cell>> board;
		bool blackTurn;
		int capturesBlack;
		int capturesWhite;
		int passCount;
		std::pair<int, int> koPoint;   // 新增
		int lastMoveRow;               // 新增：保存该状态下的最后落子标记
		int lastMoveCol;
	};

	// 在 class GoCore 中添加私有成员
	std::pair<int, int> koPoint;       // 当前劫点，(-1,-1)表示无劫

	int boardSize;
	int lastMoveRow;
	int lastMoveCol;
	std::vector<std::vector<Cell>> board;
	bool blackTurn;
	int capturesBlack;
	int capturesWhite;
	int passCount;
	std::stack<HistoryState> history;   // 存储历史状态用于悔棋
	std::vector<MoveRecord> moveLog;    // 全程落子记录（用于导出棋谱）

	bool isInBoard(int row, int col) const;
	std::vector<std::pair<int, int>> findConnected(int row, int col, Cell color) const;
	void saveState();                    // 保存当前状态到历史
	void restoreState(const HistoryState& state);
};

#endif