--TEST--
Check valid-yaml parsing
--SKIPIF--
<?php if (!extension_loaded("syck")) print "skip"; ?>
--POST--
--GET--
--INI--
--FILE--
<?php 
$yaml = <<<EOY
---
test:
  hash: {key: value; key2: "value 2"; key3: 3; key4: null}
  array: [value, "value 2", 3, null]
EOY;

try {
	syck_load($yaml);
	echo "loaded fine";
} catch (ErrorException $e) {
	echo $e->getMessage();
}
?>
--EXPECT--
syntax error, unexpected ':', expecting '}' or ','
