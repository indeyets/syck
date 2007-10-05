<?php

if (!extension_loaded('syck'))
    dl('syck.so');

require_once "PHPUnit/Framework/TestCase.php";
require 'helpers.php';

error_reporting(E_ALL);


class TestLoad extends PHPUnit_Framework_TestCase
{
    //
    // Common tests
    //

    public function testTest()
    {
        $this->assertEquals(1, 1);
    }

    public function testExtensionLoaded()
    {
        $this->assertTrue(function_exists('syck_load'));
        $this->assertTrue(function_exists('syck_dump'));
    }

    //
    // Simple-types tests
    //

    public function testNull()
    {
        $this->assertNull(syck_load("---\n"));
        $this->assertNull(syck_load(""));
        $this->assertNull(syck_load("~"));
        $this->assertNull(syck_load("null"));
        $this->assertNull(syck_load("Null"));
        $this->assertNull(syck_load("NULL"));
    }

    public function testBool()
    {
        $this->assertTrue(syck_load("yes"));
        $this->assertTrue(syck_load("Yes"));
        $this->assertTrue(syck_load("YES"));
        $this->assertTrue(syck_load("true"));
        $this->assertTrue(syck_load("True"));
        $this->assertTrue(syck_load("TRUE"));
        $this->assertTrue(syck_load("on"));
        $this->assertTrue(syck_load("On"));
        $this->assertTrue(syck_load("ON"));
        $this->assertTrue(syck_load("!bool y"));
        $this->assertTrue(syck_load("!bool Y"));

        $this->assertFalse(syck_load("no"));
        $this->assertFalse(syck_load("No"));
        $this->assertFalse(syck_load("NO"));
        $this->assertFalse(syck_load("false"));
        $this->assertFalse(syck_load("False"));
        $this->assertFalse(syck_load("FALSE"));
        $this->assertFalse(syck_load("off"));
        $this->assertFalse(syck_load("Off"));
        $this->assertFalse(syck_load("OFF"));
        $this->assertFalse(syck_load("!bool n"));
        $this->assertFalse(syck_load("!bool N"));
    }

    public function testString()
    {
        // basic string
        $this->assertSame(syck_load('a string'), 'a string');

        // number as a string
        $this->assertSame(syck_load('!str 12'), '12');
        $this->assertSame(syck_load('"12"'), '12');
        $this->assertSame(syck_load('"\
1\
2"'), '12');
        $this->assertSame(syck_load("'12'"), '12');
    }

    public function testInteger()
    {
        $this->assertSame(syck_load('685230'), 685230);
        $this->assertSame(syck_load('+685,230'), 685230);
        $this->assertSame(syck_load('02472256'), 685230);
        $this->assertSame(syck_load('0x,0A,74,AE'), 685230);
        $this->assertSame(syck_load('0xF00D'), 0xF00D);
        $this->assertSame(syck_load('07654321'), 07654321);
        $this->assertSame(syck_load('190:20:30'), 685230);
    }

    public function testFloatFix()
    {
        $this->assertSame(syck_load('99.0'), 99.0);
        $this->assertSame(syck_load('!float 99'), 99.0);
        $this->assertSame(syck_load('190:20:30.15'), 6.8523015e+5);
    }

    public function testFloatExponential()
    {
        $this->assertSame(syck_load('1.0e+1'), 10.0);
        $this->assertSame(syck_load('1.0e-1'), 0.1);
    }

    public function testInfinity()
    {
        $this->assertSame(syck_load('.inf'), INF);
        $this->assertSame(syck_load('.Inf'), INF);
        $this->assertSame(syck_load('.INF'), INF);
    }

    public function testNegativeInfinity()
    {
        $this->assertSame(syck_load('-.inf'), -INF);
        $this->assertSame(syck_load('-.Inf'), -INF);
        $this->assertSame(syck_load('-.INF'), -INF);
    }

    public function testNan()
    {
        // NAN !== NAN, but NAN == NAN
        $this->assertEquals(syck_load('.nan'), NAN);
        $this->assertEquals(syck_load('.NaN'), NAN);
        $this->assertEquals(syck_load('.NAN'), NAN);
    }

    public function testTimestamps()
    {
        // canonical
        $this->assertType('DateTime', syck_load("2001-12-15T02:59:43.1Z"));
        // iso8601
        $this->assertType('DateTime', syck_load("2001-12-14t21:59:43.10-05:00"));
        // spaced
        $this->assertType('DateTime', syck_load("2001-12-14 21:59:43.10 -05"));
        // date
        $this->assertType('DateTime', syck_load("2002-12-14"));

        // explicit
        $this->assertType('DateTime', syck_load("!php/object::Datetime 2002-12-14"));
    }

    public function testArray()
    {
        $this->assertEquals(syck_load('[]'), array());
        $this->assertEquals(syck_load('[a, b, c]'), array('a', 'b', 'c'));
        $this->assertEquals(syck_load('!php/array []'), array());

        // ArrayObject implements ArrayAccess: OK
        $this->assertEquals(syck_load('!php/array::ArrayObject []'), new ArrayObject());
        $this->assertEquals(syck_load('!php/array::ArrayObject [1, 2, 3]'), new ArrayObject(array(1, 2, 3)));

        // SyckTestSomeClass doesn't implement ArrayAccess: FAILURE
        try {
            syck_load('!php/array::SyckTestSomeClass []');
            $this->assertTrue(false);
        } catch (SyckException $e) {
            $this->assertTrue(true);
        }

        // SyckTestOtherClass doesn't exist: FAILURE
        try {
            syck_load('!php/array::SyckTestOtherClass []');
            $this->assertTrue(false);
        } catch (SyckException $e) {
            $this->assertTrue(true);
        }
    }

    public function testHash()
    {
        $this->assertEquals(syck_load('{}'), array());
        $this->assertEquals(syck_load('{0: a, 1: b, 2: c}'), array('a', 'b', 'c'));

        // ArrayObject implements ArrayAccess: OK
        $this->assertEquals(new ArrayObject(), syck_load('!php/hash::ArrayObject {}'));
        $this->assertEquals(
            new ArrayObject(array('a' => 1, 'b' => 2, 3 => 3, 4 => 'd', 'e' => 5)),
            syck_load('!php/hash::ArrayObject {a: 1, b: 2, 3: 3, 4: d, e: 5}')
        );

        // SyckTestSomeClass doesn't implement ArrayAccess: FAILURE
        try {
            syck_load('!php/hash::SyckTestSomeClass {}');
            $this->assertTrue(false);
        } catch (SyckException $e) {
            $this->assertTrue(true);
        }

        // SyckTestOtherClass doesn't exist: FAILURE
        try {
            syck_load('!php/hash::SyckTestOtherClass {}');
            $this->assertTrue(false);
        } catch (SyckException $e) {
            $this->assertTrue(true);
        }
    }

    public function testSerializable()
    {
        $obj = syck_load('!php/object::MySerializable teststring');

        $this->assertType('MySerializable', $obj);
        $this->assertSame('teststring', $obj->test());
    }
}
