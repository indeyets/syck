<?php

if (!extension_loaded('syck'))
    dl('syck.so');

require_once "PHPUnit/Framework/TestCase.php";
require 'helpers.php';

error_reporting(E_ALL);

class TestDump extends PHPUnit_Framework_TestCase
{
    public function testNull()
    {
        $arr = array('qq' => null);
        $this->assertEquals($arr, syck_load(syck_dump($arr)));
    }

    public function testArray()
    {
        $arr = array('a', 'b', 'c', 'd');
        $string = syck_dump($arr);
        $arr2 = syck_load($string);

        $this->assertEquals($arr2, $arr);
    }

    public function testMovedArray()
    {
        $arr = array_flip(range(1, 1000));
        $string = syck_dump($arr);
        $arr2 = syck_load($string);

        $this->assertEquals($arr2, $arr);
    }

    public function testMixedArray()
    {
        $arr = array('test', 'a' => 'test2', 'test3');

        $string = syck_dump($arr);
        $arr2 = syck_load($string);

        $this->assertEquals($arr2, $arr);
    }

    public function testNegativeKeysArray()
    {
        $arr = array(-1 => 'test', -2 => 'test2', 0 => 'test3');

        $string = syck_dump($arr);
        $arr2 = syck_load($string);

        $this->assertEquals($arr2, $arr);
    }

    public function testSerializable()
    {
        $obj = new MySerializable('some string');
        $this->assertEquals($obj, syck_load(syck_dump($obj)));
    }

    public function testDatetime()
    {
        $obj = new DateTime();
        $this->assertEquals($obj, syck_load(syck_dump($obj)));
    }
}
