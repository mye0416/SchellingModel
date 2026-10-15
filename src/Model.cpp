/* Demo_03_Model.cpp */

#include <stdio.h>
#include <vector>
#include <boost/mpi.hpp>
#include "repast_hpc/AgentId.h"
#include "repast_hpc/RepastProcess.h"
#include "repast_hpc/Utilities.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/initialize_random.h"
#include "repast_hpc/SVDataSetBuilder.h"
#include "repast_hpc/Point.h"
#include "repast_hpc/Random.h"

#include "Model.h"

AgentPackageProvider::AgentPackageProvider(repast::SharedContext<Agent>* agentPtr):
    agents(agentPtr) {}
void AgentPackageProvider::providePackage(Agent* agent, std::vector<AgentPackage>& out) {
    repast::AgentId id = agent->getId();
    AgentPackage package(id.id(), id.startingRank(), id.agentType(),
                         id.currentRank(), agent->getType(),
                         agent->getSatisfiedStatus());
    out.push_back(package);
}
void AgentPackageProvider::provideContent(repast::AgentRequest req, std::vector<AgentPackage>& out) {
    std::vector<repast::AgentId> ids = req.requestedAgents();
    for (size_t i = 0; i < ids.size(); i++) {
        providePackage(agents->getAgent(ids[i]), out);
    }
}
AgentPackageReceiver::AgentPackageReceiver(repast::SharedContext<Agent>* agentPtr):
    agents(agentPtr) {}
Agent* AgentPackageReceiver::createAgent(AgentPackage package) {
    repast::AgentId id(package.id, package.rank, package.type, package.currentRank);
    return new Agent(id, package.agentType, package.isSatisfied);
}
void AgentPackageReceiver::updateAgent(AgentPackage package) {
    repast::AgentId id(package.id, package.rank, package.type);
    Agent* agent = agents->getAgent(id);
    agent->set(package.currentRank, package.agentType, package.isSatisfied);
}

DataSource_AgentSatisfiedTotals::DataSource_AgentSatisfiedTotals(
    repast::SharedContext<Agent>* c) : context(c) {}

int DataSource_AgentSatisfiedTotals::getData() {
    int sum = 0;

    repast::SharedContext<Agent>::const_local_iterator iter =
        context->localBegin();

    repast::SharedContext<Agent>::const_local_iterator iterEnd =
        context->localEnd();

    while (iter != iterEnd) {
        sum += (*iter)->getSatisfiedStatus();
        iter++;
    }
    return sum;
}
SchellingModel::SchellingModel(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm): context(comm){
	props = new repast::Properties(propsFile, argc, argv, comm);
	stopAt = repast::strToInt(props->getProperty("stop.at"));
	countOfAgents = repast::strToInt(props->getProperty("count.of.agents"));
	boardSize = repast::strToInt(props->getProperty("board.size"));
	initializeRandom(*props, comm);

	
repast::Point<double> origin(0, 0);
repast::Point<double> extent(boardSize, boardSize);
repast::GridDimensions gd(origin, extent);

int procX = repast::strToInt(props->getProperty("proc.per.x"));
int procY = repast::strToInt(props->getProperty("proc.per.y"));
int bufferSize = repast::strToInt(props->getProperty("grid.buffer"));

std::vector<int> processDims;
processDims.push_back(procX);
processDims.push_back(procY);

discreteSpace = new repast::SharedDiscreteSpace<
    Agent, repast::StrictBorders, repast::SimpleAdder<Agent>
>("AgentDiscreteSpace", gd, processDims, bufferSize, comm);

printf("Rank %d. Bounds (global): (%.1f,%.1f) & (%.1f,%.1f). Dimensions (local): (%.1f,%.1f) & (%.1f,%.1f).\n",
       repast::RepastProcess::instance()->rank(),
       discreteSpace->bounds().origin().getX(),
       discreteSpace->bounds().origin().getY(),
       discreteSpace->bounds().extents().getX(),
       discreteSpace->bounds().extents().getY(),
       discreteSpace->dimensions().origin().getX(),
       discreteSpace->dimensions().origin().getY(),
       discreteSpace->dimensions().extents().getX(),
       discreteSpace->dimensions().extents().getY());

context.addProjection(discreteSpace);
provider = new AgentPackageProvider(&context);
receiver = new AgentPackageReceiver(&context);

// Data collection
std::string fileOutputName("./outputs/agent_total_data.csv");

repast::SVDataSetBuilder builder(
    fileOutputName.c_str(),
    ",",
    repast::RepastProcess::instance()->getScheduleRunner().schedule()
);

DataSource_AgentSatisfiedTotals* agentSatisfiedTotals_DataSource =
    new DataSource_AgentSatisfiedTotals(&context);

builder.addDataSource(
    createSVDataSource(
        "Total Satisfied",
        agentSatisfiedTotals_DataSource,
        std::plus<int>()
    )
);

agentValues = builder.createDataSet();
}

