CREATE TABLE scantable (
    id SERIAL PRIMARY KEY,
    name TEXT
);

INSERT INTO scantable (name) VALUES ('Alice');
INSERT INTO scantable (name) VALUES ('Bob');
INSERT INTO scantable (name) VALUES (NULL);

SELECT full_table_scan('scantable');

SELECT table_scan_with_scankeys('scantable');

CREATE INDEX scantable_id_name_idx ON scantable(id, name);
SELECT table_scan_with_index('scantable', 'scantable_id_name_idx');

SELECT table_scan_and_sort_attribute('scantable', 'id');
SELECT table_scan_and_sort_attribute('scantable', 'name');
