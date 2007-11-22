<?php

if (!extension_loaded('syck'))
    dl('syck.so');

require_once "PHPUnit/Framework/TestCase.php";

class TestMerge extends PHPUnit_Framework_TestCase
{
    public function testSimple()
    {
        $yaml = <<<YAML
---
define: &pointer_to_define
   - 1
   - 2
   - 3
reference: *pointer_to_define
YAML;

        $arr = syck_load($yaml);
        $this->assertEquals($arr['define'], $arr['reference']);
    }
}
