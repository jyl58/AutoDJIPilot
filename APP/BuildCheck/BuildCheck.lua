home_path=os.getenv("HOME");
package.path=home_path.."/AutoDJIPilot/APP/common/tools.lua;"
require(tools.lua)
require(geo.lua)

CHECK_FLY_MAX_SPEED=2.0
CHECK_FLY_MIN_SPEED=0.3
CHECK_BREAK_BOUNDARY=2.0

PCODE_FILE_PATH=home_path.."/AutoDJIPilot/APP/BuildCheck/PathCode/Path.pcode

--open the pcode file
local pcode_handler=io.open(PCODE_FILE_PATH,"r")
if pcode_handler == nil then
	print("open the pcode file"..PCODE_FILE_PATH.." err")
	return;
end

--read the origin point lat lon and alt
local origin_lat,origin_lon,origin_alt;
for line in pcode_handler:lines() do
	-- split with space
	line_tab=stringSplit(line," ");
	if line_tab[1] == "O:" then 
		origin_lat=tonumber(line_tab[2]);
		origin_lon=tonumber(line_tab[3]);
		origin_alt=tonumber(line_tab[4]);
		break;
	end
end

-- take off vehicle
LuaTakeoff();

--fly to origin 
LuaFlyByGPS(origin_lat,origin_lon);

--set the gimbal angle. reletive vehicle head  
LuaSetGimbalAngle(-90);

-- start video record
LuaVideoStart();

-- read the path point 
local target_x,target_y,target_z;
for path in pcode_hanlder:lines() do
	path_tab=stringSplit(path," ");
	if path_tab[1] == "P1" then
		target_x=tonumber(string.sub(path_tab[2],2,-1));
		target_y=tonumber(string.sub(path_tab[3],2,-1));
		target_z=tonumber(string.sub(path_tab[4],2,-1));
		<--turn the head to next point-->
		turnVehicleHead(target_x,target_y);
		<--TODO: add fly commd function-->
		flyToTargetPoint(target_x,target_y,target_z);
	end
	<--TODO: add gimbal commd function-->
end
--close the pcode
pcode_handler:close()
--stop video record
LuaVideoStop();

-- go home
LuaGoHome();

<--flight done-->

---function 
function turnVehicleHead(target_x,target_y)
	local current_lat,current_lon,current_alt;
	local current_x_reletive_origin,current_y_reletive_origin;
	local remaing_x,remaing_y,remaing_z;
	-- get current pos. unit:deg,deg,m
	current_lat,current_lon,current_alt=LuaGetLocationGPS();
	current_x_reletive_origin,current_y_reletive_origin=GEO.get_vector_to_next_waypoint(origin_lat,origin_lon,current_lat,current_lon);
		
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
		current_x_reletive_origin,current_y_reletive_origin=GEO.get_vector_to_next_waypoint(origin_lat,origin_lon,current_lat,current_lon);
		current_z_reletive_origin=current_alt-origin_alt;
		
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

