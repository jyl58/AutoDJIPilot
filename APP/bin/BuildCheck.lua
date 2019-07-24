---function 
function turnVehicleHead(target_x,target_y)
	local current_lat,current_lon,current_alt;
	local current_x_reletive_origin,current_y_reletive_origin;
	local remaing_x,remaing_y,remaing_z;
	-- get current pos. unit:deg,deg,m
	current_lat,current_lon,current_alt=LuaGetLocationGPS();
	current_x_reletive_origin,current_y_reletive_origin=GEO.get_vector_to_next_waypoint(PathFly.origin_lat,PathFly.origin_lon,current_lat,current_lon);
		
	remaing_x=target_x-current_x_reletive_origin;
	remaing_y=target_y-current_y_reletive_origin;
	if (remaing_y>0.01) then
		target_head_deg=math.deg(GEO.wrap_2PI(math.atan(remaing_x,remaing_y)));
		local current_head_deg=LuaGetHead();
		if (math.abs(current_head_deg-target_head_deg) > 1.0 ) then
			LuaTurnHead(target_head_deg);
		end
	end
end

function flyToTargetPoint(target_x,target_y,target_z)
	local current_lat,current_lon,current_alt;
	local current_x_reletive_origin,current_y_reletive_origin,current_z_reletive_origin;
	local remaing_x,remaing_y,remaing_z;
	local v_factor=0;
	local x_speed_cmd,y_speed_cmd,z_speed_cmd;
	while(true) do
		-- get current pos. unit:deg,deg,m
		current_lat,current_lon,current_alt=LuaGetLocationGPS();
		current_x_reletive_origin,current_y_reletive_origin=GEO.get_vector_to_next_waypoint(PathFly.origin_lat,PathFly.origin_lon,current_lat,current_lon);
		current_z_reletive_origin=current_alt-PathFly.origin_alt;
		
		remaing_x=target_x-current_x_reletive_origin;
		remaing_y=target_y-current_y_reletive_origin;
		remaing_z=target_z-current_z_reletive_origin;
		local distance=math.sqrt(remaing_x^2+remaing_y^2+remaing_z^2);
		if(distance < 0.5) then
			--reached the target point
			break;
		end
		--limit max fly speed
		if(distance>CHECK_BREAK_BOUNDARY) then
			v_factor = CHECK_FLY_MAX_SPEED;
		else
			v_factor = CHECK_FLY_MAX_SPEED*(distance/CHECK_BREAK_BOUNDARY);
		end
		--allot the speed weight
		if(distance > 0.01) then
			x_speed_cmd = v_factor*(remaing_x/distance)
			y_speed_cmd = v_factor*(remaing_y/distance)
			z_speed_cmd = v_factor*(remaing_z/distance)
		else
			x_speed_cmd = 0;
			y_speed_cmd = 0;
			z_speed_cmd = 0;
		end
		--send speed cmd
		LuaFlyByVelocity(x_speed_cmd,y_speed_cmd,z_speed_cmd);
		--sleep 10ms
		LuaDelay(10);
	end
end

home_path=os.getenv("HOME");
package.path=home_path.."/AutoDJIPilot/APP/common/tools.lua";
require "tools"
package.path=home_path.."/AutoDJIPilot/APP/common/GEO.lua";
require "GEO"

CHECK_FLY_MAX_SPEED=2.0;
CHECK_FLY_MIN_SPEED=0.3;
CHECK_BREAK_BOUNDARY=2.0;

PCODE_FILE_PATH="/mnt/dietpi_userdata/Path.pcode";

--open the pcode file
local pcode_handler=io.open(PCODE_FILE_PATH,"r")
if pcode_handler == nil then
	print("open the pcode file"..PCODE_FILE_PATH.." err")
	return;
end

--read the origin point lat lon and alt
PathFly={}

for line in pcode_handler:lines() do
	-- split with space
	line_tab=stringSplit(line," ");
	if line_tab[1] == "O:" then 
		PathFly.origin_lat=tonumber(line_tab[2]);
		PathFly.origin_lon=tonumber(line_tab[3]);
		PathFly.origin_alt=tonumber(line_tab[4]);
		break;
	end
end

-- take off vehicle
LuaTakeoff();
print("takeoff done")
--fly to origin 
LuaFlyByGPS(PathFly.origin_lat,PathFly.origin_lon);
print("arrive start point")
--set the gimbal angle. reletive vehicle head  
LuaSetGimbalAngle(0,0,-90);
print("gimbal set done")
-- start video record
LuaVideoStart();
print("video started")
-- read the path point 
for line in pcode_handler:lines() do
	-- split with space
	line_tab=stringSplit(line," ");
	if line_tab[1] == "P1" then
		PathFly.target_x = tonumber(string.sub(line_tab[2],2,-1));
		PathFly.target_y = tonumber(string.sub(line_tab[3],2,-1));
		PathFly.target_z = tonumber(string.sub(line_tab[4],2,-1));
		--[[turn the head to next point--]]
		turnVehicleHead(PathFly.target_x,PathFly.target_y);
		--[[TODO: add fly commd function--]]
		flyToTargetPoint(PathFly.target_x,PathFly.target_y,PathFly.target_z);
	end
	if line_tab[1]=="#Layer" then
		print("Fly at "..line_tab[2].." layer")
	end
	--[[TODO: add gimbal commd function--]]
end
--close the pcode
pcode_handler:close()
--stop video record

LuaVideoStop();
print("Done path fly,now go home")
-- go home
LuaGoHome();

--[[flight done--]]

