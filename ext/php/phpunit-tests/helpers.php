<?php

class SyckTestSomeClass {}

class MySerializable implements Serializable
{
    private $string = null;

    public function __construct($string = null)
    {
        if (null === $string)
            throw new Exception('This is not supposed to be called implicitly');

        $this->string = $string;
    }

    public function serialize()
    {
        return $this->string;
    }

    public function unserialize($string)
    {
        $this->string = $string;
    }

    public function test()
    {
        return $this->string;
    }
}
