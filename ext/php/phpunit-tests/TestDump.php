<?php

if (!extension_loaded('syck'))
    dl('syck.so');

date_default_timezone_set('GMT');

require_once "PHPUnit/Framework/TestCase.php";
require 'helpers.php';

error_reporting(E_ALL | E_STRICT);

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
        $this->assertEquals($obj->format('U'), syck_load(syck_dump($obj))->format('U'));
    }

    public function testNumericStrings()
    {
        $obj = '73,123';
        $this->assertSame($obj, syck_load(syck_dump($obj)));
    }

    public function testLongStrings()
    {
        $obj = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\nUt enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\nDuis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.\nExcepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
        $this->assertSame($obj, syck_load(syck_dump($obj)));
    }

    public function testLargeArrays()
    {
        $obj = range(1, 10000);
        $this->assertSame($obj, syck_load(syck_dump($obj)));
    }
}