SchellingModel::~SchellingModel(){
	delete props;
    delete provider;
    delete receiver;
    delete agentValues;
}

void SchellingModel::initAgents(){
/*
int rank = repast::RepastProcess::instance()->rank();

repast::IntUniformGenerator gen =
   repast::Random::instance()->createUniIntGenerator(0, boardSize - 1);

int countType0 = countOfAgents / 2;
int countType1 = countOfAgents - countType0;

double threshold =
    repast::strToDouble(props->getProperty("threshold"));

for (int i = 0; i < countOfAgents; i++) {

    // Randomly find an empty location
    int xRand, yRand;
    std::vector<Agent*> agentList;

    do {
        agentList.clear();

        xRand = gen.next();
        yRand = gen.next();

        discreteSpace->getObjectsAt(
            repast::Point<int>(xRand, yRand), agentList
        );

    } while (agentList.size() != 0);

    // Create an agent and assign its type
    repast::Point<int> initialLocation(xRand, yRand);

    repast::AgentId id(i, rank, 0);
    id.currentRank(rank);

    int type;

    if (countType0 > 0) {
        type = 0;
        countType0--;
    } else {
        type = 1;
        countType1--;
    }

    Agent* agent = new Agent(id, type, threshold);

    context.addAgent(agent);
    discreteSpace->moveTo(id, initialLocation);
}
    }
*/
int rank = repast::RepastProcess::instance()->rank();

int xMin = ceil(discreteSpace->dimensions().origin().getX());
int xMax = floor(discreteSpace->dimensions().origin().getX() +
                 discreteSpace->dimensions().extents().getX() - 0.00001);

int yMin = ceil(discreteSpace->dimensions().origin().getY());
int yMax = floor(discreteSpace->dimensions().origin().getY() +
                 discreteSpace->dimensions().extents().getY() - 0.00001);

repast::IntUniformGenerator genX =
    repast::Random::instance()->createUniIntGenerator(xMin, xMax);

repast::IntUniformGenerator genY =
    repast::Random::instance()->createUniIntGenerator(yMin, yMax);

double threshold =
    repast::strToDouble(props->getProperty("threshold"));

for (int i = 0; i < countOfAgents; i++) {

    int x = genX.next();
    int y = genY.next();

    if (i == 0 && rank == 0) {
        x = 1; y = 2;
    } else if (i == 0 && rank == 1) {
        x = 2; y = 3;
    } else if (i == 0 && rank == 2) {
        x = 4; y = 2;
    } else if (i == 0 && rank == 3) {
        x = 5; y = 5;
    }

    repast::Point<int> initialLocation(x, y);

    repast::AgentId id(i, rank, 0);
    id.currentRank(rank);

    Agent* agent = new Agent(id, 0, threshold);
    agent->set(rank, 0, false);

    context.addAgent(agent);
    discreteSpace->moveTo(id, initialLocation);

    std::vector<int> agentLoc;
    discreteSpace->getLocation(id, agentLoc);

    printf("Init: Agent %d,%d,%d - at (%d,%d).\n",
           id.id(), id.startingRank(), id.currentRank(),
           agentLoc[0], agentLoc[1]);
}
}
void SchellingModel::doPerTick(){
/*
// Calculate average satisfaction
double avgSatisfied = 0;

std::vector<Agent*> agents;

context.selectAgents(
    repast::SharedContext<Agent>::LOCAL,
    countOfAgents,
    agents
);

std::vector<Agent*>::iterator it = agents.begin();

while (it != agents.end()) {
    (*it)->updateStatus(&context, discreteSpace);
    avgSatisfied += (*it)->getSatisfiedStatus();
    it++;
}

avgSatisfied /= countOfAgents;

// Print average satisfaction
double currentTick =
    repast::RepastProcess::instance()
        ->getScheduleRunner().currentTick();

if (repast::RepastProcess::instance()->rank() == 0) {
    printf("Tick: %.1f\tAvg satisfied: %.2f\n",
           currentTick, avgSatisfied);

    if (currentTick == 1 || currentTick == stopAt || avgSatisfied == 1)
        printToScreen();

    if (avgSatisfied == 1)
        repast::RepastProcess::instance()->getScheduleRunner().stop();
}

// Move dissatisfied agents
it = agents.begin();

while (it != agents.end()) {
    if (!(*it)->getSatisfiedStatus()) {
        (*it)->move(discreteSpace);
    }
    it++;
}
*/
discreteSpace->balance();

repast::RepastProcess::instance()
    ->synchronizeAgentStatus<Agent, AgentPackage,
        AgentPackageProvider, AgentPackageReceiver>(
            context, *provider, *receiver, *receiver);

repast::RepastProcess::instance()
    ->synchronizeProjectionInfo<Agent, AgentPackage,
        AgentPackageProvider, AgentPackageReceiver>(
            context, *provider, *receiver, *receiver);

repast::RepastProcess::instance()
    ->synchronizeAgentStates<AgentPackage,
        AgentPackageProvider, AgentPackageReceiver>(
            *provider, *receiver);

int rank = repast::RepastProcess::instance()->rank();
double currentTick =
    repast::RepastProcess::instance()->getScheduleRunner().currentTick();

if (rank == 0 || rank == 1) {
    printf("Tick %.1f - Rank %d - LOCAL\n", currentTick, rank);

    repast::SharedContext<Agent>::const_state_aware_iterator iter =
        context.begin(repast::SharedContext<Agent>::LOCAL);
    repast::SharedContext<Agent>::const_state_aware_iterator end =
        context.end(repast::SharedContext<Agent>::LOCAL);

    while (iter != end) {
        Agent* agent = (&**iter);
        repast::AgentId id = agent->getId();
        std::vector<int> loc;
        discreteSpace->getLocation(id, loc);

        printf("Agent %d,%d,%d - at (%d,%d) - satisfied %d.\n",
               id.id(), id.startingRank(), id.currentRank(),
               loc[0], loc[1], agent->getSatisfiedStatus());
        iter++;
    }

    printf("Tick %.1f - Rank %d - NON_LOCAL\n", currentTick, rank);

    iter = context.begin(repast::SharedContext<Agent>::NON_LOCAL);
    end = context.end(repast::SharedContext<Agent>::NON_LOCAL);

    while (iter != end) {
        Agent* agent = (&**iter);
        repast::AgentId id = agent->getId();
        std::vector<int> loc;
        discreteSpace->getLocation(id, loc);

        printf("Agent %d,%d,%d - at (%d,%d) - satisfied %d.\n",
               id.id(), id.startingRank(), id.currentRank(),
               loc[0], loc[1], agent->getSatisfiedStatus());
        iter++;
    }
}
if (currentTick == 1 && rank == 0) {
    repast::AgentId id(0, 0, 0);

    std::vector<int> agentNewLoc;
    agentNewLoc.push_back(0);
    agentNewLoc.push_back(3);

    discreteSpace->moveTo(id, agentNewLoc);
}
}

