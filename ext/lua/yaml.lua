-- borrow from lposix
--
require "syck"

yaml = {}

function yaml.dump(obj, io)
	if io then
		io:write(syck.dump(obj))
	else
		return syck.dump(obj)
	end
end

function yaml.dump_file(obj, file)
	local f, err = io.open(file, "w")
	if not f then return nil, err end
	f:write(yaml.dump(obj))
	f:close()
end

function yaml.load(str)
	return syck.load(str)
end

function yaml.load_file(file)
	local f, err = io.open(file, "r")
	if not f then return nil, err end
	local ret = yaml.load(f:read("*all"))
	f:close()
	return ret
end

setmetatable(yaml, {__index=syck})