////////////////////////////////////////////////////////////////////////
// Crystal Server - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "utils/thread_holder_base.hpp"

class Task;

struct Stat {
	Stat(uint64_t _executionTime, const std::string &_description, const std::string &_extraDescription) :
		executionTime(_executionTime), description(_description), extraDescription(_extraDescription) {};
	uint64_t executionTime = 0;
	std::string description;
	std::string extraDescription;
};

struct statsData {
	statsData(uint32_t _calls, uint64_t _executionTime, const std::string &_extraInfo) :
		calls(_calls), executionTime(_executionTime), extraInfo(_extraInfo) { }
	uint32_t calls = 0;
	uint64_t executionTime = 0;
	std::string extraInfo;
};

using statsMap = std::map<std::string, statsData>;

class Stats : public ThreadHolder<Stats> {
public:
	void threadMain();
	void shutdown() {
		setState(THREAD_STATE_TERMINATED);
	}

	static Stats &getInstance();

	void addLuaStats(Stat* stats);
	void addSqlStats(Stat* stats);
	void addSpecialStats(Stat* stats);

	static uint32_t SLOW_EXECUTION_TIME;
	static uint32_t VERY_SLOW_EXECUTION_TIME;
	static int64_t DUMP_INTERVAL;

	std::atomic<uint32_t> playersOnline;

private:
	void parseLuaQueue(std::forward_list<Stat*> &queue);
	void parseSqlQueue(std::forward_list<Stat*> &queue);
	void parseSpecialQueue(std::forward_list<Stat*> &queue);
	static void writeSlowInfo(const std::string &file, uint64_t executionTime, const std::string &description, const std::string &extraDescription);
	void writeStats(const std::string &file, const statsMap &stats, const std::string &extraInfo = "") const;

	std::mutex statsLock;
	struct
	{
		std::forward_list<Stat*> queue;
		statsMap stats;
		int64_t lastDump;
	} lua, sql, special;
};

constexpr auto g_stats = Stats::getInstance;

class AutoStat {
public:
	explicit AutoStat(const std::string &description, const std::string &extraDescription = "") :
		time_point(std::chrono::high_resolution_clock::now()), stat(new Stat(0, description, extraDescription)) { }

	~AutoStat() {
		stat->executionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_point)
								  .count();
		stat->executionTime -= minusTime;
		g_stats().addSpecialStats(stat);
	}

protected:
	uint64_t minusTime = 0;
	std::chrono::high_resolution_clock::time_point time_point;

private:
	Stat* stat;
};

class AutoStatRecursive : public AutoStat {
public:
	explicit AutoStatRecursive(const std::string &description, const std::string &extraDescription = "") :
		AutoStat(description, extraDescription) {
		parent = activeStat;
		activeStat = this;
	}
	~AutoStatRecursive() {
		assert(activeStat == this);
		activeStat = parent;
		if (activeStat) {
			activeStat->minusTime += std::chrono::duration_cast<std::chrono::nanoseconds>(
										 std::chrono::high_resolution_clock::now() - time_point
			)
										 .count();
		}
	}

private:
	static AutoStatRecursive* activeStat;
	AutoStatRecursive* parent;
};
