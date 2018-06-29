#include <opendht.h>

#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>

#define ENV_DHT_NODE_PORT	"DHT_NODE_PORT"

dht::DhtRunner node;

std::condition_variable cv;
std::mutex m;
std::atomic_bool done {false};

void StopBootstrapNode(int sign) {
	std::cout << "SIGTERM received! Waiting shutdown... ";
	node.shutdown([]{
		std::cout << "Done" << std::endl; 
        std::lock_guard<std::mutex> lk(m);
        done = true;
        cv.notify_one();
    });
}

int main()
{
	// Initialize signal handler
	std::signal(SIGTERM, StopBootstrapNode);

	const char *envPort = std::getenv(ENV_DHT_NODE_PORT);

	if (envPort == nullptr) {
		std::cerr << "The env variable DHT_NODE_PORT is not set in the current system. Abort!" << std::endl;
		return 1;
	}

	uint port = std::atoi(envPort);

	// Launch the dht node on a new thread, using a
	// generated RSA key pair, and listen on port set by the env variable DHT_NODE_PORT.
	node.run(port, dht::crypto::generateIdentity(), true);
	std::cout << "Bootstrap node is listening on port " << port << "..." << std::endl;
	
    // Wait for shutdown
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, []{ return done.load(); });

	// Wait dht thread ending
	node.join();
	std::cout << "Bootstrap node terminated!" << std::endl;

	return 0;
}
