#include <opendht.h>

#include <csignal>
#include <cstdlib>
#include <iostream>

#define ENV_DHT_NODE_PORT	"DHT_NODE_PORT"
#define ERR_DHT_PORT_NSET	"The env variable DHT_NODE_PORT is not set in the current system."
#define MSG_ABORT			"Abort!"
#define MSG_BOOSTRAP_CLOSE	"Bootstrap node will be close down..."
#define MSG_LISTENING		"Bootstrap node is listening on port"
#define MSG_SIGTERM			"SIGTERM received!"
#define MSG_TERMINATED		"Bootstrap node terminated!"

static bool __waitTermination = true;

void StopBootstrapNode(int sign) {
	std::cout << MSG_SIGTERM << " " << MSG_BOOSTRAP_CLOSE << std::endl;
	__waitTermination = false;
}

int main()
{
	// Initialize signal handler
	std::signal(SIGTERM, StopBootstrapNode);

	const char *envPort = std::getenv(ENV_DHT_NODE_PORT);

	if (envPort == nullptr) {
		std::cerr << ERR_DHT_PORT_NSET << " " << MSG_ABORT << std::endl;
		return 1;
	}

	uint port = std::atoi(envPort);

	dht::DhtRunner node;

	// Launch the dht node on a new thread, using a
	// generated RSA key pair, and listen on port set by the env variable DHT_NODE_PORT.
	node.run(port, dht::crypto::generateIdentity(), true);
	std::cout << MSG_LISTENING << " " << port << "..." << std::endl;
	// Wait until a SIGTERM is emitted
	while (__waitTermination);

	// Wait for dht threads to end
	node.join();
	std::cout << MSG_TERMINATED << std::endl;

	return 0;
}
