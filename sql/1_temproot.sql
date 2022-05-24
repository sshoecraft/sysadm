DROP TABLE IF EXISTS temproot;
CREATE TABLE temproot (
	hostname	varchar(64),
	user		varchar(16),
	days		INT NOT NULL DEFAULT 14,
	updated		TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	PRIMARY KEY(hostname,user)
) ENGINE = InnoDB;