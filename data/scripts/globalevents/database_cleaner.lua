local config = {
	deleteAccountWithNoPlayers = true,
	printResult = true,
	saveResultToFile = true,
	logFileName = "cleanplayers.txt",
}

local cleanup = {
	{ level = 8, time = 30 * 24 * 60 * 60 },
	{ level = 50, time = 90 * 24 * 60 * 60 },
	{ level = 100, time = 180 * 24 * 60 * 60 },
	{ level = 200, time = 360 * 24 * 60 * 60 },
	{ level = 300, time = 720 * 24 * 60 * 60 },
	{ level = 400, time = 1095 * 24 * 60 * 60 },
	{ level = 500, time = 1460 * 24 * 60 * 60 },
}

local function queryCount(query)
	local result = db.storeQuery(query)
	if not result then
		return 0
	end
	local n = Result.getNumber(result, "count")
	Result.free(result)
	return n
end

local function getPlayerCount()
	return queryCount("SELECT COUNT(`id`) AS `count` FROM `players`;")
end

local function getAccountCount()
	return queryCount("SELECT COUNT(`id`) AS `count` FROM `accounts`;")
end

-- ─── startup event ───────────────────────────────────────────────────────────
local playerCleaner = GlobalEvent("PlayerCleaner")
playerCleaner:type("startup")

function playerCleaner.onStartup()
	local beforePlayers = getPlayerCount()
	local beforeAccounts = getAccountCount()

	local pids = {}

	local ignoredNames = {
		"Rook Sample",
		"Sorcerer Sample",
		"Druid Sample",
		"Paladin Sample",
		"Knight Sample",
		"Monk Sample",
		"GOD",
	}
	local ignoredList = "'" .. table.concat(ignoredNames, "','") .. "'"

	if not configManager.getBoolean(configKeys.CLEAN_DATABASE) then
		return
	end

	for _, tier in ipairs(cleanup) do
		local query = ("SELECT `id`, `account_id` FROM `players`" .. " WHERE `level` < " .. tier.level .. " AND `name` NOT IN(" .. ignoredList .. ")" .. " AND `group_id` < 2" .. " AND `lastlogin` < UNIX_TIMESTAMP() - " .. tier.time .. " AND `lastlogin` > 0;")

		local result = db.storeQuery(query)
		if result then
			repeat
				local pid = Result.getNumber(result, "id")
				local aid = Result.getNumber(result, "account_id")
				pids[pid] = aid
			until not Result.next(result)
			Result.free(result)
		end
	end

	-- ── delete players ────────────────────────────────────────────────────────
	for pid, _ in pairs(pids) do
		db.query(("DELETE FROM `znote_players` WHERE `player_id` = %d;"):format(pid))
		db.query(("DELETE FROM `players` WHERE `id` = %d;"):format(pid))
	end

	-- ── delete empty accounts ─────────────────────────────────────────────────
	if config.deleteAccountWithNoPlayers then
		local aidsProcessed = {}

		for _, aid in pairs(pids) do
			if not aidsProcessed[aid] then
				aidsProcessed[aid] = true

				local cnt = queryCount(("SELECT COUNT(`id`) AS `count` FROM `players` WHERE `account_id` = %d;"):format(aid))

				if cnt <= 0 then
					db.query(("DELETE FROM `znote_accounts` WHERE `account_id` = %d;"):format(aid))
					db.query(("DELETE FROM `accounts` WHERE `id` = %d;"):format(aid))
				end
			end
		end
	end

	-- ── report ────────────────────────────────────────────────────────────────
	local deletedPlayers = beforePlayers - getPlayerCount()
	local deletedAccounts = beforeAccounts - getAccountCount()

	if deletedPlayers > 0 or deletedAccounts > 0 then
		local text = string.format(">> [DBCLEANUP] %d inactive player(s)%s deleted from the database.", deletedPlayers, config.deleteAccountWithNoPlayers and (" and " .. deletedAccounts .. " empty account(s)") or "")

		if config.printResult then
			print("")
			print(text)
			print("")
		end

		if config.saveResultToFile then
			local file = io.open("data/logs/" .. config.logFileName, "a")
			if file then
				file:write("[" .. os.date("%d %B %Y %X") .. "] " .. text .. "\n\n")
				file:close()
			end
		end
	end

	return true
end

playerCleaner:register()
