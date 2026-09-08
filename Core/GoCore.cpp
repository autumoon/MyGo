#include "../Core/GoCore.h"
#include <queue>

GoCore::GoCore(int boardSize /*= 9*/)
	: boardSize(boardSize), blackTurn(true), capturesBlack(0), capturesWhite(0), passCount(0),
	koPoint(-1, -1), lastMoveRow(-1), lastMoveCol(-1)   // 添加
{
	reset();
}

bool GoCore::isInBoard(int row, int col) const {
    return row >= 0 && row < boardSize && col >= 0 && col < boardSize;
}

std::vector<std::pair<int,int>> GoCore::findConnected(int row, int col, Cell color) const {
    std::vector<std::pair<int,int>> result;
    if (!isInBoard(row, col) || board[row][col] != color)
        return result;

    std::vector<std::vector<bool>> visited(boardSize, std::vector<bool>(boardSize, false));
    std::queue<std::pair<int,int>> q;

    q.push({row, col});
    visited[row][col] = true;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    while (!q.empty()) {
        std::pair<int,int> p = q.front();
        q.pop();
        int r = p.first;
        int c = p.second;
        result.push_back({r, c});

        for (int i = 0; i < 4; ++i) {
            int nr = r + dr[i];
            int nc = c + dc[i];
            if (isInBoard(nr, nc) && !visited[nr][nc] && board[nr][nc] == color) {
                visited[nr][nc] = true;
                q.push({nr, nc});
            }
        }
    }

    return result;
}

std::vector<std::pair<int, int>> GoCore::findGroup(int row, int col) const {
	if (!isInBoard(row, col) || board[row][col] == Cell::Empty)
		return std::vector<std::pair<int, int>>();
	return findConnected(row, col, board[row][col]);
}

int GoCore::countLiberties(const std::vector<std::pair<int,int>>& group) const {
    int liberties = 0;
    if (group.empty()) return 0;

    std::vector<std::vector<bool>> visited(boardSize, std::vector<bool>(boardSize, false));
    std::queue<std::pair<int,int>> q;
    std::pair<int,int> start = group[0];
    q.push(start);
    visited[start.first][start.second] = true;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    while (!q.empty()) {
        std::pair<int,int> p = q.front();
        q.pop();
        int r = p.first;
        int c = p.second;

        for (int i = 0; i < 4; ++i) {
            int nr = r + dr[i];
            int nc = c + dc[i];
            if (!isInBoard(nr, nc)) continue;

            if (board[nr][nc] == Cell::Empty) {
                ++liberties;
            }
            else if (board[nr][nc] == board[start.first][start.second] && !visited[nr][nc]) {
                visited[nr][nc] = true;
                q.push({nr, nc});
            }
        }
    }

    return liberties;
}

Cell GoCore::getCell(int row, int col) const {
    if (!isInBoard(row, col))
        return Cell::Empty;
    return board[row][col];
}

bool GoCore::isBlackTurn() const {
    return blackTurn;
}

int GoCore::getCapturesBlack() const {
    return capturesBlack;
}

int GoCore::getCapturesWhite() const {
    return capturesWhite;
}

bool GoCore::isGameOver() const {
	// 吃5子获胜：任意一方提子数达到5即结束
	if (capturesBlack >= 5 || capturesWhite >= 5)
		return true;
	// 平局：双方连续停一手（无棋可下或双方同意终局）
	return passCount >= 2;
}

std::string GoCore::getWinner() const {
	if (capturesBlack >= 5)
		return "黑棋获胜 (吃5子)";
	else if (capturesWhite >= 5)
		return "白棋获胜 (吃5子)";
	else if (isGameOver())
		return "平局";
	else
		return "游戏未结束";
}

bool GoCore::hasLegalMove() const {
	if (isGameOver()) return false;
	for (int r = 0; r < boardSize; ++r) {
		for (int c = 0; c < boardSize; ++c) {
			if (board[r][c] != Cell::Empty) continue;
			// 直接用临时副本尝试落子，判断是否合法（含提子/自杀/劫检查）
			GoCore sim = *this;
			if (sim.placeStone(r, c)) return true;
		}
	}
	return false;
}

