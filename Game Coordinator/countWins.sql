SELECT COUNT(users.id) FROM users, particpations, teams, (SELECT teams.id AS [WINNING_TEAMS], MAX(teams.score) AS [WINNING_SCORE] FROM games,particpations,teams,users
WHERE games.id = teams.gameid AND teams.id = particpations.teamid AND particpations.playerId = users.id
GROUP BY games.id)
WHERE users.id = particpations.playerId AND particpations.teamid = teams.id AND teams.id = WINNING_TEAMS
AND users.id = %i;