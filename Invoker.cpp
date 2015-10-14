#include "Invoker.h"

using namespace Mona;

ConnectionsManager::ConnectionsManager(Invoker& invoker):_invoker(invoker),Task(invoker),Startable("ServerManager") {
}

void ConnectionsManager::run(Exception& ex) {
	do {
		waitHandle();
	} while (sleep(DELAY_CONNECTIONS_MANAGER) != STOP);
}

void ConnectionsManager::handle(Exception& ex) { _invoker.manage(); }

Invoker::~Invoker() {
	Startable::stop();
}

// Start the socket manager if not started
bool Invoker::start() {
	if(Startable::running()) {
		ERROR("Invoker is already running, call stop method before");
		return false;
	}

	if(!sockets.running()) {
		Exception ex;
		if (!((Mona::SocketManager&)sockets).start(ex) || ex || !sockets.running())
			return false;
	}

	Exception ex;
	bool result;
	EXCEPTION_TO_LOG(result = Startable::start(ex, Startable::PRIORITY_HIGH), "Invoker");
	if (result)
		TaskHandler::start();
	return result;
}

unsigned int Invoker::addConnection(std::shared_ptr<RTMFPConnection> pConn) {
	_connections.push_back(pConn);
	return _connections.size(); // Index of a connection is the position in the vector + 1 (0 is reserved for errors)
}

std::shared_ptr<RTMFPConnection>	Invoker::getConnection(unsigned int index) {
	return _connections.at(index-1); // Index of a connection is the position in the vector + 1 (0 is reserved for errors)
}

void Invoker::removeConnection(unsigned int index) {
	if (index>0 && _connections.size()>=index)
		_connections.erase(_connections.begin()+(index-1));
}

unsigned int Invoker::count() {
	return _connections.size();
}

void Invoker::manage() {
	for(auto it : _connections) {
		it->manage();
	}
}

void Invoker::run(Exception& exc) {
	Exception exWarn, ex;
	if (!_manager.start(exWarn, Startable::PRIORITY_LOW))
		ex.set(exWarn);
	else if (exWarn)
		WARN(exWarn.error());
	while (!ex && sleep() != STOP)
		giveHandle(ex);
	if (ex)
		FATAL("Server, ", ex.error());

	_manager.stop();

	if (sockets.running())
		((Mona::SocketManager&)sockets).stop();

	poolThreads.join();

	// release memory
	((Mona::PoolBuffers&)poolBuffers).clear();
}