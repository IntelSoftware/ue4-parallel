"# ue4-parallel" 

[I] - Build steps
1. Right click on SchoolOfFish Unreal Engine Project File -> Generate Visual Studio project files
2. Double click on SchoolOfFish Unreal Engine Project File. Accept rebuild request
3. In Unreal Engine Editor. Settings -> Project Settings...
4. In opened dialogue choose Maps&Modes (at left side) and setup next:
	Default GameMode: BP_FlockingGameMode
	Editor Startup Map: underwater
	Game Default Map: underwater
	
[II] - Packaging steps:
1. In Unreal Engine Editor. File -> Package Project -> Windows -> Windows(64-bit)
2. Wait until project packaging

[III] - Launch game with parameters from command line:

List of command line arguments:

Available calcModes:
* CPU_SINGLE - CPU single-threaded calculation of flocking behaviour (default)
* CPU_MULTI - CPU multi-threaded calculation of flocking behaviour
* GPU_MULTI - GPU multi-threaded calculation of flocking behaviou
	
General settings to compute flocking behaviour
* agentCount - total instances of fish, 1000 by default
* maxVelocity - maximum velocity of fish, 2500 by default
* maxAcceleration - maximum acceleration of fish, 1500 by default
* radiusCohesion - Cohesion radius. The radius inside which the fish will tend to inside the circle (approach), 1000 by default
* radiusSeparation - Separation radius. The radius within which the fish will tend to avoid collisions, 120 by default
* radiusAlignment - Alignment radius. The radius inside which the fish will tend to follow in one direction, 240 by default

Gain factors for the three types of fish behavior.
* kCohesion - 100 by default
* kSeparation - 1 by default
* kAlignment - 20 by default

Flocking area size
* mapSizeX - size of area where fishes can flock. X axis, [-20000; 20000] by default
* mapSizeY - size of area where fishes can flock. X axis, [-20000; 20000] by default
* mapSizeZ - size of area where fishes can flock. X axis, [-3000; 0] by default

Examples:
* SchoolOfFish calcMode="GPU_MULTI" agentCount 10000 maxVelocity 1200 maxAcceleration 600 radiusCohesion 480 radiusSeparation 120 radiusAlignment 240 kCohesion 10 kSeparation 2 kAlignment 5

* SchoolOfFish calcMode="CPU_SINGLE" agentCount 1000

* SchoolOfFish calcMode="CPU_MULTI" agentCount 3000

