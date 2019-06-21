LuaTakeoff()
print("finish takeoff")
LuaClimbBy(5)
alt=LuaGetAlt()
print("current alt:",alt)
LuaFlyByBearingAndDistance(90,5)
lat,lon,alt=LuaGetLocationGPS()

print("lat,lon,alt",lat,lon,alt)

LuaGoHome()
