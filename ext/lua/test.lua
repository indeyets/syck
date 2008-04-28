
require "yaml"
require "lunit"

lunit.import "all"

local testcase = lunit.TestCase("LuaYAML Testcases")

function testcase.test_load()
	assert_error(function() yaml.load() end)
	assert_nil(yaml.load("--- "))
	assert_true(yaml.load("--- true"))
	assert_false(yaml.load("--- false"))
	assert_equal(10, yaml.load("--- 10"))
	assert_equal(10.3321, yaml.load("--- 10.3321"))
	local t = yaml.load("--- \n- 5\n- 10\n- 15")
	assert_equal(5, t[1])
	assert_equal(10, t[2])
	assert_equal(15, t[3])
	local t = yaml.load("--- \n- one\n- two\n- three")
	assert_equal("one", t[1])
	assert_equal("two", t[2])
	assert_equal("three", t[3])
	local t = yaml.load("--- \nthree: 15\ntwo: 10\none: 5")
	assert_equal(5, t.one)
	assert_equal(10, t.two)
	assert_equal(15, t.three)
	local t = yaml.load("--- \nints: \n  - 1\n  - 2\n  - 3\nmaps: \n  three: 3\n  two: 2\n  one: 1\nstrings: \n  - one\n  - two\n  - three")
	assert_equal(1, t.ints[1])
	assert_equal(2, t.ints[2])
	assert_equal(3, t.ints[3])
	assert_equal(1, t.maps.one)
	assert_equal(2, t.maps.two)
	assert_equal(3, t.maps.three)
	assert_equal("one", t.strings[1])
	assert_equal("two", t.strings[2])
	assert_equal("three", t.strings[3])
end

function testcase.test_dump()
	assert_equal("--- \n", yaml.dump(nil))
	assert_equal("--- hey\n", yaml.dump("hey"))
	assert_equal("--- 5\n", yaml.dump(5))
	assert_equal("--- 5.9991\n", yaml.dump(5.9991))
	assert_equal("--- true\n", yaml.dump(true))
	assert_equal("--- false\n", yaml.dump(false))
	assert_equal("--- \n- 5\n- 6\n- 7\n", yaml.dump({5, 6, 7}))

	local str = "--- \n- \n  - 1\n  - 2\n  - 3\n- \n  - 6\n  - 7\n  - 8\n"
	assert_equal(str, yaml.dump({{1, 2, 3}, {6, 7, 8}}))
	local str = "--- \n- \n  - 1\n  - 2\n  - 3\n- \n  - one\n  - two\n  - three\n"
	assert_equal(str, yaml.dump({{1, 2, 3}, {"one", "two", "three"}}))

	local str = "--- \none: 1\nthree: 3\ntwo: 2\n"
	assert_equal(str, yaml.dump({one=1, two=2, three=3}))
end

function testcase.test_file()
	local file = "test.dump"

	local f = assert(io.open(file, "w"))
	local obj = {5, 6, "hello"}

	yaml.dump(obj, f)
	f:close()

	local obj2, err = yaml.load_file(file)
	assert_nil(err)
	assert_equal(5, obj2[1])
	assert_equal(6, obj2[2])
	assert_equal("hello", obj2[3])

	yaml.dump_file(obj, file)
	local obj2, err = yaml.load_file(file)
	assert_nil(err)
	assert_equal(5, obj2[1])
	assert_equal(6, obj2[2])
	assert_equal("hello", obj2[3])
end

os.exit(lunit.run())
