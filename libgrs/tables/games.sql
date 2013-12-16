CREATE TABLE games
(
	id int NOT NULL AUTO_INCREMENT,
	name varchar(64),
	PRIMARY KEY(id)
);

CREATE TABLE gamemodes
(
	id int NOT NULL AUTO_INCREMENT,
	debug tinyint NOT NULL,
	game_id int NOT NULL,
	mode int,
	name varchar(64),
	PRIMARY KEY(id)
);

CREATE INDEX gamemode_mode on gamemodes (mode);

CREATE TABLE scores
(
	id int NOT NULL AUTO_INCREMENT,
	game_id int NOT NULL,
	game_mode int NOT NULL,
	game_version int NOT NULL,
	date datetime NOT NULL,
	user_id varchar(256) NOT NULL,
	score real NOT NULL,
	user_name varchar(32) NOT NULL,
	tag varchar(64) NOT NULL,
	country_code varchar(8) NOT NULL,
	is_hacked bit NOT NULL,
	hash varchar(32) NOT NULL,
	PRIMARY KEY(id)
);

CREATE INDEX score_game_id on scores (game_id);
CREATE INDEX score_game_mode on scores (game_mode);
CREATE INDEX score_score on scores (score);
CREATE INDEX score_hash on scores (hash);

CREATE TABLE crash_logs
(
	id int NOT NULL AUTO_INCREMENT,
	game_id int NOT NULL,
	game_version int NOT NULL,
	date datetime NOT NULL,
	message varchar(256) NOT NULL,
	hash varchar(32) NOT NULL,
	PRIMARY KEY(id)
);
