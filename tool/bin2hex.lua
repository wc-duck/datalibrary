src = ScriptArgs['src']
dst = ScriptArgs['dst']

local infile = io.open(src, "rb")
local outfile = io.open(dst, "wb")
local data = infile:read("*a")
infile:close()

function hexify_with_colon( str )
    local line = {}
    for j=1, #str do
        table.insert( line, string.format( "0x%02x", str:byte(j) ) )
    end
    return table.concat( line, ", " )
end

local lines = {}

i = 0
while i * 16 < #data do
    sub = string.sub( data, i * 16 + 1, i * 16 + 16 )
    table.insert( lines, hexify_with_colon( sub ) )
    i = i + 1
end

outfile:write( table.concat( lines, ",\n" ) )
