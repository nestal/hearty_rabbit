create table blob_table(
	id bytea primary key,
	original_filename text default null,
	mime text not null,
	upload_time timestamp default Now(),
	creation_time timestamp default Now(),
	meta json default null
);

create table user_table(
	id text primary key,
	display_name text,
	hashed_password bytea not null,
	salt bytea not null,
	iteration integer not null,
	algorithm text not null,
	meta json default null
);

create table album_table(
	owner text references user_table,
	name text not null,
	creation_time timestamp not null,
	meta json default null,
	primary key (owner, name)
);

create table album_blob(
    blob_id bytea references blob_table,
    owner text,
    album text,
    foreign key (owner, album) references album_table (owner, name)
);