int GoCore::getBoardSize() const {
    return boardSize;
}

// ---------- 新增函数 ----------
void GoCore::saveState() {
	HistoryState state;
	state.board = board;
	state.blackTurn = blackTurn;
	state.capturesBlack = capturesBlack;
	state.capturesWhite = capturesWhite;
	state.passCount = passCount;
	state.koPoint = koPoint;     // 保存
	state.lastMoveRow = lastMoveRow;   // 保存最后落子标记
	state.lastMoveCol = lastMoveCol;
	history.push(state);
}

void GoCore::restoreState(const HistoryState& state) {
	board = state.board;
	blackTurn = state.blackTurn;
	capturesBlack = state.capturesBlack;
	capturesWhite = state.capturesWhite;
	passCount = state.passCount;
	koPoint = state.koPoint;     // 恢复
	lastMoveRow = state.lastMoveRow;   // 恢复最后落子标记
	lastMoveCol = state.lastMoveCol;
}

bool GoCore::pass() {
	if (isGameOver()) return false;
	moveLog.push_back(MoveRecord(-1, -1, blackTurn));   // 记录 Pass（当前走棋方）
	saveState();   // 记录当前状态（包括pass操作）
	blackTurn = !blackTurn;
	passCount++;
	lastMoveRow = -1;
	lastMoveCol = -1;
	// 如果连续两次Pass，游戏结束，但不需要额外操作
	return true;
}

bool GoCore::undo() {
	if (history.empty()) return false;
	// 弹出历史，恢复状态（不保存当前状态，因为悔棋是撤销上一步）
	HistoryState state = history.top();
	history.pop();
	restoreState(state);
	if (!moveLog.empty()) moveLog.pop_back();   // 同步移除棋谱记录
	return true;
}

bool GoCore::placeStone(int row, int col) {
	if (!isInBoard(row, col)) return false;
	if (board[row][col] != Cell::Empty) return false;

	// 备份所有状态
	auto backupBoard = board;
	auto backupTurn = blackTurn;
	auto backupCapB = capturesBlack;
	auto backupCapW = capturesWhite;
	auto backupPass = passCount;
	auto backupKo = koPoint;

	Cell placingColor = blackTurn ? Cell::Black : Cell::White;
	Cell oppositeColor = (placingColor == Cell::Black) ? Cell::White : Cell::Black;

	board[row][col] = placingColor;

	// 提掉所有无气的对方棋子，并记录被提的位置
	std::vector<std::pair<int, int>> capturedPositions;
	for (int i = 0; i < boardSize; ++i) {
		for (int j = 0; j < boardSize; ++j) {
			if (board[i][j] == oppositeColor) {
				auto group = findConnected(i, j, oppositeColor);
				if (!group.empty() && countLiberties(group) == 0) {
					for (auto& p : group) {
						board[p.first][p.second] = Cell::Empty;
						capturedPositions.push_back(p);
					}
				}
			}
		}
	}

	int captured = static_cast<int>(capturedPositions.size());

	// 更新提子数
	if (placingColor == Cell::Black)
		capturesBlack += captured;
	else
		capturesWhite += captured;

	// 检查刚落子的棋子是否有气
	auto myGroup = findGroup(row, col);
	if (myGroup.empty() || countLiberties(myGroup) == 0) {
		// 非法落子，恢复所有备份
		board = backupBoard;
		blackTurn = backupTurn;
		capturesBlack = backupCapB;
		capturesWhite = backupCapW;
		passCount = backupPass;
		koPoint = backupKo;
		return false;
	}

	// ---------- 劫争检测（移到这里） ----------
	// 如果上一步有劫点，且本次落子位置就是劫点，且只提掉一个子，则禁止
	if (backupKo.first != -1 && backupKo.second != -1 &&
		row == backupKo.first && col == backupKo.second &&
		captured == 1) {
		// 非法劫争，恢复备份
		board = backupBoard;
		blackTurn = backupTurn;
		capturesBlack = backupCapB;
		capturesWhite = backupCapW;
		passCount = backupPass;
		koPoint = backupKo;
		return false;
	}

	// ---------- 更新劫点 ----------
	if (captured == 1) {
		koPoint = capturedPositions[0];   // 只提一子，记录劫点
	}
	else {
		koPoint = { -1, -1 };             // 其他情况清空劫点
	}

	// 保存历史（落子前的状态）
	HistoryState state;
	state.board = backupBoard;
	state.blackTurn = backupTurn;
	state.capturesBlack = backupCapB;
	state.capturesWhite = backupCapW;
	state.passCount = backupPass;
	state.koPoint = backupKo;
	state.lastMoveRow = lastMoveRow;
	state.lastMoveCol = lastMoveCol;
	history.push(state);

blackTurn = !blackTurn;
	passCount = 0;
	lastMoveRow = row;
	lastMoveCol = col;
	moveLog.push_back(MoveRecord(row, col, placingColor == Cell::Black));   // 记录本次落子
	return true;
}
// 修改 reset，清空历史
void GoCore::reset() {
	board.assign(boardSize, std::vector<Cell>(boardSize, Cell::Empty));
	blackTurn = true;
	capturesBlack = 0;
	capturesWhite = 0;
	passCount = 0;
	koPoint = { -1, -1 };          // 重置
	lastMoveRow = -1;
	lastMoveCol = -1;
	while (!history.empty()) history.pop();
	moveLog.clear();               // 清空棋谱记录
}

