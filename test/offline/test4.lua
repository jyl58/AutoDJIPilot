require(tools.lua)
require(geo.lua)

PCODE_FILE_PATH="/home/"

--open the pcode file
local pcode_handler=io.open(PCODE_FILE_PATH,"r")
if pcode_handler == nil then
	
end

--read the origin point lat lon and alt
for line in pcode_handler:read() do
	-- split with space
	line_tab=stringSplit(line," ");
	if line_tab[1] == "O:" then 
		print(line_tab[2]);
		print(line_tab[3]);
		print(line_tab[4]);
		break;
	end
	if line_tab[1] == "P1" then
		print(line_tab[2]);
		print(line_tab[3]);
		print(line_tab[4]);
	end
end

