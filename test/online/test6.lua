LuaTakeoff()
print("take off down")
LuaTurnHead(180)
print("reach target head point")
LuaClimbTo(10)
LuaTurnHead(0)
LuaClimbBy(-5)
print("at target alt")
LuaFlyByBearingAndDistance(90,10)

LuaGoHome()

print("land down")

