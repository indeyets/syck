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
  hash: {key: value, key2: "value 2", key3: 3, key4: null}
  array: [value, "value 2", 3, null]
EOY;

var_dump(syck_load($yaml));
?>
--EXPECT--
array(1) {
  ["test"]=>
  array(2) {
    ["hash"]=>
    array(4) {
      ["key"]=>
      string(5) "value"
      ["key2"]=>
      string(7) "value 2"
      ["key3"]=>
      int(3)
      ["key4"]=>
      NULL
    }
    ["array"]=>
    array(4) {
      [0]=>
      string(5) "value"
      [1]=>
      string(7) "value 2"
      [2]=>
      int(3)
      [3]=>
      NULL
    }
  }
}
