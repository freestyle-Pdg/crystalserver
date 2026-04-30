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

enum ThreadState {
	THREAD_STATE_RUNNING,
	THREAD_STATE_CLOSING,
	THREAD_STATE_TERMINATED,
};

template <typename Derived>
class ThreadHolder {
public:
	ThreadHolder() { }
	void start() {
		setState(THREAD_STATE_RUNNING);
		thread = std::thread(&Derived::threadMain, static_cast<Derived*>(this));
	}

	void stop() {
		setState(THREAD_STATE_CLOSING);
	}

	void join() {
		if (thread.joinable()) {
			thread.join();
		}
	}

protected:
	void setState(ThreadState newState) {
		threadState.store(newState, std::memory_order_relaxed);
	}

	ThreadState getState() const {
		return threadState.load(std::memory_order_relaxed);
	}

private:
	std::atomic<ThreadState> threadState { THREAD_STATE_TERMINATED };
	std::thread thread;
};
