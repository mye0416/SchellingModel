#include "Agent.h"
#include "repast_hpc/Moore2DGridQuery.h"
#include "repast_hpc/Point.h"
#include <stdio.h>

Agent::Agent(repast::AgentId id, int type, double threshold): agentId(id), agentType(type), threshold(threshold) {}

Agent::~Agent() {}


void Agent::updateStatus(repast::SharedContext<Agent>* context,
		repast::SharedDiscreteSpace<Agent, repast::StrictBorders, repast::SimpleAdder<Agent> >* space){
std::vector<Agent*> neighbourAgents;

// Get the agent's current location
std::vector<int> agentLoc;
space->getLocation(agentId, agentLoc);

// Find the 8 surrounding neighbours
repast::Point<int> center(agentLoc);
repast::Moore2DGridQuery<Agent> moore2DQuery(space);

moore2DQuery.query(center, 1, false, neighbourAgents);

// Count neighbours of the same type
int similarCount = 0;

std::vector<Agent*>::iterator tempAgent = neighbourAgents.begin();

while (tempAgent != neighbourAgents.end()) {

    if ((*tempAgent)->getType() == agentType) {
        similarCount++;
    }

    tempAgent++;
}

// Update satisfaction status
isSatisfied =
    ((double)similarCount / neighbourAgents.size() >= threshold);
}

void Agent::move(repast::SharedDiscreteSpace<Agent, repast::StrictBorders, repast::SimpleAdder<Agent> >* space){
// Get the agent's current location
std::vector<int> agentLoc;
space->getLocation(agentId, agentLoc);

// Set the range of random coordinates
int xMax = (int)space->bounds().extents(0) - 1;

repast::IntUniformGenerator gen =
    repast::Random::instance()->createUniIntGenerator(1, xMax);

std::vector<int> agentNewLoc;

// Find a valid empty location
do {
    agentNewLoc.clear();

    int xRand, yRand;
    std::vector<Agent*> agentList;

    do {
        agentList.clear();

        xRand = gen.next();
        yRand = gen.next();

        space->getObjectsAt(
            repast::Point<int>(xRand, yRand),
            agentList
        );

    } while (agentList.size() != 0);

    agentNewLoc.push_back(xRand);
    agentNewLoc.push_back(yRand);

} while (!space->bounds().contains(agentNewLoc));

// Move the agent
space->moveTo(agentId, agentNewLoc);
}

