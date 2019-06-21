
LuaTakeoff()
print("take off down")
LuaTurnHead(180)
print("reach target head point")
LuaClimbTo(10)
LuaTurnHead(0)
LuaClimbBy(-5)
print("at target alt")
--LuaFlyByGPS(36.5429,120.9579)

LuaGoHome()

print("land down")

