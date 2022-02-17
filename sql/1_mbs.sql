DROP TABLE IF EXISTS mbs;
CREATE TABLE mbs (
	hostname	varchar(64) NOT NULL UNIQUE,
	application	varchar(128),
	deleted		INT DEFAULT 0
) ENGINE = InnoDB;
