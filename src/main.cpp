#include "IslandHopper.h"

#include <igor/igor.h>
using namespace Igor;

int main()
{
	Igor::startup();

    IslandHopper* islandHopper = new IslandHopper();
    islandHopper->run();
	delete islandHopper;
	
	Igor::shutdown();

	return 0;
}
