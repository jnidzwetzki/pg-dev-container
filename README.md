# pg-dev-container


### Import PostgreSQL source code into VSCode
```
code --add /usr/local/src/postgresql
```

### Installing the first extension
```
cd src/extension/01_hello_world/
make
sudo make install
make installcheck

createdb test
psql test

SELECT * FROM pg_available_extensions;
CREATE EXTENSION hello_world;
\dx
\df+ hello_world
select hello_world('Mr X');

dropdb test
```