// GoCore.cpp 新增
int GoCore::assessMove(int row, int col) const {
	if (!isInBoard(row, col) || board[row][col] != Cell::Empty)
		return -1000000;   // 非法位置

	const int dr[4] = { -1, 1, 0, 0 };
	const int dc[4] = { 0, 0, -1, 1 };

	// ---------- 落子前：统计邻近黑块的气（用于防守-紧气评估）----------
	struct EnemyBlock {
		std::pair<int, int> seed;   // 块的任意起点
		int libsBefore;
	};
	std::vector<EnemyBlock> enemyBlocks;
	for (int d = 0; d < 4; ++d) {
		int nr = row + dr[d];
		int nc = col + dc[d];
		if (isInBoard(nr, nc) && board[nr][nc] == Cell::Black) {
			auto group = findGroup(nr, nc);
			if (group.empty()) continue;
			bool duplicate = false;
			for (auto& b : enemyBlocks)
				if (b.seed == group[0]) { duplicate = true; break; }
			if (!duplicate) {
				enemyBlocks.push_back({ group[0], countLiberties(group) });
			}
		}
	}

	// 落子前：全盘扫描最弱的己方白块（用于全局逃命响应）
	// 该块气越少越危险；若盘上有 1 气白块，任何不救它的落子都意味着它将被提
	std::vector<std::pair<int, int>> scannedOwn;
	int weakestLibs = boardSize * boardSize + 1;
	int weakestSize = 0;
	std::pair<int, int> weakestSeed(-1, -1);
	for (int r = 0; r < boardSize; ++r) {
		for (int col = 0; col < boardSize; ++col) {
			if (board[r][col] != Cell::White) continue;
			auto g = findGroup(r, col);
			if (g.empty()) continue;
			bool dup = false;
			for (auto& s : scannedOwn) { if (s == g[0]) { dup = true; break; } }
			if (dup) continue;
			scannedOwn.push_back(g[0]);
			int libs = countLiberties(g);
			if (libs < weakestLibs) {
				weakestLibs = libs;
				weakestSize = static_cast<int>(g.size());
				weakestSeed = g[0];
			}
		}
	}

	// 落子前：判断最弱濒死块（1 气）是否存在"任何可救点"（能把它气 +1）
	// 若全盘根本没有任何点能救（已被黑彻底围死），"放任"罚就无意义——
	// 不罚，避免把远处无关的开阔点也全盘拖成负分、误触发 Pass 弃棋（实测 —80×size 时整盘 max 为负）。
	// 只有"明明能救却不救"的点才重罚，这才把最高分让给真实救援点。
	bool rescuePossible = false;
	if (weakestSeed.first != -1 && weakestLibs <= 1) {
		for (int rr = 0; rr < boardSize && !rescuePossible; ++rr)
			for (int cc = 0; cc < boardSize && !rescuePossible; ++cc)
				if (board[rr][cc] == Cell::Empty) {
					GoCore probe = *this;
					if (!probe.placeStone(rr, cc)) continue;
					auto pwg = probe.findGroup(weakestSeed.first, weakestSeed.second);
					int pwl = pwg.empty() ? 0 : probe.countLiberties(pwg);
					if (pwl >= weakestLibs + 1) rescuePossible = true;
				}
	}

	// 创建临时副本进行模拟
	GoCore sim = *this;    // 默认拷贝构造可用
						   // AI 执白棋（玩家执黑）
	if (!sim.placeStone(row, col)) {
		return -1000000;   // 落子失败（自杀/劫等）
	}

	// 1. 提子奖励（吃子数增量）
	int captureGain = sim.getCapturesWhite() - this->getCapturesWhite();
	int score = captureGain * 100;

	// 2. 打吃奖励（落子后邻近黑块气变为1）
	int atariCount = 0;
	std::vector<std::pair<int, int>> checkedGroups;  // 记录已检查组，防重复
	for (int d = 0; d < 4; ++d) {
		int nr = row + dr[d];
		int nc = col + dc[d];
		if (isInBoard(nr, nc) && sim.getCell(nr, nc) == Cell::Black) {
			auto group = sim.findGroup(nr, nc);
			if (group.empty()) continue;
			bool duplicate = false;
			for (auto& g : checkedGroups) {
				if (g == group[0]) { duplicate = true; break; }
			}
			if (!duplicate) {
				checkedGroups.push_back(group[0]);
				if (sim.countLiberties(group) == 1) atariCount++;
			}
		}
	}
	score += atariCount * 30;

	// 3. 防守-紧气奖励：挤压邻近黑块的生存空间
	//    黑块原本气越少（越危险、越该被攻），紧气价值越高（威胁权重）
	for (auto& b : enemyBlocks) {
		int libsAfter;
		if (sim.getCell(b.seed.first, b.seed.second) == Cell::Black) {
			auto g = sim.findGroup(b.seed.first, b.seed.second);
			libsAfter = g.empty() ? 0 : sim.countLiberties(g);
		}
		else {
			libsAfter = 0;   // 己方提掉了该块
		}
		int squeezed = b.libsBefore - libsAfter;      // 气被挤压的量
		if (squeezed > 0) {
			int threat = (b.libsBefore <= 2) ? 2 : 1; // 本就快死的大龙加倍施压
			score += squeezed * 25 * threat;
		}
		else if (b.libsBefore == 1 && libsAfter == 1) {
			// 已是单气被紧贴：虽未减少气，但继续占位牵制
			score += 15;
		}
	}

	// 4. 防守-截断联络：落子点在黑棋势力中间，阻止敌方成大龙
	int adjBlack = 0, adjWhite = 0;
	for (int d = 0; d < 4; ++d) {
		int nr = row + dr[d];
		int nc = col + dc[d];
		if (!isInBoard(nr, nc)) continue;
		if (sim.getCell(nr, nc) == Cell::Black) adjBlack++;
		else if (sim.getCell(nr, nc) == Cell::White) adjWhite++;
	}
	score += adjBlack * 10;     // 深入敌方腹地，破坏潜在联络

	// 5. 进攻-己方气数奖励（落子后己方棋子的气）
	auto myGroup = sim.findGroup(row, col);
	int myLibs = sim.countLiberties(myGroup);
	score += myLibs * 5;

	// 8. 己方濒死块营救（全局逃命响应，优先级高于截断/连接）：
	//    盘上最弱白块气<=2 时，本手把它救活/缓解 → 高额加分；
	//    若最弱白块仅 1 气（已被叫吃）而本手又不能救它 → 该块必被黑下一手提掉，重罚
	if (weakestSeed.first != -1 && weakestLibs <= 2) {
		int wLibsAfter;
		auto wg = sim.findGroup(weakestSeed.first, weakestSeed.second);
		wLibsAfter = wg.empty() ? 0 : sim.countLiberties(wg);
		if (wLibsAfter >= weakestLibs + 2) {
			score += weakestSize * 80;      // 救活：气明显变多（权重 60→80，防困难 MC 随机翻转）
		}
		else if (wLibsAfter == weakestLibs + 1) {
			score += weakestSize * 40;      // 缓解（如 1气→2气）
		}
		else if (weakestLibs <= 1 && wLibsAfter <= weakestLibs && rescuePossible) {
			score -= weakestSize * 40;      // 能救不救，按块大小惩罚（80→40：避免个别小块把全盘打成负分、误触发 Pass 弃棋）
		}
	}

	// 6. 进攻-连接增援：紧邻己方棋块，报团成势
	score += adjWhite * 12;

	// 7. 中心偏好（越靠近中心加分）
	float center = (boardSize - 1) / 2.0f;
	float dist = sqrtf((row - center) * (row - center) + (col - center) * (col - center));
	float centerBonus = std::max(0.0f, 10.0f - dist * 2.0f);
	score += static_cast<int>(centerBonus);

	// 9. 送死角/低气惩罚：落子后己方块只剩 1 气、无提子收益、且贴着对方棋子
	//    → 黑下一手随手就能提掉（评估只看落子瞬时，见不到对方反提；
	//      打吃+濒死紧气可瞬时堆到 200+ 分，罚 80 压不住，故重罚）
	if (myLibs <= 1 && captureGain == 0 && adjBlack >= 1) {
		score -= 150;
	}

	return score;
}

