CREATE TABLE news_items
(
	id int NOT NULL AUTO_INCREMENT,
	date datetime NOT NULL,
	author varchar(32) NOT NULL,
	title varchar(64) NOT NULL,
	text varchar(4096) NOT NULL,
	PRIMARY KEY(id)
);
