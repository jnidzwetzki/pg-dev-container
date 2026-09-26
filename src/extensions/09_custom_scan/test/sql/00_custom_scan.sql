-- The extension script only loads the library into the session that runs
-- CREATE EXTENSION, so load it into this session.
LOAD 'custom_scan';

CREATE TABLE customers (
    id SERIAL PRIMARY KEY,
    name TEXT,
    credit_card_number TEXT
);

INSERT INTO customers (name, credit_card_number) VALUES ('Alice', '4111-1111-1111-1111');
INSERT INTO customers (name, credit_card_number) VALUES ('Bob', '5500 0000 0000 0004');
INSERT INTO customers (name, credit_card_number) VALUES ('Carol', NULL);

-- A table without a credit card number column is not affected
CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    customer_id INTEGER,
    amount NUMERIC
);

INSERT INTO orders (customer_id, amount) VALUES (1, 42.00), (1, 13.37), (2, 99.99);

-- The credit card numbers are masked
SELECT * FROM customers ORDER BY id;
EXPLAIN (COSTS OFF) SELECT * FROM customers;
EXPLAIN (COSTS OFF) SELECT * FROM orders;

-- The quals are evaluated on the masked values
SELECT * FROM customers WHERE credit_card_number = '4111-1111-1111-1111';
SELECT * FROM customers WHERE credit_card_number LIKE '%1111';

-- An index on the column is not used, since it would return unmasked values
CREATE INDEX customers_ccn_idx ON customers (credit_card_number);
SET enable_seqscan = off;
EXPLAIN (COSTS OFF) SELECT credit_card_number FROM customers WHERE credit_card_number = '4111-1111-1111-1111';
RESET enable_seqscan;

-- Joins work as usual
EXPLAIN (COSTS OFF) SELECT c.name, c.credit_card_number, o.amount FROM customers c JOIN orders o ON c.id = o.customer_id;
SELECT c.name, c.credit_card_number, o.amount FROM customers c JOIN orders o ON c.id = o.customer_id ORDER BY o.id;

-- System columns are still available
SELECT ctid, tableoid::regclass, credit_card_number FROM customers ORDER BY id;

-- A rescan of the custom scan node (inner side of a nested loop join)
SET enable_hashjoin = off;
SET enable_mergejoin = off;
SET enable_material = off;
EXPLAIN (COSTS OFF) SELECT o.id, c.credit_card_number FROM orders o LEFT JOIN customers c ON c.id = o.customer_id;
SELECT o.id, c.credit_card_number FROM orders o LEFT JOIN customers c ON c.id = o.customer_id ORDER BY o.id;
RESET enable_hashjoin;
RESET enable_mergejoin;
RESET enable_material;

-- Limitations: statements that do not use the custom scan node see the stored values
COPY customers TO STDOUT;
COPY (SELECT * FROM customers ORDER BY id) TO STDOUT;
EXPLAIN (COSTS OFF) SELECT * FROM customers FOR UPDATE;
UPDATE customers SET name = name WHERE id = 1 RETURNING credit_card_number;