// ---------- 白方叫吃/救援查询（关键手保护）----------
// 返回盘上第一个 1 气白块的 seed 与该块大小；无则 seed=(-1,-1)
static std::pair<int, int> findAtariWhiteBlock(const GoCore& g, int& size) {
	int n = g.getBoardSize();
	for (int r = 0; r < n; ++r)
		for (int c = 0; c < n; ++c)
			if (g.getCell(r, c) == Cell::White) {
				auto grp = g.findGroup(r, c);
				if (grp.empty()) continue;
				if (g.countLiberties(grp) == 1) {
					size = static_cast<int>(grp.size());
					return grp[0];
				}
			}
	size = 0;
	return std::make_pair(-1, -1);
}

bool GoCore::whiteInAtari() const {
	int sz = 0;
	return findAtariWhiteBlock(*this, sz).first != -1;
}

int GoCore::weakSaveLevel(int row, int col) const {
	if (!isInBoard(row, col) || board[row][col] != Cell::Empty) return 0;
	int sz = 0;
	std::pair<int, int> seed = findAtariWhiteBlock(*this, sz);
	if (seed.first == -1) return 0;
	GoCore sim = *this;
	if (!sim.placeStone(row, col)) return 0;
	auto wg = sim.findGroup(seed.first, seed.second);
	int wl = wg.empty() ? 0 : sim.countLiberties(wg);
	if (wl >= 3) return 2;   // 1 气 → ≥3 气：救活
	if (wl == 2) return 1;   // 1 气 → 2 气：缓解
	return 0;
}

