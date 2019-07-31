--GEO tools tab
GEO={}
--meta table
GEO.mt={
	__index=function () 
				error(" :Read the param is invalid",0); 
			end,
	__newindex=function()
				error(" :Set the param is invalid",0);
			end
}
--earth radius
GEO.CONSTANTS_RADIUS_OF_EARTH=6378137.0; -- unit: m
--convet to (0,2pi)
GEO.wrap_2PI=function (angle_rad)
	local res = math.fmod(angle_rad, 2*math.pi);
    if res < 0 then 
        res =res+2*math.pi;
    end
    return res;
end
--convet to (-pi,pi)
GEO.wrap_PI=function (angle_rad)
	local res = GEO.wrap_2PI(angle_rad);
	if res > math.pi then 
        res = res-2*math.pi
    end
    return res;
end
--get target point us breaing and distance
GEO.waypoint_from_heading_and_distance=function(lat_start,lon_start,head,distance)
	local bearing = GEO.wrap_2PI(head);
	local radius_ratio = math.abs(distance) / GEO.CONSTANTS_RADIUS_OF_EARTH;
	local lat_start_rad = math.rad(lat_start);
	local lon_start_rad = math.rad(lon_start);

	local lat_target = math.asin(math.sin(lat_start_rad) * math.cos(radius_ratio) + math.cos(lat_start_rad) * math.sin(radius_ratio) * math.cos(bearing));
	local lon_target = lon_start_rad + math.atan(math.sin(bearing) * math.sin(radius_ratio) * math.cos(lat_start_rad), math.cos(radius_ratio) - math.sin(lat_start_rad) * math.sin(lat_target));

	local lat_target = math.deg(lat_target);
	local lon_target = math.deg(lon_target);
	return lat_target,lon_target;
end
--get target use vector
GEO.waypoint_from_vector=function (lat_start,lon_start,vn,ve)
	local lat_start_rad = math.rad(lat_start);
	local lon_start_rad = math.rad(lon_start);
	local target_lat_rad=(lat_start_rad + v_n / GEO.CONSTANTS_RADIUS_OF_EARTH);
	local target_lon_rad= (lon_start_rad + v_e / (GEO.CONSTANTS_RADIUS_OF_EARTH * math.cos(lat_start_rad)));
	local target_lat=math.deg(target_lat_rad);
	local target_lon=math.deg(target_lon_rad);
	return target_lat,target_lon;
end
--get distance to next point
GEO.get_distance_to_next_waypoint=function (lat_now,lon_now,lat_next,lon_next)
    local lat_now_rad = math.rad(lat_now);
    local lon_now_rad = math.rad(lon_now);
    local lat_next_rad = math.rad(lat_next);
    local lon_next_rad = math.rad(lon_next);

    local d_lat = lat_next_rad - lat_now_rad;
    local d_lon = lon_next_rad - lon_now_rad;
	
	local a = math.sin(d_lat / 2.0) * math.sin(d_lat /2.0) + math.sin(d_lon / 2.0) * math.sin(d_lon /2.0) * math.cos(lat_now_rad) * math.cos(lat_next_rad);
	
    local c = 2.0 * math.atan(math.sqrt(a), math.sqrt(1.0 - a));

    return GEO.CONSTANTS_RADIUS_OF_EARTH * c;
end
--get bearing to next way point
GEO.get_bearing_to_next_waypoint=function (lat_now, lon_now, lat_next, lon_next)

    local lat_now_rad = math.rad(lat_now);
	local lon_now_rad = math.rad(lon_now);
    local lat_next_rad = math.rad(lat_next);
    local lon_next_rad = math.rad(lon_next);

    local d_lon = lon_next_rad - lon_now_rad;

    local theta = math.atan(math.sin(d_lon) * math.cos(lat_next_rad), math.cos(lat_now_rad) * math.sin(lat_next_rad) - math.sin(lat_now_rad) * math.cos(lat_next_rad) * math.cos(d_lon));

    local theta = GEO.wrap_PI(theta);

    return theta;
end
--get NE vector to next point
GEO.get_vector_to_next_waypoint=function ( lat_now,lon_now,lat_next,lon_next)
	local lat_now_rad =  math.rad(lat_now);
	local lon_now_rad =  math.rad(lon_now);
	local lat_next_rad = math.rad(lat_next);
	local lon_next_rad = math.rad(lon_next);

	local d_lon = lon_next_rad - lon_now_rad;

	local v_n = GEO.CONSTANTS_RADIUS_OF_EARTH * (math.cos(lat_now_rad) * math.sin(lat_next_rad) - math.sin(lat_now_rad) * math.cos(lat_next_rad) * math.cos(d_lon));
	local v_e = GEO.CONSTANTS_RADIUS_OF_EARTH * math.sin(d_lon) * math.cos(lat_next_rad);
	return v_n,v_e
end

return GEO