void SchellingModel::initSchedule(repast::ScheduleRunner& runner) {
    runner.scheduleEvent(1, 1,
        repast::Schedule::FunctorPtr(
            new repast::MethodFunctor<SchellingModel>(
                this, &SchellingModel::doPerTick)));

    runner.scheduleStop(stopAt);

    // Data collection
    runner.scheduleEvent(0, 1,
        repast::Schedule::FunctorPtr(
            new repast::MethodFunctor<repast::DataSet>(
                agentValues, &repast::DataSet::record)));

    runner.scheduleEvent(1, 1,
        repast::Schedule::FunctorPtr(
            new repast::MethodFunctor<repast::DataSet>(
                agentValues, &repast::DataSet::write)));

    runner.scheduleEndEvent(
        repast::Schedule::FunctorPtr(
            new repast::MethodFunctor<repast::DataSet>(
                agentValues, &repast::DataSet::write)));
}

void SchellingModel::printToScreen() {

    // Print the board to screen
    std::vector<Agent*> agentList;

    for (int i = 0; i <= boardSize + 1; i++) {
        for (int j = 0; j <= boardSize + 1; j++) {

            if (i == 0 || i == boardSize + 1)
                std::cout << "-";

            else if (j == 0 || j == boardSize + 1)
                std::cout << "|";

            else {
                agentList.clear();

                discreteSpace->getObjectsAt(
                    repast::Point<int>(i, j), agentList
                );

                if (agentList.size() > 1) {
                    std::cerr << "More than 1 agent per cell"
                              << std::endl;
                }

                if (agentList.size() == 0)
                    std::cout << " ";

                else if (agentList.front()->getType() == 0)
                    std::cout << "X";

                else if (agentList.front()->getType() == 1)
                    std::cout << ".";
            }
        }

        std::cout << std::endl;
    }
}