// ---------- 简化评估：早期"净发展"风格 ----------
// 只从自身发展的角度打分（提子 / 己方气 / 连接抱团 / 中心偏好），
// 不包含打吃、截断、紧气、全局逃命等攻防计算 —— 对应早期未优化的中等算法，
// 供「简单」难度使用（有棋感但不追击、不知追杀逃生）。
int GoCore::assessMoveSimple(int row, int col) const {
	if (!isInBoard(row, col) || board[row][col] != Cell::Empty)
		return -1000000;   // 非法位置

	const int dr[4] = { -1, 1, 0, 0 };
	const int dc[4] = { 0, 0, -1, 1 };

	GoCore sim = *this;
	if (!sim.placeStone(row, col)) {
		return -1000000;   // 落子失败（自杀/劫等）
	}

	// 1. 提子奖励
	int score = (sim.getCapturesWhite() - this->getCapturesWhite()) * 100;

	// 2. 己方气数（落子后所在块的气）
	auto myGroup = sim.findGroup(row, col);
	int myLibs = sim.countLiberties(myGroup);
	score += myLibs * 5;

	// 3. 连接抱团（紧邻己方子数）
	int adjWhite = 0;
	for (int d = 0; d < 4; ++d) {
		int nr = row + dr[d];
		int nc = col + dc[d];
		if (isInBoard(nr, nc) && sim.getCell(nr, nc) == Cell::White) adjWhite++;
	}
	score += adjWhite * 12;

	// 4. 中心偏好
	float center = (boardSize - 1) / 2.0f;
	float dist = sqrtf((row - center) * (row - center) + (col - center) * (col - center));
	float centerBonus = std::max(0.0f, 10.0f - dist * 2.0f);
	score += static_cast<int>(centerBonus);

	return score;
}

