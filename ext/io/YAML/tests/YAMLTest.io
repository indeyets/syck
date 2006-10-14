YAML

YAMLTest := UnitTest clone do(
	type := "YAMLTests"
	
	test_1_open := method(
		self name := "myDatabase.sqlite3"
		File clone setPath(name) remove
		self db := YAML clone
		//db debugOn		
		db setPath(name)
		db open
		assertTrue(db isOpen)
	)
)
