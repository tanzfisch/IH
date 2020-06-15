#include "IslandHopper.h"

#include <igor/igor.h>

int main()
{
	igor::startup();

    IslandHopper* islandHopper = new IslandHopper();
    islandHopper->run();
	delete islandHopper;
	
	igor::shutdown();

	return 0;
}
