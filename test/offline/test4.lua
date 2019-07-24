home_path=os.getenv("HOME");
package.path=home_path.."/AutoDJIPilot/APP/common/tools.lua";
require "tools"
require "GEO"

PCODE_FILE_PATH="/mnt/dietpi_userdata/Path.pcode";

--open the pcode file
local pcode_handler=io.open(PCODE_FILE_PATH,"r")
if pcode_handler == nil then
	print("open file err!")
	return
end

--read the origin point lat lon and alt
for line in pcode_handler:lines() do
	-- split with space
	line_tab=stringSplit(line," ");
	if line_tab[1] == "O:" then 
		print(tonumber(line_tab[2]));
		print(tonumber(line_tab[3]));
		print(tonumber(line_tab[4]));
	end
	if line_tab[1] == "P1" then
		print(tonumber(string.sub(line_tab[2],2,-1)));
		print(tonumber(string.sub(line_tab[3],2,-1)));
		print(tonumber(string.sub(line_tab[4],2,-1)));
	end
end

pcode_handler:close()