// ---------- SGF 棋谱导出 ----------
std::string GoCore::exportSGF() const {
	std::string s = "(;GM[1]FF[4]CA[UTF-8]SZ[" + std::to_string(boardSize) + "]PB[玩家]PW[AI]";
	if (isGameOver()) {
		if (capturesBlack >= 5) s += "RE[B+]";
		else if (capturesWhite >= 5) s += "RE[W+]";
		else s += "RE[Draw]";
	}
	else {
		s += "RE[?]";   // 未结束
	}
	for (const auto& m : moveLog) {
		s += m.isBlack ? ";B[" : ";W[";
		if (m.row < 0) {
			s += "]";   // Pass
		}
		else {
			s += (char)('a' + m.col);
			s += (char)('a' + m.row);
			s += "]";
		}
	}
	s += ")";
	return s;
}

// ---------- SGF 棋谱导入 ----------
bool GoCore::loadSGF(const std::string& text, std::string& errMsg) {
	reset();
	int moveCount = 0;
	size_t pos = 0;
	for (;;) {
		size_t b = text.find(";B[", pos);
		size_t w = text.find(";W[", pos);
		size_t mv;
		if (b == std::string::npos && w == std::string::npos) break;
		if (b == std::string::npos) mv = w;
		else if (w == std::string::npos) mv = b;
		else mv = (b < w) ? b : w;

		size_t open = mv + 3;                 // 坐标内容起点（mv+1='B'，mv+2='['）
		size_t close = text.find(']', open);
		if (close == std::string::npos) break;
		std::string cc = text.substr(open, close - open);
		pos = close + 1;
		++moveCount;

		if (cc.empty()) {
			// Pass（黑白交替由 blackTurn 决定，容错不校验颜色）
			if (!pass()) {
				errMsg = "第 " + std::to_string(moveCount) + " 手 Pass 无效";
				return false;
			}
			continue;
		}
		if (cc.size() < 2) {
			errMsg = "第 " + std::to_string(moveCount) + " 手坐标非法";
			return false;
		}
		int col = cc[0] - 'a';
		int row = cc[1] - 'a';
		if (col < 0 || col >= boardSize || row < 0 || row >= boardSize) {
			errMsg = "第 " + std::to_string(moveCount) + " 手坐标越界（当前 "
				+ std::to_string(boardSize) + " 路棋盘）";
			return false;
		}
		if (!placeStone(row, col)) {
			errMsg = "第 " + std::to_string(moveCount) + " 手落子非法（位置被占/自杀/打劫）";
			return false;
		}
	}
	return true;
}

